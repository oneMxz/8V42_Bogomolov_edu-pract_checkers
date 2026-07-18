#include "gamelogic.h"
#include <cstring>
#include <QDebug>

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

            // ===== ПРОВЕРЯЕМ, СТАНЕТ ЛИ ШАШКА ДАМКОЙ =====
            bool willBecomeKing = false;
            if (piece == White && nr == 0) {
                willBecomeKing = true;
                move.becameKing = true;
            } else if (piece == Black && nr == 7) {
                willBecomeKing = true;
                move.becameKing = true;
            }

            // ===== ВРЕМЕННО ПРИМЕНЯЕМ ХОД (через const_cast) =====
            int temp[8][8];
            memcpy(temp, board, sizeof(board));

            GameLogic* nonConst = const_cast<GameLogic*>(this);
            nonConst->board[mr][mc] = Empty;
            nonConst->board[nr][nc] = willBecomeKing ? (white ? WhiteKing : BlackKing) : piece;
            nonConst->board[row][col] = Empty;

            // ===== ИЩЕМ ПРОДОЛЖЕНИЕ =====
            QVector<Move> next;
            if (willBecomeKing) {
                nonConst->getKingCaptures(nr, nc, next);
                qDebug() << "  became king, checking continuation as KING, found:" << next.size();
            } else {
                nonConst->getCaptures(nr, nc, next);
            }

            // ===== СОБИРАЕМ РЕЗУЛЬТАТЫ =====
            if (!next.isEmpty()) {
                for (const Move &nm : next) {
                    if (!nm.captured.isEmpty()) {
                        Move full = move;
                        full.captured.append(nm.captured);
                        full.to = nm.to;
                        full.becameKing = move.becameKing || nm.becameKing;
                        moves.append(full);
                    }
                }
            } else {
                moves.append(move);
            }

            // ===== ВОССТАНАВЛИВАЕМ ДОСКУ =====
            memcpy(nonConst->board, temp, sizeof(board));
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
            // Ищем все возможные позиции для рубки на этом направлении
            int r = row + dr;
            int c = col + dc;

            // Собираем всех врагов на пути
            QVector<QPoint> enemiesOnPath;

            while (isValid(r, c)) {
                if (board[r][c] != Empty) {
                    int enemy = board[r][c];
                    bool isEnemy = (white && isBlackPiece(enemy)) || (!white && isWhite(enemy));
                    if (isEnemy) {
                        enemiesOnPath.append(QPoint(c, r));
                    } else {
                        // Своя шашка - дальше не идем
                        break;
                    }
                }
                r += dr;
                c += dc;
            }

            // Если нашли врагов, проверяем куда можно встать
            if (!enemiesOnPath.isEmpty()) {
                // Идем от последнего врага дальше, ищем пустые клетки
                QPoint lastEnemy = enemiesOnPath.last();
                int startR = lastEnemy.y() + dr;
                int startC = lastEnemy.x() + dc;

                int checkR = startR;
                int checkC = startC;

                while (isValid(checkR, checkC) && board[checkR][checkC] == Empty) {
                    // Нашли пустую клетку - это потенциальный ход
                    QVector<QPoint> capturedThisMove;

                    // Собираем всех врагов между row,col и checkR,checkC
                    int tempR = row + dr;
                    int tempC = col + dc;
                    while (tempR != checkR || tempC != checkC) {
                        if (isValid(tempR, tempC) && board[tempR][tempC] != Empty) {
                            int enemy = board[tempR][tempC];
                            bool isEnemy = (white && isBlackPiece(enemy)) || (!white && isWhite(enemy));
                            if (isEnemy) {
                                capturedThisMove.append(QPoint(tempC, tempR));
                            }
                        }
                        tempR += dr;
                        tempC += dc;
                    }

                    if (!capturedThisMove.isEmpty()) {
                        Move move(QPoint(col, row), QPoint(checkC, checkR));
                        move.captured = capturedThisMove;

                        // Проверяем цепочку
                        int tempBoard[8][8];
                        memcpy(tempBoard, board, sizeof(board));

                        // Убираем побитых
                        for (const QPoint &p : move.captured) {
                            const_cast<GameLogic*>(this)->board[p.y()][p.x()] = Empty;
                        }
                        // Перемещаем дамку
                        const_cast<GameLogic*>(this)->board[checkR][checkC] = piece;
                        const_cast<GameLogic*>(this)->board[row][col] = Empty;

                        // Проверяем продолжение
                        QVector<Move> next;
                        const_cast<GameLogic*>(this)->getKingCaptures(checkR, checkC, next);

                        if (!next.isEmpty()) {
                            for (const Move &nm : next) {
                                if (!nm.captured.isEmpty()) {
                                    Move full = move;
                                    full.captured.append(nm.captured);
                                    full.to = nm.to;
                                    full.becameKing = move.becameKing || nm.becameKing;
                                    moves.append(full);
                                }
                            }
                        } else {
                            moves.append(move);
                        }

                        memcpy(const_cast<GameLogic*>(this)->board, tempBoard, sizeof(board));
                    }

                    checkR += dr;
                    checkC += dc;
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

    bool found = false;
    for (const Move &m : m_availableMoves) {
        if (m.from == move.from && m.to == move.to) {
            found = true;
            break;
        }
    }
    if (!found) return false;

    bool isCaptureMove = !move.captured.isEmpty();

    applyMove(move);

    int tr = move.to.y();
    int tc = move.to.x();

    // Ensure promotion is applied
    makeKing(tr, tc);

    int newPiece = board[tr][tc];
    qDebug() << "After move at" << tr << tc << "type:" << newPiece << "isKing:" << isKing(newPiece);

    // Check for capture continuation
    bool hasMore = false;
    if (isCaptureMove) {
        QVector<Move> next;
        if (isKing(newPiece)) {
            getKingCaptures(tr, tc, next);
            qDebug() << "KING: found continuations:" << next.size();
            for (const Move &m : next) {
                qDebug() << "  move to" << m.to;
            }
        } else {
            getCaptures(tr, tc, next);
            qDebug() << "REGULAR: found continuations:" << next.size();
        }
        hasMore = !next.isEmpty();
    }

    if (!hasMore) {
        qDebug() << "NO continuation - turn passes";
        m_isWhiteTurn = !m_isWhiteTurn;
        m_selected = QPoint(-1, -1);
        m_availableMoves.clear();

        if (!hasMoves(m_isWhiteTurn) || countPieces(m_isWhiteTurn) == 0) {
            m_gameOver = true;
        }
    } else {
        qDebug() << "HAS continuation - continuing capture";
        m_selected = move.to;
        m_availableMoves = generateMoves(m_selected.y(), m_selected.x());
        qDebug() << "Available moves for continuation:" << m_availableMoves.size();
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

    qDebug() << "applyMove: from" << fr << fc << "to" << tr << tc << "piece:" << piece;

    // Remove captured pieces
    for (const QPoint &p : move.captured) {
        int captured = board[p.y()][p.x()];
        qDebug() << "  captured piece at" << p.y() << p.x() << "type:" << captured;
        if (isWhite(piece) && isBlackPiece(captured)) {
            m_blackCaptured++;
        } else if (isBlackPiece(piece) && isWhite(captured)) {
            m_whiteCaptured++;
        }
        board[p.y()][p.x()] = Empty;
    }

    // Move piece
    board[tr][tc] = piece;
    board[fr][fc] = Empty;

    // ===== ИСПОЛЬЗУЕМ move.becameKing ДЛЯ ПРЕВРАЩЕНИЯ =====
    if (move.becameKing) {
        if (piece == White) {
            board[tr][tc] = WhiteKing;
            qDebug() << "  → WHITE BECAME KING (from move flag)!";
        } else if (piece == Black) {
            board[tr][tc] = BlackKing;
            qDebug() << "  → BLACK BECAME KING (from move flag)!";
        }
    } else {
        // Обычная проверка (если почему-то флаг не сработал)
        if (piece == White && tr == 0) {
            board[tr][tc] = WhiteKing;
            qDebug() << "  → WHITE BECAME KING at row 0!";
        } else if (piece == Black && tr == 7) {
            board[tr][tc] = BlackKing;
            qDebug() << "  → BLACK BECAME KING at row 7!";
        }
    }

    qDebug() << "  after move at" << tr << tc << "type:" << board[tr][tc];
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