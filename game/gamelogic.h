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
        bool becameKing = false;

        Move() {}
        Move(const QPoint &f, const QPoint &t) : from(f), to(t) {}
        bool isCapture() const { return !captured.isEmpty(); }
        bool isValid() const { return from != to; }
    };

    GameLogic();
    void initBoard();
    void reset();

    const int (&getBoard() const)[8][8] { return board; }
    int getCell(int row, int col) const { return board[row][col]; }
    void setCell(int row, int col, int value) { board[row][col] = value; }

    bool isWhiteTurn() const { return m_isWhiteTurn; }
    void setWhiteTurn(bool turn) { m_isWhiteTurn = turn; }

    bool isGameOver() const { return m_gameOver; }
    void setGameOver(bool over) { m_gameOver = over; }

    int getWhiteCaptured() const { return m_whiteCaptured; }
    int getBlackCaptured() const { return m_blackCaptured; }
    void setWhiteCaptured(int count) { m_whiteCaptured = count; }
    void setBlackCaptured(int count) { m_blackCaptured = count; }

    int countPieces(bool white) const;

    bool isValid(int r, int c) const;
    bool isBlack(int r, int c) const;
    bool isWhite(int type) const;
    bool isBlackPiece(int type) const;
    bool isKing(int type) const;
    bool isMyPiece(int type) const;

    QVector<Move> generateMoves(int row, int col) const;
    QVector<Move> generateMovesForPiece(int row, int col) const { return generateMoves(row, col); }

    //Выполнение хода
    bool makeMove(const Move &move);
    QVector<Move> getAvailableMoves() const { return m_availableMoves; }
    void setAvailableMoves(const QVector<Move> &moves) { m_availableMoves = moves; }

    void setSelected(const QPoint &pos) { m_selected = pos; }
    QPoint getSelected() const { return m_selected; }

    //Проверки наличия ходов
    bool hasMoves(bool white) const;
    bool hasCaptures(bool white) const;

    //Таймер
    void startTimer(int secondsPerPlayer);
    void stopTimer();
    void updateTimer();
    bool isTimerExpired() const;
    int getWhiteTime() const { return m_whiteTime; }
    int getBlackTime() const { return m_blackTime; }
    void setWhiteTime(int time) { m_whiteTime = time; }
    void setBlackTime(int time) { m_blackTime = time; }

private:
    void getSimpleMoves(int row, int col, QVector<Move> &moves) const;
    void getCaptures(int row, int col, QVector<Move> &moves) const;
    void getKingMoves(int row, int col, QVector<Move> &moves) const;
    void getKingCaptures(int row, int col, QVector<Move> &moves) const;
    void addCapturedPieces(const Move &move, QVector<QPoint> &captured) const;

    void applyMove(const Move &move);
    void makeKing(int row, int col);

    int board[8][8];
    bool m_isWhiteTurn;
    bool m_gameOver;
    int m_whiteCaptured;   // белых побито (чёрными)
    int m_blackCaptured;   // чёрных побито (белыми)
    QPoint m_selected;
    QVector<Move> m_availableMoves;

    int m_whiteTime = 0;
    int m_blackTime = 0;
    bool m_timerEnabled = false;
};

#endif