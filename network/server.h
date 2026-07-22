#ifndef SERVER_H
#define SERVER_H

#include <QTcpServer>
#include <QTcpSocket>
#include <QMap>
#include <QVector>
#include <QTimer>
#include "protocol.h"
#include "../game/gamelogic.h"

struct Player {
    QTcpSocket *socket;
    QString name;
    quint16 id;
    bool isReady = false;
    bool isSpectator = false;
    bool isWhite = false;
    quint16 roomId = 0;
};

struct Room {
    quint16 id;
    QString name;
    QVector<Player*> players;
    GameLogic game;
    bool isGameActive = false;
    QTcpSocket* playerWhite = nullptr;
    QTcpSocket* playerBlack = nullptr;
    QTimer *timer;
    int whiteTime = 300;
    int blackTime = 300;
};

class Server : public QTcpServer
{
    Q_OBJECT

public:
    explicit Server(QObject *parent = nullptr);
    ~Server();

    bool startServer(quint16 port = 5555);
    void stopServer();

signals:
    void newConnection();

protected:
    void incomingConnection(qintptr socketDescriptor) override;

private slots:
    void onClientDisconnected();
    void onReadyRead();
    void onTimerTick();

private:
    QTcpServer *m_server;
    QVector<QTcpSocket*> m_clients;
    QMap<QTcpSocket*, Player*> m_players;
    QMap<quint16, Room*> m_rooms;
    quint16 m_nextRoomId = 1;
    quint16 m_nextPlayerId = 1;
    QTimer *m_timer;

    Player* getPlayer(QTcpSocket *socket);
    Room* getRoom(quint16 roomId);
    Room* getRoomByPlayer(QTcpSocket *socket);

    void handleConnect(QTcpSocket *socket, QDataStream &in);
    void handleCreateRoom(QTcpSocket *socket);
    void handleJoinRoom(QTcpSocket *socket, QDataStream &in);
    void handleMakeMove(QTcpSocket *socket, QDataStream &in);
    void handleChatMessage(QTcpSocket *socket, QDataStream &in);

    void broadcastToRoom(Room *room, MessageType type, const QByteArray &data);
    void sendToPlayer(QTcpSocket *socket, MessageType type, const QByteArray &data);
    void sendGameState(Room *room);
    void startGame(Room *room);
    void checkGameOver(Room *room);
    void sendRoomListToAll();
};

#endif // SERVER_H