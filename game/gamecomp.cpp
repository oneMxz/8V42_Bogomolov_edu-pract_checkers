#include "gamecomp.h"
#include <cstdlib>
#include <ctime>
#include <climits>

GameAI::GameAI()
{
    srand(static_cast<unsigned>(time(nullptr)));
}

GameLogic::Move GameAI::getBestMove(const GameLogic &game, bool isWhite, int depth)
{
    QVector<GameLogic::Move> allMoves = getAllMoves(game, isWhite);

    if (allMoves.isEmpty()) {
        return GameLogic::Move();
    }

    // Обязательные рубки
    QVector<GameLogic::Move> captureMoves;
    for (const GameLogic::Move &move : allMoves) {
        if (move.isCapture()) {
            captureMoves.append(move);
        }
    }

    if (!captureMoves.isEmpty()) {
        int maxCaptures = 0;
        GameLogic::Move bestMove;
        for (const GameLogic::Move &move : captureMoves) {
            if (move.captured.size() > maxCaptures) {
                maxCaptures = move.captured.size();
                bestMove = move;
            }
        }
        return bestMove;
    }

    int bestScore = INT_MIN;
    GameLogic::Move bestMove;
    bool isFirst = true;

    for (const GameLogic::Move &move : allMoves) {
        GameLogic tempGame = game;
        tempGame.makeMove(move);
        int score = minimax(tempGame, depth - 1, false, isWhite);

        // ===== ИНВЕРТИРУЕМ, ЕСЛИ AI ЗА ЧЕРНЫХ =====
        if (!isWhite) {
            score = -score;
        }

        score += (rand() % 20) - 10;

        if (isFirst || score > bestScore) {
            bestScore = score;
            bestMove = move;
            isFirst = false;
        }
    }

    return bestMove;
}

int GameAI::minimax(GameLogic &game, int depth, bool isMaximizing, bool aiIsWhite)
{
    if (depth == 0 || game.isGameOver()) {
        int score = evaluateBoard(game.getBoard());
        return aiIsWhite ? score : -score;
    }

    bool isWhiteTurn = game.isWhiteTurn();
    QVector<GameLogic::Move> moves = getAllMoves(game, isWhiteTurn);

    if (moves.isEmpty()) {
        int score = evaluateBoard(game.getBoard());
        return aiIsWhite ? score : -score;
    }

    if (isMaximizing) {
        int maxScore = INT_MIN;
        for (const GameLogic::Move &move : moves) {
            GameLogic tempGame = game;
            tempGame.makeMove(move);
            int score = minimax(tempGame, depth - 1, false, aiIsWhite);
            maxScore = std::max(maxScore, score);
        }
        return maxScore;
    } else {
        int minScore = INT_MAX;
        for (const GameLogic::Move &move : moves) {
            GameLogic tempGame = game;
            tempGame.makeMove(move);
            int score = minimax(tempGame, depth - 1, true, aiIsWhite);
            minScore = std::min(minScore, score);
        }
        return minScore;
    }
}

QVector<GameLogic::Move> GameAI::getAllMoves(const GameLogic &game, bool isWhite)
{
    QVector<GameLogic::Move> allMoves;

    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            int piece = game.getCell(row, col);

            bool isWhitePiece = game.isWhite(piece);
            if ((isWhite && isWhitePiece) || (!isWhite && game.isBlackPiece(piece))) {
                QVector<GameLogic::Move> moves = game.generateMoves(row, col);
                allMoves.append(moves);
            }
        }
    }

    return allMoves;
}

int GameAI::evaluateBoard(const int board[8][8])
{
    int score = 0;

    int whitePieces = countPieces(board, true);
    int blackPieces = countPieces(board, false);
    int whiteKings = countKings(board, true);
    int blackKings = countKings(board, false);

    score += (whitePieces - blackPieces) * 10;
    score += (whiteKings - blackKings) * 30;
    score += positionBonus(board, true) - positionBonus(board, false);

    return score;
}

int GameAI::countPieces(const int board[8][8], bool white)
{
    int count = 0;
    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) {
            int piece = board[r][c];
            if ((white && (piece == GameLogic::White || piece == GameLogic::WhiteKing)) ||
                (!white && (piece == GameLogic::Black || piece == GameLogic::BlackKing))) {
                count++;
            }
        }
    }
    return count;
}

int GameAI::countKings(const int board[8][8], bool white)
{
    int count = 0;
    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) {
            int piece = board[r][c];
            if ((white && piece == GameLogic::WhiteKing) ||
                (!white && piece == GameLogic::BlackKing)) {
                count++;
            }
        }
    }
    return count;
}

int GameAI::positionBonus(const int board[8][8], bool white)
{
    int bonus = 0;

    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) {
            int piece = board[r][c];
            if (piece == GameLogic::Empty) continue;

            bool isWhite = (piece == GameLogic::White || piece == GameLogic::WhiteKing);
            if (isWhite != white) continue;

            if (r >= 2 && r <= 5 && c >= 2 && c <= 5) {
                int distFromCenter = std::abs(r - 3) + std::abs(c - 3);
                bonus += (6 - distFromCenter) * 2;
            }

            if (white) {
                bonus += (7 - r) * 1;
            } else {
                bonus += r * 1;
            }
        }
    }

    return bonus;
}

int GameAI::evaluatePosition(const int board[8][8])
{
    return evaluateBoard(board);
}

bool GameAI::isValid(int r, int c) const
{
    return r >= 0 && r < 8 && c >= 0 && c < 8;
}

bool GameAI::isWhitePiece(int type) const
{
    return type == GameLogic::White || type == GameLogic::WhiteKing;
}

bool GameAI::isBlackPiece(int type) const
{
    return type == GameLogic::Black || type == GameLogic::BlackKing;
}

bool GameAI::isKing(int type) const
{
    return type == GameLogic::WhiteKing || type == GameLogic::BlackKing;
}