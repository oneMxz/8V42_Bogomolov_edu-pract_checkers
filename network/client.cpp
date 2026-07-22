#include "client.h"
#include <QDataStream>
#include <QDebug>

Client::Client(QObject *parent)
    : QObject(parent)
    , m_socket(new QTcpSocket(this))
    , m_roomId(0)
{
    connect(m_socket, &QTcpSocket::connected, this, &Client::onConnected);
    connect(m_socket, &QTcpSocket::disconnected, this, &Client::onDisconnected);
    connect(m_socket, &QTcpSocket::readyRead, this, &Client::onReadyRead);
    connect(m_socket, &QTcpSocket::errorOccurred, this, &Client::onError);
}

Client::~Client()
{
    disconnectFromServer();
}

bool Client::connectToServer(const QString &host, quint16 port)
{
    if (isConnected()) return true;
    m_socket->connectToHost(host, port);
    return m_socket->waitForConnected(3000);
}

void Client::disconnectFromServer()
{
    if (m_socket->state() == QTcpSocket::ConnectedState) {
        m_socket->disconnectFromHost();
    }
}

bool Client::isConnected() const
{
    return m_socket->state() == QTcpSocket::ConnectedState;
}

void Client::sendConnect(const QString &playerName)
{
    m_playerName = playerName;
    QByteArray data;
    QDataStream out(&data, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_6_0);

    // ===== ПЕРЕДАЁМ СТРОКУ КАК QByteArray =====
    QByteArray nameData = playerName.toUtf8();
    out << nameData;

    sendMessage(MessageType::Connect, data);
}

void Client::sendCreateRoom()
{
    qDebug() << "Sending CreateRoom message";
    sendMessage(MessageType::CreateRoom);
}

void Client::sendJoinRoom(quint16 roomId)
{
    m_roomId = roomId;
    QByteArray data;
    QDataStream out(&data, QIODevice::WriteOnly);
    out << roomId;
    sendMessage(MessageType::JoinRoom, data);
}

void Client::sendMakeMove(int fromRow, int fromCol, int toRow, int toCol)
{
    NetworkMove move;
    move.fromRow = fromRow;
    move.fromCol = fromCol;
    move.toRow = toRow;
    move.toCol = toCol;

    QByteArray data;
    QDataStream out(&data, QIODevice::WriteOnly);
    out << move;
    sendMessage(MessageType::MakeMove, data);
}

void Client::sendChatMessage(const QString &message)
{
    QByteArray data;
    QDataStream out(&data, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_6_0);
    QByteArray messageData = message.toUtf8();
    out << messageData;
    sendMessage(MessageType::ChatMessage, data);
}

void Client::sendMessage(MessageType type, const QByteArray &data)
{
    if (m_socket->state() != QTcpSocket::ConnectedState) return;

    QByteArray message;
    QDataStream out(&message, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_6_0);
    out << static_cast<quint8>(type);
    out << data;
    m_socket->write(message);
}

void Client::onConnected()
{
    qDebug() << "Connected to server";
    emit connected();
}

void Client::onDisconnected()
{
    qDebug() << "Disconnected from server";
    emit disconnected();
}

void Client::onReadyRead()
{
    QDataStream in(m_socket);
    in.setVersion(QDataStream::Qt_6_0);

    while (m_socket->bytesAvailable() >= sizeof(quint8)) {
        quint8 type;
        in >> type;
        processMessage(in, static_cast<MessageType>(type));
    }
}

void Client::processMessage(QDataStream &in, MessageType type)
{
    in.setVersion(QDataStream::Qt_6_0);
    switch (type) {
    case MessageType::ConnectionAccepted: {
        QByteArray messageData;
        in >> messageData;
        QString message = QString::fromUtf8(messageData);
        qDebug() << "Connection accepted:" << message;
        break;
    }
    case MessageType::RoomCreated: {
        quint16 roomId;
        in >> roomId;
        m_roomId = roomId;
        emit roomCreated(roomId);
        break;
    }
    case MessageType::RoomJoined: {
        quint16 roomId;
        QByteArray nameData;
        in >> roomId;
        in >> nameData;
        QString playerName = QString::fromUtf8(nameData);
        emit roomJoined(roomId, playerName);
        break;
    }
    case MessageType::RoomList: {
        quint16 roomCount;
        in >> roomCount;
        qDebug() << "  roomCount read:" << roomCount;

        QVector<QPair<quint16, QString>> rooms;
        for (int i = 0; i < roomCount; ++i) {
            quint16 roomId;
            quint16 playerCount;
            in >> roomId;
            in >> playerCount;
            rooms.append(qMakePair(roomId, QString::number(playerCount)));
        }

        qDebug() << "Received room list. Rooms count:" << rooms.size();
        emit roomList(rooms);
        break;
    }

    case MessageType::GameStarted: {
        emit gameStarted();
        break;
    }
    case MessageType::GameState: {
        BoardState state;
        in >> state;
        emit gameState(state);
        break;
    }
    case MessageType::YourTurn: {
        emit yourTurn();
        break;
    }
    case MessageType::OpponentTurn: {
        emit opponentTurn();
        break;
    }

    case MessageType::GameOver: {
        QByteArray winnerData;
        in >> winnerData;
        QString winner = QString::fromUtf8(winnerData);
        emit gameOver(winner);
        break;
    }

    case MessageType::ChatBroadcast: {
        QByteArray senderData, messageData;
        in >> senderData >> messageData;
        QString sender = QString::fromUtf8(senderData);
        QString message = QString::fromUtf8(messageData);
        emit chatMessage(sender, message);
        break;
    }
    case MessageType::PlayerJoined: {
        QByteArray nameData;
        in >> nameData;
        QString playerName = QString::fromUtf8(nameData);
        emit playerJoined(playerName);
        break;
    }
    case MessageType::PlayerLeft: {
        QByteArray nameData;
        in >> nameData;
        QString playerName = QString::fromUtf8(nameData);
        emit playerLeft(playerName);
        break;
    }
    case MessageType::ColorAssigned: {
        bool isWhite;
        in >> isWhite;
        emit colorAssigned(isWhite);
        break;
    }
    case MessageType::TimerUpdate: {
        int whiteTime, blackTime;
        in >> whiteTime >> blackTime;
        emit timerUpdate(whiteTime, blackTime);
        break;
    }
    default:
        qDebug() << "Unknown message type:" << static_cast<int>(type);
        break;
    }
}

void Client::onError(QAbstractSocket::SocketError socketError)
{
    emit error(m_socket->errorString());
}