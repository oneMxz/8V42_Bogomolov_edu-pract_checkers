#include "gamelogic.h"
#include <cstring>

GameLogic::GameLogic()
    : m_isWhiteTurn(true)
    , m_gameOver(false)
    , m_whiteCaptured(0)
    , m_blackCaptured(0)
    , m_selected(-1, -1)
{
    initBoard();
}

void GameLogic::initBoard()
{
    memset(board, 0, sizeof(board));

    for (int r = 0; r < 3; r++) {
        for (int c = 0; c < 8; c++) {
            if ((r + c) % 2 == 1) board[r][c] = Black;
        }
    }

    for (int r = 5; r < 8; r++) {
        for (int c = 0; c < 8; c++) {
            if ((r + c) % 2 == 1) board[r][c] = White;
        }
    }

    m_whiteCaptured = 0;
    m_blackCaptured = 0;
    m_selected = QPoint(-1, -1);
    m_availableMoves.clear();
    m_isWhiteTurn = true;
    m_gameOver = false;
}

void GameLogic::reset()
{
    initBoard();
}

// ============================================================
// ГЕНЕРАЦИЯ ХОДОВ
// ============================================================

QVector<GameLogic::Move> GameLogic::generateMoves(int row, int col) const
{
    QVector<Move> moves;

    int piece = board[row][col];
    if (piece == Empty) return moves;
    if (!isMyPiece(piece)) return moves;

    if (isKing(piece)) {
        getKingMoves(row, col, moves);
        getKingCaptures(row, col, moves);
    } else {
        getSimpleMoves(row, col, moves);
        getCaptures(row, col, moves);
    }

    return moves;
}

QVector<GameLogic::Move> GameLogic::generateMovesForPiece(int row, int col) const
{
    return generateMoves(row, col);
}

void GameLogic::getSimpleMoves(int row, int col, QVector<Move> &moves) const
{
    int dir = isWhite(board[row][col]) ? -1 : 1;

    for (int dc : {-1, 1}) {
        int nr = row + dir;
        int nc = col + dc;
        if (isValid(nr, nc) && board[nr][nc] == Empty) {
            moves.append(Move(QPoint(col, row), QPoint(nc, nr)));
        }
    }
}

void GameLogic::getCaptures(int row, int col, QVector<Move> &moves) const
{
    int piece = board[row][col];
    bool white = isWhite(piece);

    for (int dr : {-1, 1}) {
        for (int dc : {-1, 1}) {
            int mr = row + dr;
            int mc = col + dc;
            int nr = row + 2 * dr;
            int nc = col + 2 * dc;

            if (!isValid(nr, nc)) continue;

            int enemy = board[mr][mc];
            if (enemy == Empty) continue;

            bool isEnemy = (white && isBlackPiece(enemy)) || (!white && isWhite(enemy));
            if (!isEnemy) continue;

            if (board[nr][nc] != Empty) continue;

            Move move(QPoint(col, row), QPoint(nc, nr));
            move.captured.append(QPoint(mc, mr));

            // Проверяем цепочку
            int temp[8][8];
            memcpy(temp, board, sizeof(board));

            const_cast<GameLogic*>(this)->board[mr][mc] = Empty;
            const_cast<GameLogic*>(this)->board[nr][nc] = piece;
            const_cast<GameLogic*>(this)->board[row][col] = Empty;

            QVector<Move> next;
            const_cast<GameLogic*>(this)->getCaptures(nr, nc, next);

            if (!next.isEmpty()) {
                for (const Move &nm : next) {
                    if (!nm.captured.isEmpty()) {
                        Move full = move;
                        full.captured.append(nm.captured);
                        full.to = nm.to;
                        moves.append(full);
                    }
                }
            } else {
                moves.append(move);
            }

            memcpy(const_cast<GameLogic*>(this)->board, temp, sizeof(board));
        }
    }
}

void GameLogic::getKingMoves(int row, int col, QVector<Move> &moves) const
{
    for (int dr : {-1, 1}) {
        for (int dc : {-1, 1}) {
            int r = row + dr;
            int c = col + dc;
            while (isValid(r, c) && board[r][c] == Empty) {
                moves.append(Move(QPoint(col, row), QPoint(c, r)));
                r += dr;
                c += dc;
            }
        }
    }
}

void GameLogic::getKingCaptures(int row, int col, QVector<Move> &moves) const
{
    int piece = board[row][col];
    bool white = isWhite(piece);

    for (int dr : {-1, 1}) {
        for (int dc : {-1, 1}) {
            // Собираем всех врагов на этом направлении
            QVector<QPoint> enemies;
            QVector<QPoint> emptyCells;

            int r = row + dr;
            int c = col + dc;

            // Проходим по всему направлению
            while (isValid(r, c)) {
                if (board[r][c] != Empty) {
                    int enemy = board[r][c];
                    bool isEnemy = (white && isBlackPiece(enemy)) || (!white && isWhite(enemy));
                    if (isEnemy) {
                        enemies.append(QPoint(c, r));
                    } else {
                        break; // Своя шашка - стоп
                    }
                } else {
                    emptyCells.append(QPoint(c, r));
                }
                r += dr;
                c += dc;
            }

            // Если есть враги и пустые клетки за ними
            if (!enemies.isEmpty() && !emptyCells.isEmpty()) {
                for (const QPoint &empty : emptyCells) {
                    QVector<QPoint> capturedThisMove;
                    bool hasEnemyBefore = false;

                    // Проверяем все клетки между row,col и empty
                    int checkR = row + dr;
                    int checkC = col + dc;
                    while (checkR != empty.y() || checkC != empty.x()) {
                        if (isValid(checkR, checkC) && board[checkR][checkC] != Empty) {
                            int enemy = board[checkR][checkC];
                            bool isEnemy = (white && isBlackPiece(enemy)) || (!white && isWhite(enemy));
                            if (isEnemy) {
                                capturedThisMove.append(QPoint(checkC, checkR));
                                hasEnemyBefore = true;
                            } else {
                                hasEnemyBefore = false;
                                break;
                            }
                        }
                        checkR += dr;
                        checkC += dc;
                    }

                    if (!hasEnemyBefore || capturedThisMove.isEmpty()) continue;

                    Move move(QPoint(col, row), QPoint(empty.x(), empty.y()));
                    move.captured = capturedThisMove;

                    // Проверяем цепочку
                    int temp[8][8];
                    memcpy(temp, board, sizeof(board));

                    for (const QPoint &p : move.captured) {
                        const_cast<GameLogic*>(this)->board[p.y()][p.x()] = Empty;
                    }
                    const_cast<GameLogic*>(this)->board[empty.y()][empty.x()] = piece;
                    const_cast<GameLogic*>(this)->board[row][col] = Empty;

                    QVector<Move> next;
                    const_cast<GameLogic*>(this)->getKingCaptures(empty.y(), empty.x(), next);

                    if (!next.isEmpty()) {
                        for (const Move &nm : next) {
                            if (!nm.captured.isEmpty()) {
                                Move full = move;
                                full.captured.append(nm.captured);
                                full.to = nm.to;
                                moves.append(full);
                            }
                        }
                    } else {
                        moves.append(move);
                    }

                    memcpy(const_cast<GameLogic*>(this)->board, temp, sizeof(board));
                }
            }
        }
    }
}

// ============================================================
// ВЫПОЛНЕНИЕ ХОДА
// ============================================================

bool GameLogic::makeMove(const Move &move)
{
    if (m_gameOver) return false;

    // Проверяем, что ход есть в доступных
    bool found = false;
    for (const Move &m : m_availableMoves) {
        if (m.from == move.from && m.to == move.to) {
            found = true;
            break;
        }
    }
    if (!found) return false;

    applyMove(move);

    // Проверяем продолжение рубки
    bool hasMore = false;
    if (!move.captured.isEmpty()) {
        int tr = move.to.y();
        int tc = move.to.x();
        QVector<Move> next;
        int newPiece = board[tr][tc];
        if (isKing(newPiece)) {
            getKingCaptures(tr, tc, next);
        } else {
            getCaptures(tr, tc, next);
        }
        hasMore = !next.isEmpty();
    }

    if (!hasMore) {
        m_isWhiteTurn = !m_isWhiteTurn;
        m_selected = QPoint(-1, -1);
        m_availableMoves.clear();

        if (!hasMoves(m_isWhiteTurn) || countPieces(m_isWhiteTurn) == 0) {
            m_gameOver = true;
        }
    } else {
        // Продолжаем рубку
        m_selected = move.to;
        m_availableMoves = generateMoves(m_selected.y(), m_selected.x());
    }

    return true;
}

void GameLogic::applyMove(const Move &move)
{
    int fr = move.from.y();
    int fc = move.from.x();
    int tr = move.to.y();
    int tc = move.to.x();
    int piece = board[fr][fc];

    // Убираем побитые
    for (const QPoint &p : move.captured) {
        int captured = board[p.y()][p.x()];
        if (isWhite(piece) && isBlackPiece(captured)) {
            m_blackCaptured++;
        } else if (isBlackPiece(piece) && isWhite(captured)) {
            m_whiteCaptured++;
        }
        board[p.y()][p.x()] = Empty;
    }

    // Перемещаем
    board[tr][tc] = piece;
    board[fr][fc] = Empty;
    makeKing(tr, tc);
}

void GameLogic::makeKing(int row, int col)
{
    int piece = board[row][col];
    if (piece == White && row == 0) board[row][col] = WhiteKing;
    if (piece == Black && row == 7) board[row][col] = BlackKing;
}

// ============================================================
// ПРОВЕРКИ
// ============================================================

bool GameLogic::hasMoves(bool white) const
{
    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) {
            int p = board[r][c];
            if ((white && isWhite(p)) || (!white && isBlackPiece(p))) {
                QVector<Move> moves = generateMoves(r, c);
                if (!moves.isEmpty()) return true;
            }
        }
    }
    return false;
}

bool GameLogic::hasCaptures(bool white) const
{
    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) {
            int p = board[r][c];
            if ((white && isWhite(p)) || (!white && isBlackPiece(p))) {
                QVector<Move> moves;
                if (isKing(p)) {
                    getKingCaptures(r, c, moves);
                } else {
                    getCaptures(r, c, moves);
                }
                if (!moves.isEmpty()) return true;
            }
        }
    }
    return false;
}

int GameLogic::countPieces(bool white) const
{
    int count = 0;
    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) {
            int p = board[r][c];
            if ((white && isWhite(p)) || (!white && isBlackPiece(p))) {
                count++;
            }
        }
    }
    return count;
}

// ============================================================
// ВСПОМОГАТЕЛЬНЫЕ
// ============================================================

bool GameLogic::isValid(int r, int c) const
{
    return r >= 0 && r < 8 && c >= 0 && c < 8;
}

bool GameLogic::isBlack(int r, int c) const
{
    return (r + c) % 2 == 1;
}

bool GameLogic::isWhite(int type) const
{
    return type == White || type == WhiteKing;
}

bool GameLogic::isBlackPiece(int type) const
{
    return type == Black || type == BlackKing;
}

bool GameLogic::isKing(int type) const
{
    return type == WhiteKing || type == BlackKing;
}

bool GameLogic::isMyPiece(int type) const
{
    return (m_isWhiteTurn && isWhite(type)) || (!m_isWhiteTurn && isBlackPiece(type));
}