#ifndef CHECKERBOARD_H
#define CHECKERBOARD_H

#include <QWidget>
#include <QTimer>
#include <QPushButton>
#include <QMessageBox>
#include <QTextEdit>
#include <QLineEdit>

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
    void setNetworkMode(bool enabled);
    void setPlayerName(const QString &name);
    void setClient(Client *client);
    bool isNetworkMode() const { return m_networkMode; }

signals:
    void exitToMenuRequested();
    void exitToLobbyRequested();
    void chatMessageSent(const QString &message);
    void moveToNetwork(int fromRow, int fromCol, int toRow, int toCol);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    // Размер доски в пикселях (константа)
    static constexpr int BOARD_SIZE = 600;

    //Вспомогательные методы отрисовки
    void drawBoard(QPainter &p);
    void drawCheckers(QPainter &p);
    void drawSelection(QPainter &p);
    void drawMoves(QPainter &p);
    void drawHover(QPainter &p);
    void drawInfo(QPainter &p);
    void drawGameOver(QPainter &p);

    //Анимация
    void startAnimation(const GameLogic::Move &move);

    //Ходы и история
    void recordMove(const GameLogic::Move &move);
    void saveGameHistory();
    void makeAIMove();

    //Вспомогательные
    QPoint getCell(const QPoint &pos) const;
    void addChatMessage(const QString &sender, const QString &message);
    void addSystemMessage(const QString &message);
    void updateBoard();   // Просто вызывает update()

    // Игровая логика
    GameLogic m_logic;
    bool m_aiMode = false;
    bool m_aiIsWhite = false;
    GameAI m_gameAI;
    QPoint hoverPos;

    // Анимация
    QPoint animFrom;
    QPoint animTo;
    QVector<QPoint> animCaptured;
    float animProgress = 0.0f;
    QTimer *animTimer = nullptr;
    bool isAnimating = false;
    QPushButton *exitButton = nullptr;
    QPushButton *saveButton = nullptr;
    QTextEdit *m_chatDisplay = nullptr;
    QLineEdit *m_chatInput = nullptr;
    QPushButton *m_sendButton = nullptr;

    // История ходов
    QStringList m_moveHistory;
    int m_moveCounter = 0;

    // Таймер для локального режима
    QTimer *m_timer = nullptr;

    // Сетевые переменные
    Client *m_client = nullptr;
    bool m_networkMode = false;
    bool m_isMyTurn = false;
    bool m_myColorIsWhite = false;
    QString m_playerName;

private slots:
    void animate();
    void onExitButtonClicked();
    void onSaveButtonClicked();
    void onSendMessageClicked();

    // Таймер
    void onTimerTick();

    // Сетевые слоты
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

#endif // CHECKERBOARD_H