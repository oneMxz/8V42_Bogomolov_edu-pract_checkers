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
    QTcpSocket *socket;     //Сокет клиента
    QString name;           //Имя игрока
    quint16 id;             //Уникальный идентификатор
    bool isReady = false;   //Готов ли игрок (не используется)
    bool isSpectator = false; //Является ли зрителем
    bool isWhite = false;   //Играет ли белыми
    quint16 roomId = 0;   //ID комнаты, в которой находится
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
    //Обработчики входящих сообщений
    void handleConnect(QTcpSocket *socket, QDataStream &in);
    void handleCreateRoom(QTcpSocket *socket);
    void handleJoinRoom(QTcpSocket *socket, QDataStream &in);
    void handleMakeMove(QTcpSocket *socket, QDataStream &in);
    void handleChatMessage(QTcpSocket *socket, QDataStream &in);
    void handleLeaveRoom(QTcpSocket *socket);

    void broadcastToRoom(Room *room, MessageType type, const QByteArray &data);
    void sendToPlayer(QTcpSocket *socket, MessageType type, const QByteArray &data);
    void sendGameState(Room *room);
    void sendRoomListToAll();

    void startGame(Room *room);
    void scheduleRestart(Room *room);
    void checkGameOver(Room *room);


    QTcpServer *m_server;
    QVector<QTcpSocket*> m_clients;
    QMap<QTcpSocket*, Player*> m_players;
    QMap<quint16, Room*> m_rooms;
    quint16 m_nextRoomId = 1;
    quint16 m_nextPlayerId = 1;
    QTimer *m_timer;

    //Вспомогательные методы для поиска
    Player* getPlayer(QTcpSocket *socket);
    Room* getRoom(quint16 roomId);
    Room* getRoomByPlayer(QTcpSocket *socket);
};

#endif // SERVER_H