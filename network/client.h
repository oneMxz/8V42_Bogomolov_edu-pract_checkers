#ifndef CLIENT_H
#define CLIENT_H

#include <QTcpSocket>
#include <QObject>
#include "protocol.h"

class Client : public QObject
{
    Q_OBJECT

public:
    explicit Client(QObject *parent = nullptr);
    ~Client();

    bool connectToServer(const QString &host, quint16 port); //Подключение к серверу по указанному адресу и порту
    void disconnectFromServer();
    bool isConnected() const;

    // Отправка сообщений
    void sendConnect(const QString &playerName);
    void sendCreateRoom();
    void sendJoinRoom(quint16 roomId);
    void sendLeaveRoom();
    void sendMakeMove(int fromRow, int fromCol, int toRow, int toCol);
    void sendChatMessage(const QString &message);
    void setPlayerName(const QString &name) { m_playerName = name; }

    QString getPlayerName() const { return m_playerName; }

signals:
    void connected();
    void disconnected();
    void error(const QString &message);

    void roomList(const QVector<QPair<quint16, QString>> &rooms);
    void roomCreated(quint16 roomId);
    void roomJoined(quint16 roomId, const QString &playerName);

    void gameStarted();
    void gameState(const BoardState &state);
    void gameOver(const QString &winner);

    void yourTurn();
    void opponentTurn();
    void chatMessage(const QString &sender, const QString &message);
    void playerJoined(const QString &playerName);
    void playerLeft(const QString &playerName);
    void colorAssigned(bool isWhite);
    void timerUpdate(int whiteTime, int blackTime);

private slots:
    void onReadyRead();
    void onConnected();
    void onDisconnected();
    void onError(QAbstractSocket::SocketError socketError);

private:
    void sendMessage(MessageType type, const QByteArray &data = QByteArray());
    void processMessage(const QByteArray &data, MessageType type);

    QTcpSocket *m_socket;
    quint16 m_roomId;
    QString m_playerName;
};

#endif // CLIENT_H