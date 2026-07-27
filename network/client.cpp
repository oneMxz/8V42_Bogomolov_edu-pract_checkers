#include "client.h"
#include <QDataStream>

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
    if (m_socket->state() == QTcpSocket::ConnectedState)
        m_socket->disconnectFromHost();
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
    QByteArray nameData = playerName.toUtf8();
    out << nameData;
    sendMessage(MessageType::Connect, data);
}

void Client::sendCreateRoom()
{
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

void Client::sendLeaveRoom()
{
    sendMessage(MessageType::LeaveRoom);
}

void Client::sendMessage(MessageType type, const QByteArray &data)
{
    if (m_socket->state() != QTcpSocket::ConnectedState) return;
    SocketUtils::sendMessage(m_socket, static_cast<quint8>(type), data);
}

void Client::onConnected()
{
    emit connected();
}

void Client::onDisconnected()
{
    emit disconnected();
}

void Client::onReadyRead()
{
    quint8 type;
    QByteArray data;
    while (SocketUtils::readMessage(m_socket, type, data))
        processMessage(data, static_cast<MessageType>(type));
}

void Client::processMessage(const QByteArray &data, MessageType type)
{
    QDataStream in(data);
    in.setVersion(QDataStream::Qt_6_0);

    switch (type) {
    case MessageType::ConnectionAccepted: {
        QByteArray msgData;
        in >> msgData;
        // сообщение игнорируется
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
        emit roomJoined(roomId, QString::fromUtf8(nameData));
        break;
    }
    case MessageType::RoomList: {
        quint16 roomCount;
        in >> roomCount;
        QVector<QPair<quint16, QString>> rooms;
        for (int i = 0; i < roomCount; ++i) {
            quint16 roomId, playerCount;
            in >> roomId;
            in >> playerCount;
            rooms.append(qMakePair(roomId, QString::number(playerCount)));
        }
        emit roomList(rooms);
        break;
    }
    case MessageType::GameStarted:
        emit gameStarted();
        break;
    case MessageType::GameState: {
        BoardState state;
        in >> state;
        emit gameState(state);
        break;
    }
    case MessageType::YourTurn:
        emit yourTurn();
        break;
    case MessageType::OpponentTurn:
        emit opponentTurn();
        break;
    case MessageType::GameOver: {
        QByteArray winnerData;
        in >> winnerData;
        emit gameOver(QString::fromUtf8(winnerData));
        break;
    }
    case MessageType::ChatBroadcast: {
        QByteArray senderData, messageData;
        in >> senderData >> messageData;
        emit chatMessage(QString::fromUtf8(senderData), QString::fromUtf8(messageData));
        break;
    }
    case MessageType::PlayerJoined: {
        QByteArray nameData;
        in >> nameData;
        emit playerJoined(QString::fromUtf8(nameData));
        break;
    }
    case MessageType::PlayerLeft: {
        QByteArray nameData;
        in >> nameData;
        emit playerLeft(QString::fromUtf8(nameData));
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
    case MessageType::RoomFull:
        emit error("Комната заполнена");
        break;
    case MessageType::ConnectionRejected: {
        QByteArray msgData;
        in >> msgData;
        emit error("Подключение отклонено: " + QString::fromUtf8(msgData));
        break;
    }
    case MessageType::Error: {
        QByteArray msgData;
        in >> msgData;
        emit error(QString::fromUtf8(msgData));
        break;
    }
    default:
        break;
    }
}

void Client::onError(QAbstractSocket::SocketError socketError)
{
    emit error(m_socket->errorString());
}