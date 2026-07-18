#ifndef AI_H
#define AI_H

#include <QVector>
#include <QPoint>
#include "gamelogic.h"



class CompInt
{
public:
    CompInt();

    // Получить лучший ход для текущей позиции
    GameLogic::Move getBestMove(const GameLogic &game, bool isWhite);

    // Оценка позиции (положительное значение - лучше для белых)
    int evaluatePosition(const int board[8][8]);

private:
    QVector<GameLogic::Move> getAllMoves(const GameLogic &game, bool isWhite);

    // Оценка одного хода
    int evaluateMove(const GameLogic &game, const GameLogic::Move &move, bool isWhite);

    // Простая эвристика
    int countPieces(const int board[8][8], bool white);
    int countKings(const int board[8][8], bool white);
    int evaluateBoard(const int board[8][8]);
};

#endif