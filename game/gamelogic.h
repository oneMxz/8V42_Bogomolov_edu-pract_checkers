#ifndef GAMELOGIC_H
#define GAMELOGIC_H

#include <QVector>
#include <QPoint>

class GameLogic
{
public:
    enum Cell {
        Empty = 0,
        White = 1,
        Black = 2,
        WhiteKing = 3,
        BlackKing = 4
    };

    struct Move {
        QPoint from;
        QPoint to;
        QVector<QPoint> captured;
        bool becameKing =  false;

        Move() {}
        Move(const QPoint &f, const QPoint &t) : from(f), to(t) {}
        bool isCapture() const { return !captured.isEmpty(); }
    };

    GameLogic();

    // Инициализация
    void initBoard();
    void reset();

    // Доступ к доске
    const int (&getBoard() const)[8][8] { return board; }
    int getCell(int row, int col) const { return board[row][col]; }

    // Состояние игры
    bool isWhiteTurn() const { return m_isWhiteTurn; }
    bool isGameOver() const { return m_gameOver; }
    int getWhiteCaptured() const { return m_whiteCaptured; }
    int getBlackCaptured() const { return m_blackCaptured; }
    int countPieces(bool white) const;

    // Вспомогательные методы
    bool isValid(int r, int c) const;
    bool isBlack(int r, int c) const;
    bool isWhite(int type) const;
    bool isBlackPiece(int type) const;
    bool isKing(int type) const;
    bool isMyPiece(int type) const;


    // Генерация ходов
    QVector<Move> generateMoves(int row, int col) const;
    QVector<Move> generateMovesForPiece(int row, int col) const;

    // Выполнение хода
    bool makeMove(const Move &move);
    QVector<Move> getAvailableMoves() const { return m_availableMoves; }
    void setSelected(const QPoint &pos) { m_selected = pos; }
    QPoint getSelected() const { return m_selected; }

    // Проверки
    bool hasMoves(bool white) const;
    bool hasCaptures(bool white) const;

    void setAvailableMoves(const QVector<Move> &moves) {m_availableMoves = moves;}

private:
    int board[8][8];
    bool m_isWhiteTurn;
    bool m_gameOver;
    int m_whiteCaptured; // Черные побили белых
    int m_blackCaptured; // Белые побили черных
    QPoint m_selected;
    QVector<Move> m_availableMoves;

    // Генерация ходов
    void getSimpleMoves(int row, int col, QVector<Move> &moves) const;
    void getCaptures(int row, int col, QVector<Move> &moves) const;
    void getKingMoves(int row, int col, QVector<Move> &moves) const;
    void getKingCaptures(int row, int col, QVector<Move> &moves) const;
    void addCapturedPieces(const Move &move, QVector<QPoint> &captured) const;

    // Внутреннее выполнение хода
    void applyMove(const Move &move);
    void makeKing(int row, int col);
};

#endif // GAMELOGIC_H