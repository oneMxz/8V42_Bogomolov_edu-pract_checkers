#ifndef CHECKERBOARD_H
#define CHECKERBOARD_H

#include <QWidget>
#include <QTimer>
#include <QPushButton>
#include <QMessageBox>
#include <QTextEdit>
#include <QLineEdit>
#include <QTimer>

#include "../game/gamelogic.h"
#include "../game/gamecomp.h"
#include "../network/client.h"
#include "../network/protocol.h"

class CheckerBoard : public QWidget
{
    Q_OBJECT

public:
    explicit CheckerBoard(QWidget *parent = nullptr);
    void resetGame();
    void setAIMode(bool enabled, bool aiIsWhite = false);
    void setNetworkMode(bool enabled, const QString &host = "", quint16 port = 5555);
    void setPlayerName(const QString &name);
    bool isNetworkMode() const { return m_networkMode; }

signals:
    void exitToMenuRequested();
    void chatMessageSent(const QString &message);
    void moveToNetwork(int fromRow, int fromCol, int toRow, int toCol);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    GameLogic m_logic;

    // Анимация
    QPoint animFrom;
    QPoint animTo;
    QVector<QPoint> animCaptured;
    float animProgress;
    QTimer *animTimer;
    bool isAnimating;

    bool m_aiMode = false;
    bool m_aiIsWhite = false;
    GameAI m_gameAI;
    QPoint hoverPos;
    QPushButton *exitButton;
    QPushButton *saveButton;

    void startAnimation(const GameLogic::Move &move);
    void updateBoard();

    QStringList m_moveHistory;   // история ходов
    void saveGameHistory();    // сохранить в файл

    void recordMove(const GameLogic::Move &move);
    int m_moveCounter = 0;

    void drawBoard(QPainter &p);
    void drawCheckers(QPainter &p);
    void drawSelection(QPainter &p);
    void drawMoves(QPainter &p);
    void drawHover(QPainter &p);
    void drawInfo(QPainter &p);
    void drawGameOver(QPainter &p);
    QTimer *m_timer;          // тикает каждую секунду

    QPoint getCell(const QPoint &pos) const;
    void makeAIMove();

    Client *m_client = nullptr;
    bool m_networkMode = false;
    bool m_isMyTurn = false;
    bool m_myColorIsWhite = false;
    QString m_playerName;

    // Чат
    QTextEdit *m_chatDisplay;
    QLineEdit *m_chatInput;
    QPushButton *m_sendButton;

    void addChatMessage(const QString &sender, const QString &message);
    void addSystemMessage(const QString &message);

private slots:
    void animate();
    void onExitButtonClicked();
    void onSaveButtonClicked();
    void onSendMessageClicked();

    void onTimerTick();
    // ===== СЕТЕВЫЕ СЛОТЫ (ДОБАВИТЬ ВСЕ) =====
    void onConnected();
    void onDisconnected();
    void onNetworkError(const QString &message);
    void onRoomCreated(quint16 roomId);
    void onRoomJoined(quint16 roomId, const QString &playerName);
    void onPlayerJoined(const QString &playerName);
    void onPlayerLeft(const QString &playerName);
    void onGameStarted();
    void onGameState(const BoardState &state);
    void onYourTurn();
    void onOpponentTurn();
    void onGameOver(const QString &winner);
    void onChatMessage(const QString &sender, const QString &message);
    void onColorAssigned(bool isWhite);
    void onTimerUpdate(int whiteTime, int blackTime);
};

#endif