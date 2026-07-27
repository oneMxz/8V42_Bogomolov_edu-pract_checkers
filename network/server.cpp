#include "server.h"
#include <QDataStream>
#include <QTimer>
#include <QRandomGenerator>
#include <algorithm>

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
    if (!listen(QHostAddress::Any, port))
        return false;
    m_timer->start();
    return true;
}

void Server::stopServer()
{
    m_timer->stop();
    close();
    for (QTcpSocket *socket : m_clients)
        socket->disconnectFromHost();
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
    Room *room = nullptr;

    if (player) {
        room = getRoom(player->roomId);
        if (room) {
            room->players.removeAll(player);
            QByteArray data;
            QDataStream out(&data, QIODevice::WriteOnly);
            out << player->name;
            broadcastToRoom(room, MessageType::PlayerLeft, data);

            if (room->isGameActive && !room->players.isEmpty()) {
                room->isGameActive = false;
                for (Player *p : room->players) {
                    p->roomId = 0;
                    p->isSpectator = false;
                    p->isWhite = false;
                }
                QByteArray gameOverData;
                QDataStream goOut(&gameOverData, QIODevice::WriteOnly);
                goOut << QString("Игрок покинул игру");
                broadcastToRoom(room, MessageType::GameOver, gameOverData);
            }
        }

        player->roomId = 0;
        m_players.remove(socket);
        delete player;
    }

    m_clients.removeAll(socket);
    socket->deleteLater();

    if (room && room->players.isEmpty()) {
        m_rooms.remove(room->id);
        delete room;
    }

    sendRoomListToAll();
}

void Server::onReadyRead()
{
    QTcpSocket *socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket) return;

    quint8 type;
    QByteArray data;
    while (SocketUtils::readMessage(socket, type, data)) {
        QDataStream in(data);
        in.setVersion(QDataStream::Qt_6_0);

        switch (static_cast<MessageType>(type)) {
        case MessageType::Connect:      handleConnect(socket, in); break;
        case MessageType::CreateRoom:   handleCreateRoom(socket); break;
        case MessageType::JoinRoom:     handleJoinRoom(socket, in); break;
        case MessageType::MakeMove:     handleMakeMove(socket, in); break;
        case MessageType::ChatMessage:  handleChatMessage(socket, in); break;
        case MessageType::LeaveRoom:    handleLeaveRoom(socket); break;
        default:
            // неизвестный тип – игнорируем
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

    QByteArray data;
    QDataStream out(&data, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_6_0);
    QByteArray messageData = QString("Connected to server!").toUtf8();
    out << messageData;
    sendToPlayer(socket, MessageType::ConnectionAccepted, data);
    sendRoomListToAll();
}

void Server::handleCreateRoom(QTcpSocket *socket)
{
    Player *player = getPlayer(socket);
    if (!player || player->roomId != 0) return;

    Room *room = new Room();
    room->id = m_nextRoomId++;
    room->name = "Room " + QString::number(room->id);
    room->players.append(player);
    player->roomId = room->id;
    m_rooms[room->id] = room;

    QByteArray data;
    QDataStream out(&data, QIODevice::WriteOnly);
    out << room->id;
    sendToPlayer(socket, MessageType::RoomCreated, data);
    sendRoomListToAll();
}

void Server::handleJoinRoom(QTcpSocket *socket, QDataStream &in)
{
    quint16 roomId;
    in >> roomId;

    Player *player = getPlayer(socket);
    if (!player || player->roomId != 0) return;

    Room *room = getRoom(roomId);
    if (!room) {
        sendToPlayer(socket, MessageType::Error, QByteArray());
        return;
    }
    if (room->isGameActive) {
        QByteArray data;
        QDataStream out(&data, QIODevice::WriteOnly);
        out << QString("Игра уже идёт, присоединиться нельзя.").toUtf8();
        sendToPlayer(socket, MessageType::Error, data);
        return;
    }
    if (room->players.size() >= 5) {
        sendToPlayer(socket, MessageType::RoomFull, QByteArray());
        return;
    }

    room->players.append(player);
    player->roomId = roomId;

    QByteArray data;
    QDataStream out(&data, QIODevice::WriteOnly);
    out << roomId << player->name;
    broadcastToRoom(room, MessageType::PlayerJoined, data);

    data.clear();
    out.device()->reset();
    out << roomId << player->name;
    sendToPlayer(socket, MessageType::RoomJoined, data);

    sendRoomListToAll();

    if (room->players.size() >= 2 && !room->isGameActive)
        startGame(room);
}

void Server::startGame(Room *room)
{
    if (room->isGameActive || room->players.size() < 2) return;

    room->isGameActive = true;
    room->game.reset();

    QVector<Player*> players = room->players;
    std::shuffle(players.begin(), players.end(), std::mt19937(std::random_device{}()));

    Player* player1 = players[0];
    Player* player2 = players[1];

    player1->isWhite = true;
    player2->isWhite = false;
    player1->isSpectator = false;
    player2->isSpectator = false;

    for (int i = 2; i < players.size(); ++i) {
        players[i]->isSpectator = true;
        players[i]->isWhite = false;
    }

    room->playerWhite = player1->socket;
    room->playerBlack = player2->socket;

    QByteArray data;
    QDataStream out(&data, QIODevice::WriteOnly);
    broadcastToRoom(room, MessageType::GameStarted, data);

    for (Player *player : room->players) {
        data.clear();
        out.device()->reset();
        out << player->isWhite;
        sendToPlayer(player->socket, MessageType::ColorAssigned, data);
    }

    sendGameState(room);

    QByteArray timerData;
    QDataStream timerOut(&timerData, QIODevice::WriteOnly);
    timerOut << room->whiteTime << room->blackTime;
    broadcastToRoom(room, MessageType::TimerUpdate, timerData);

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

    QPoint sel = room->game.getSelected();
    if (sel.x() != -1 && !room->game.getAvailableMoves().isEmpty()) {
        state.hasContinuation = true;
        state.selectedRow = sel.y();
        state.selectedCol = sel.x();
    } else {
        state.hasContinuation = false;
        state.selectedRow = -1;
        state.selectedCol = -1;
    }

    QByteArray data;
    QDataStream out(&data, QIODevice::WriteOnly);
    out << state;
    broadcastToRoom(room, MessageType::GameState, data);
}

void Server::broadcastToRoom(Room *room, MessageType type, const QByteArray &data)
{
    for (Player *player : room->players)
        sendToPlayer(player->socket, type, data);
}

void Server::sendToPlayer(QTcpSocket *socket, MessageType type, const QByteArray &data)
{
    SocketUtils::sendMessage(socket, static_cast<quint8>(type), data);
}

void Server::handleMakeMove(QTcpSocket *socket, QDataStream &in)
{
    Player *player = getPlayer(socket);
    if (!player || player->isSpectator) {
        if (player && player->isSpectator)
            sendToPlayer(socket, MessageType::Error, QByteArray());
        return;
    }

    Room *room = getRoom(player->roomId);
    if (!room || !room->isGameActive) return;

    if (player->isWhite != room->game.isWhiteTurn()) return;

    // Генерируем все допустимые ходы для текущего игрока
    QVector<GameLogic::Move> allMoves;
    bool isWhite = player->isWhite;
    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            int piece = room->game.getCell(r, c);
            bool isMine = (isWhite && room->game.isWhite(piece)) ||
                          (!isWhite && room->game.isBlackPiece(piece));
            if (isMine)
                allMoves.append(room->game.generateMoves(r, c));
        }
    }
    room->game.setAvailableMoves(allMoves);

    NetworkMove move;
    in >> move;

    // Проверяем, есть ли запрошенный ход в списке
    bool found = false;
    for (const auto &m : allMoves) {
        if (m.from.y() == move.fromRow && m.from.x() == move.fromCol &&
            m.to.y() == move.toRow && m.to.x() == move.toCol) {
            found = true;
            break;
        }
    }
    if (!found) {
        QByteArray errorData;
        QDataStream out(&errorData, QIODevice::WriteOnly);
        out << QString("Недопустимый ход").toUtf8();
        sendToPlayer(socket, MessageType::Error, errorData);
        return;
    }

    GameLogic::Move gameMove;
    for (const auto &m : allMoves) {
        if (m.from.y() == move.fromRow && m.from.x() == move.fromCol &&
            m.to.y() == move.toRow && m.to.x() == move.toCol) {
            gameMove = m;
            break;
        }
    }
    if (!gameMove.isValid()) return;

    if (room->game.makeMove(gameMove)) {
        sendGameState(room);
        checkGameOver(room);
        if (!room->isGameActive) return;

        // Проверяем продолжение рубки
        bool hasContinuation = false;
        QPoint selected = room->game.getSelected();
        if (selected.x() != -1) {
            QVector<GameLogic::Move> nextMoves = room->game.getAvailableMoves();
            hasContinuation = !nextMoves.isEmpty();
        }

        if (hasContinuation) {
            // Тот же игрок ходит снова
            if (room->game.isWhiteTurn()) {
                sendToPlayer(room->playerWhite, MessageType::YourTurn, QByteArray());
                sendToPlayer(room->playerBlack, MessageType::OpponentTurn, QByteArray());
            } else {
                sendToPlayer(room->playerBlack, MessageType::YourTurn, QByteArray());
                sendToPlayer(room->playerWhite, MessageType::OpponentTurn, QByteArray());
            }
        } else {
            // Передача хода
            if (room->game.isWhiteTurn()) {
                sendToPlayer(room->playerWhite, MessageType::YourTurn, QByteArray());
                sendToPlayer(room->playerBlack, MessageType::OpponentTurn, QByteArray());
            } else {
                sendToPlayer(room->playerBlack, MessageType::YourTurn, QByteArray());
                sendToPlayer(room->playerWhite, MessageType::OpponentTurn, QByteArray());
            }
        }
    } else {
        QByteArray errorData;
        QDataStream out(&errorData, QIODevice::WriteOnly);
        out << QString("Невозможно выполнить ход").toUtf8();
        sendToPlayer(socket, MessageType::Error, errorData);
    }
}

void Server::handleLeaveRoom(QTcpSocket *socket)
{
    Player *player = getPlayer(socket);
    if (!player || player->roomId == 0) return;

    Room *room = getRoom(player->roomId);
    if (!room) return;

    quint16 roomId = player->roomId;

    room->players.removeAll(player);
    player->roomId = 0;
    player->isSpectator = false;
    player->isWhite = false;

    QByteArray data;
    QDataStream out(&data, QIODevice::WriteOnly);
    out << player->name;
    broadcastToRoom(room, MessageType::PlayerLeft, data);

    if (room->players.isEmpty() || room->isGameActive) {
        if (room->isGameActive) {
            room->isGameActive = false;
            if (!room->players.isEmpty()) {
                for (Player *p : room->players) {
                    p->roomId = 0;
                    p->isSpectator = false;
                    p->isWhite = false;
                }
                QByteArray gameOverData;
                QDataStream goOut(&gameOverData, QIODevice::WriteOnly);
                goOut << QString("Игрок покинул игру");
                broadcastToRoom(room, MessageType::GameOver, gameOverData);
            }
        }
        m_rooms.remove(roomId);
        delete room;
    }

    sendRoomListToAll();
}

void Server::checkGameOver(Room *room)
{
    if (!room->game.isGameOver()) return;

    room->isGameActive = false;
    for (Player *p : room->players) {
        p->roomId = 0;
        p->isSpectator = false;
        p->isWhite = false;
    }

    QString winner = room->game.isWhiteTurn() ? "Чёрные" : "Белые";
    QByteArray data;
    QDataStream out(&data, QIODevice::WriteOnly);
    out << winner;
    broadcastToRoom(room, MessageType::GameOver, data);

    QTimer::singleShot(2000, this, [this, roomId = room->id]() {
        Room *room = getRoom(roomId);
        if (!room) return;
        m_rooms.remove(roomId);
        delete room;
        sendRoomListToAll();
    });
}

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
    return player ? getRoom(player->roomId) : nullptr;
}

void Server::handleChatMessage(QTcpSocket *socket, QDataStream &in)
{
    Player *player = getPlayer(socket);
    if (!player) return;

    QByteArray messageData;
    in >> messageData;
    QString message = QString::fromUtf8(messageData);

    QByteArray data;
    QDataStream out(&data, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_6_0);
    out << player->name.toUtf8();
    out << messageData;

    Room *room = getRoom(player->roomId);
    if (room) {
        broadcastToRoom(room, MessageType::ChatBroadcast, data);
    } else {
        for (QTcpSocket *client : m_clients)
            sendToPlayer(client, MessageType::ChatBroadcast, data);
    }
}

void Server::onTimerTick()
{
    for (Room *room : m_rooms) {
        if (!room->isGameActive) continue;

        if (room->game.isWhiteTurn())
            room->whiteTime--;
        else
            room->blackTime--;

        QByteArray data;
        QDataStream out(&data, QIODevice::WriteOnly);
        out << room->whiteTime << room->blackTime;
        broadcastToRoom(room, MessageType::TimerUpdate, data);

        if (room->whiteTime <= 0 || room->blackTime <= 0) {
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
    QByteArray data;
    QDataStream out(&data, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_6_0);

    out << static_cast<quint16>(m_rooms.size());
    for (auto it = m_rooms.begin(); it != m_rooms.end(); ++it) {
        Room *room = it.value();
        out << room->id;
        out << static_cast<quint16>(room->players.size());
    }

    for (QTcpSocket *socket : m_clients)
        sendToPlayer(socket, MessageType::RoomList, data);
}

void Server::scheduleRestart(Room *room)
{
    if (!room) return;
    QTimer::singleShot(2000, this, [this, room]() {
        if (!room || !m_rooms.contains(room->id) || room->isGameActive) return;
        if (room->players.size() >= 2)
            startGame(room);
    });
}