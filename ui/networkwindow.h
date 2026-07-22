#ifndef NETWORKWINDOW_H
#define NETWORKWINDOW_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QListWidget>
#include <QTextEdit>
#include <QLineEdit>
#include <QLabel>
#include "../network/client.h"
#include "../network/server.h"

class NetworkWindow : public QWidget
{
    Q_OBJECT

public:
    explicit NetworkWindow(QWidget *parent = nullptr);
    void setClient(Client *client);
    void setPlayerName(const QString &name);
    void startServer();

signals:
    void joinRoomRequested(quint16 roomId);
    void createRoomRequested();
    void chatMessageSent(const QString &message);
    void backToMenuRequested();

private slots:
    void onRoomCreated(quint16 roomId);
    void onRoomList(const QVector<QPair<quint16, QString>> &rooms);
    void onRoomJoined(quint16 roomId, const QString &playerName);
    void onPlayerJoined(const QString &playerName);
    void onPlayerLeft(const QString &playerName);
    void onChatMessage(const QString &sender, const QString &message);
    void onGameStarted();
    void onConnectionError(const QString &message);

    void onCreateRoomClicked();
    void onJoinRoomClicked();
    void onSendMessageClicked();
    void onBackClicked();

private:
    Client *m_client = nullptr;
    QString m_playerName;
    quint16 m_currentRoomId = 0;

    // UI элементы
    QListWidget *m_roomList;
    QTextEdit *m_chatDisplay;
    QLineEdit *m_chatInput;
    QPushButton *m_createRoomButton;
    QPushButton *m_joinRoomButton;
    QPushButton *m_sendButton;
    QPushButton *m_backButton;
    QLabel *m_statusLabel;
    Server *m_server = nullptr;

    void setupUI();
    void connectSignals();
    void addChatMessage(const QString &sender, const QString &message);
    void updateRoomList(const QVector<QPair<quint16, QString>> &rooms);
};

#endif // NETWORKWINDOW_H