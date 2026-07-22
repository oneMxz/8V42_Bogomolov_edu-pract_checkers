#include "server.h"
#include <QDataStream>
#include <QDebug>
#include <QRandomGenerator>

Server::Server(QObject *parent)
    : QTcpServer(parent)
    , m_server(new QTcpServer(this))
{
    m_timer = new QTimer(this);
    m_timer->setInterval(1000);
    connect(m_timer, &QTimer::timeout, this, &Server::onTimerTick);
}

Server::~Server()
{
    stopServer();
}

bool Server::startServer(quint16 port)
{
    if (!listen(QHostAddress::Any, port)) {
        qDebug() << "Failed to start server:" << errorString();
        return false;
    }
    qDebug() << "Server started on port" << port;
    m_timer->start();
    return true;
}

void Server::stopServer()
{
    m_timer->stop();
    close();
    for (QTcpSocket *socket : m_clients) {
        socket->disconnectFromHost();
    }
    qDeleteAll(m_rooms);
    m_rooms.clear();
    qDeleteAll(m_players);
    m_players.clear();
}

void Server::incomingConnection(qintptr socketDescriptor)
{
    QTcpSocket *socket = new QTcpSocket(this);
    socket->setSocketDescriptor(socketDescriptor);
    m_clients.append(socket);

    connect(socket, &QTcpSocket::readyRead, this, &Server::onReadyRead);
    connect(socket, &QTcpSocket::disconnected, this, &Server::onClientDisconnected);
}

void Server::onClientDisconnected()
{
    QTcpSocket *socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket) return;

    Player *player = getPlayer(socket);
    if (player) {
        // Удаляем игрока из комнаты
        Room *room = getRoom(player->roomId);
        if (room) {
            room->players.removeAll(player);
            // Уведомляем остальных
            QByteArray data;
            QDataStream out(&data, QIODevice::WriteOnly);
            out << player->name;
            broadcastToRoom(room, MessageType::PlayerLeft, data);
        }
        m_players.remove(socket);
        delete player;
    }

    m_clients.removeAll(socket);
    socket->deleteLater();
    sendRoomListToAll();
}

void Server::onReadyRead()
{
    QTcpSocket *socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket) return;

    QDataStream in(socket);
    in.setVersion(QDataStream::Qt_6_0);

    while (socket->bytesAvailable() >= sizeof(quint8)) {
        quint8 type;
        in >> type;

        switch (static_cast<MessageType>(type)) {
        case MessageType::Connect:
            handleConnect(socket, in);
            break;
        case MessageType::CreateRoom:
            handleCreateRoom(socket);
            break;
        case MessageType::JoinRoom:
            handleJoinRoom(socket, in);
            break;
        case MessageType::MakeMove:
            handleMakeMove(socket, in);
            break;
        case MessageType::ChatMessage:
            handleChatMessage(socket, in);
            break;
        default:
            qDebug() << "Unknown message type:" << type;
            break;
        }
    }
}

void Server::handleConnect(QTcpSocket *socket, QDataStream &in)
{
    in.setVersion(QDataStream::Qt_6_0);

    QByteArray nameData;
    in >> nameData;
    QString playerName = QString::fromUtf8(nameData);

    Player *player = new Player();
    player->socket = socket;
    player->name = playerName;
    player->id = m_nextPlayerId++;
    m_players[socket] = player;

    qDebug() << "Player connected:" << playerName << "ID:" << player->id;

    QByteArray data;
    QDataStream out(&data, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_6_0);

    QByteArray messageData = QString("Connected to server!").toUtf8();
    out << messageData;
    sendToPlayer(socket, MessageType::ConnectionAccepted, data);
}

void Server::handleCreateRoom(QTcpSocket *socket)
{
    Player *player = getPlayer(socket);
    if (!player) return;

    Room *room = new Room();
    room->id = m_nextRoomId++;
    room->name = "Room " + QString::number(room->id);
    room->players.append(player);
    player->roomId = room->id;
    m_rooms[room->id] = room;

    qDebug() << "Room created:" << room->id << "by" << player->name;

    QByteArray data;
    QDataStream out(&data, QIODevice::WriteOnly);
    out << room->id << player->name;
    sendToPlayer(socket, MessageType::RoomCreated, data);
    sendRoomListToAll();
}

void Server::handleJoinRoom(QTcpSocket *socket, QDataStream &in)
{
    quint16 roomId;
    in >> roomId;

    Player *player = getPlayer(socket);
    if (!player) return;

    Room *room = getRoom(roomId);
    if (!room) {
        sendToPlayer(socket, MessageType::Error, QByteArray());
        return;
    }

    if (room->players.size() >= 5) {
        sendToPlayer(socket,MessageType::RoomFull, QByteArray());
        return;
    }

    room->players.append(player);
    player->roomId = roomId;

    qDebug() << "Player" << player->name << "joined room" << roomId;

    // Уведомляем всех в комнате
    QByteArray data;
    QDataStream out(&data, QIODevice::WriteOnly);
    out << roomId << player->name;
    broadcastToRoom(room,MessageType::PlayerJoined, data);

    // Отправляем игроку подтверждение
    data.clear();
    out.device()->reset();
    out << roomId << player->name;
    sendToPlayer(socket, MessageType::RoomJoined, data);

    // Если в комнате 2+ игрока — можно начать игру
    if (room->players.size() >= 2 && !room->isGameActive) {
        startGame(room);
        sendRoomListToAll();
    }
}

void Server::startGame(Room *room)
{
    if (room->isGameActive) return;
    if (room->players.size() < 2) return;

    room->isGameActive = true;
    room->game.reset();

    // ===== СЛУЧАЙНЫЙ ВЫБОР ДВУХ ИГРОКОВ =====
    QVector<Player*> players = room->players;

    // Перемешиваем список игроков
    std::random_shuffle(players.begin(), players.end());

    // Первые два — игроки, остальные — наблюдатели
    Player* player1 = players[0];
    Player* player2 = players[1];

    // Назначаем цвета случайно
    bool whiteFirst = QRandomGenerator::global()->bounded(2) == 0;
    player1->isWhite = whiteFirst;
    player2->isWhite = !whiteFirst;
    player1->isSpectator = false;
    player2->isSpectator = false;

    // Остальные — наблюдатели
    for (int i = 2; i < players.size(); ++i) {
        players[i]->isSpectator = true;
        players[i]->isWhite = false;
    }

    // Запоминаем игроков в комнате
    room->playerWhite = player1->socket;
    room->playerBlack = player2->socket;

    qDebug() << "Game started in room" << room->id;
    qDebug() << "  Player1:" << player1->name << "(white:" << player1->isWhite << ")";
    qDebug() << "  Player2:" << player2->name << "(white:" << player2->isWhite << ")";
    qDebug() << "  Spectators:" << (players.size() - 2);

    // ===== УВЕДОМЛЯЕМ ВСЕХ =====
    // 1. Игра началась
    QByteArray data;
    QDataStream out(&data, QIODevice::WriteOnly);
    broadcastToRoom(room, MessageType::GameStarted, data);

    // 2. Назначаем цвета каждому игроку
    for (Player *player : room->players) {
        data.clear();
        out.device()->reset();
        out << player->isWhite;
        sendToPlayer(player->socket, MessageType::ColorAssigned, data);
    }

    // 3. Отправляем состояние игры
    sendGameState(room);

    // 4. Уведомляем, чей ход
    if (room->game.isWhiteTurn()) {
        sendToPlayer(room->playerWhite, MessageType::YourTurn, QByteArray());
        sendToPlayer(room->playerBlack, MessageType::OpponentTurn, QByteArray());
    } else {
        sendToPlayer(room->playerBlack, MessageType::YourTurn, QByteArray());
        sendToPlayer(room->playerWhite, MessageType::OpponentTurn, QByteArray());
    }
}

void Server::sendGameState(Room *room)
{
    BoardState state;
    memcpy(state.board, room->game.getBoard(), sizeof(state.board));
    state.isWhiteTurn = room->game.isWhiteTurn();
    state.whitePieces = room->game.countPieces(true);
    state.blackPieces = room->game.countPieces(false);
    state.whiteCaptured = room->game.getWhiteCaptured();
    state.blackCaptured = room->game.getBlackCaptured();

    QByteArray data;
    QDataStream out(&data, QIODevice::WriteOnly);
    out << state;
    broadcastToRoom(room, MessageType::GameState, data);
}

void Server::broadcastToRoom(Room *room, MessageType type, const QByteArray &data)
{
    QByteArray message;
    QDataStream out(&message, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_6_0);
    out << static_cast<quint8>(type);
    out << data;

    for (Player *player : room->players) {
        player->socket->write(message);
    }
}

void Server::sendToPlayer(QTcpSocket *socket, MessageType type, const QByteArray &data)
{
    QByteArray message;
    QDataStream out(&message, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_6_0);
    out << static_cast<quint8>(type);
    out << data;
    socket->write(message);
}

void Server::handleMakeMove(QTcpSocket *socket, QDataStream &in)
{
    Player *player = getPlayer(socket);
    if (!player) return;

    // ===== ПРОВЕРЯЕМ, ЧТО ИГРОК НЕ НАБЛЮДАТЕЛЬ =====
    if (player->isSpectator) {
        sendToPlayer(socket, MessageType::Error, QByteArray());
        return;
    }

    Room *room = getRoom(player->roomId);
    if (!room || !room->isGameActive) return;

    // Проверяем, что это ход текущего игрока
    bool isWhite = player->isWhite;
    if (isWhite != room->game.isWhiteTurn()) {
        return;
    }

    // Читаем ход
    NetworkMove move;
    in >> move;

    // Применяем ход
    GameLogic::Move gameMove(QPoint(move.fromCol, move.fromRow),
                             QPoint(move.toCol, move.toRow));

    if (room->game.makeMove(gameMove)) {
        // Отправляем новое состояние всем
        sendGameState(room);

        // Проверяем окончание игры
        checkGameOver(room);

        // Уведомляем о смене хода
        if (!room->isGameActive) return;

        if (room->game.isWhiteTurn()) {
            sendToPlayer(room->playerWhite,MessageType::YourTurn, QByteArray());
            sendToPlayer(room->playerBlack, MessageType::OpponentTurn, QByteArray());
        } else {
            sendToPlayer(room->playerBlack, MessageType::YourTurn, QByteArray());
            sendToPlayer(room->playerWhite, MessageType::OpponentTurn, QByteArray());
        }
    }
}

void Server::checkGameOver(Room *room)
{
    if (!room->game.isGameOver()) return;

    room->isGameActive = false;

    QString winner = room->game.isWhiteTurn() ? "Чёрные" : "Белые";

    QByteArray data;
    QDataStream out(&data, QIODevice::WriteOnly);
    out << winner;
    broadcastToRoom(room, MessageType::GameOver, data);

    qDebug() << "Game over in room" << room->id << "Winner:" << winner;
}

// ============================================================
// ВСПОМОГАТЕЛЬНЫЕ МЕТОДЫ
// ============================================================

Player* Server::getPlayer(QTcpSocket *socket)
{
    return m_players.value(socket, nullptr);
}

Room* Server::getRoom(quint16 roomId)
{
    return m_rooms.value(roomId, nullptr);
}

Room* Server::getRoomByPlayer(QTcpSocket *socket)
{
    Player *player = getPlayer(socket);
    if (!player) return nullptr;
    return getRoom(player->roomId);
}

// ============================================================
// ОБРАБОТКА СООБЩЕНИЙ
// ============================================================

void Server::handleChatMessage(QTcpSocket *socket, QDataStream &in)
{
    Player *player = getPlayer(socket);
    if (!player) return;

    Room *room = getRoom(player->roomId);
    if (!room) return;

    QByteArray messageData;
    in >> messageData;
    QString message = QString::fromUtf8(messageData);

    QByteArray data;
    QDataStream out(&data, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_6_0);
    QByteArray nameData = player->name.toUtf8();
    out << nameData;
    out << messageData;
    broadcastToRoom(room, MessageType::ChatBroadcast, data);
}

// ============================================================
// ТАЙМЕР
// ============================================================

void Server::onTimerTick()
{
    // Обновление времени для всех активных комнат
    for (Room *room : m_rooms) {
        if (!room->isGameActive) continue;

        // Уменьшаем время для текущего игрока
        if (room->game.isWhiteTurn()) {
            room->whiteTime--;
        } else {
            room->blackTime--;
        }

        // Отправляем обновление таймера всем в комнате
        QByteArray data;
        QDataStream out(&data, QIODevice::WriteOnly);
        out << room->whiteTime << room->blackTime;
        broadcastToRoom(room, MessageType::TimerUpdate, data);

        // Проверяем, не истекло ли время
        if (room->whiteTime <= 0 || room->blackTime <= 0) {
            // Определяем победителя по счёту
            QString winner;
            if (room->whiteTime <= 0 && room->blackTime <= 0) {
                if (room->game.getWhiteCaptured() > room->game.getBlackCaptured())
                    winner = "Белые";
                else if (room->game.getBlackCaptured() > room->game.getWhiteCaptured())
                    winner = "Черные";
                else
                    winner = "Ничья";
            } else if (room->whiteTime <= 0) {
                winner = "Черные";
            } else {
                winner = "Белые";
            }

            room->isGameActive = false;
            QByteArray gameOverData;
            QDataStream gameOverOut(&gameOverData, QIODevice::WriteOnly);
            gameOverOut << winner << "Время истекло!";
            broadcastToRoom(room, MessageType::GameOver, gameOverData);
        }
    }
}

void Server::sendRoomListToAll()
{
    // ===== СОБИРАЕМ СПИСОК КОМНАТ =====
    QByteArray data;
    QDataStream out(&data, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_6_0);

    // Сначала пишем количество комнат
    out << static_cast<quint16>(m_rooms.size());

    // Потом каждую комнату: id и количество игроков
    for (auto it = m_rooms.begin(); it != m_rooms.end(); ++it) {
        Room *room = it.value();
        out << room->id;
        out << static_cast<quint16>(room->players.size());
    }

    qDebug() << "Sending room list. Rooms count:" << m_rooms.size();

    for (QTcpSocket *socket : m_clients) {
        sendToPlayer(socket, MessageType::RoomList, data);
    }
}