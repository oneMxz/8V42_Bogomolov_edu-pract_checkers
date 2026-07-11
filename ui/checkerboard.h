#ifndef CHECKERBOARD_H
#define CHECKERBOARD_H

#include <QWidget>
#include <QTimer>
#include "../game/gamelogic.h"

class CheckerBoard : public QWidget
{
    Q_OBJECT

public:
    explicit CheckerBoard(QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

private:
    GameLogic m_logic;

    // Анимация
    QPoint animFrom;
    QPoint animTo;
    QVector<QPoint> animCaptured;
    float animProgress;
    QTimer *animTimer;
    bool isAnimating;

    QPoint hoverPos;

    void resetGame();
    void startAnimation(const GameLogic::Move &move);
    void updateBoard();

    void drawBoard(QPainter &p);
    void drawCheckers(QPainter &p);
    void drawSelection(QPainter &p);
    void drawMoves(QPainter &p);
    void drawHover(QPainter &p);
    void drawInfo(QPainter &p);
    void drawGameOver(QPainter &p);

    QPoint getCell(const QPoint &pos) const;

private slots:
    void animate();
};

#endif