#ifndef GAMECOMP_H
#define GAMECOMP_H

#include <QVector>
#include <QPoint>
#include "gamelogic.h"

class GameAI
{
public:
    GameAI();

    // Получить лучший ход для текущей позиции
    GameLogic::Move getBestMove(const GameLogic &game, bool isWhite, int depth = 2);

    // Оценка позиции
    int evaluatePosition(const int board[8][8]);
    QVector<GameLogic::Move> getAllMoves(const GameLogic &game, bool isWhite);

private:
    // Минимакс с ограниченной глубиной
    int minimax(GameLogic &game, int depth, bool isMaximizing, bool aiIsWhite);

    // Оценка доски
    int evaluateBoard(const int board[8][8]);
    int countPieces(const int board[8][8], bool white);
    int countKings(const int board[8][8], bool white);

    // Бонус за позицию (центр, продвинутость)
    int positionBonus(const int board[8][8], bool white);

    // Вспомогательные методы
    bool isValid(int r, int c) const;
    bool isWhitePiece(int type) const;
    bool isBlackPiece(int type) const;
    bool isKing(int type) const;
};

#endif // GAMECOMP_H