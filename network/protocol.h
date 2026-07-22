#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <QString>
#include <QDataStream>
#include <QVector>

// ===== ТИПЫ СООБЩЕНИЙ =====
enum class MessageType : quint8 {
    // Клиент → Сервер
    Connect = 1,
    CreateRoom = 2,
    JoinRoom = 3,
    LeaveRoom = 4,
    MakeMove = 5,
    ChatMessage = 6,
    StartGame = 7,

    // Сервер → Клиент
    ConnectionAccepted = 10,
    ConnectionRejected = 11,
    RoomCreated = 12,
    RoomJoined = 13,
    RoomFull = 14,
    GameStarted = 15,
    GameState = 16,
    YourTurn = 17,
    OpponentTurn = 18,
    GameOver = 19,
    ChatBroadcast = 20,
    PlayerJoined = 21,
    PlayerLeft = 22,
    ColorAssigned = 23,
    TimerUpdate = 24,
    RoomList = 25,

    Error = 99
};

// ===== СТРУКТУРЫ ДЛЯ СЕТИ =====
struct NetworkMove {
    int fromRow;
    int fromCol;
    int toRow;
    int toCol;
};

struct BoardState {
    int board[8][8];
    bool isWhiteTurn;
    int whitePieces;
    int blackPieces;
    int whiteCaptured;
    int blackCaptured;
};

// ===== СЕРИАЛИЗАЦИЯ =====

// NetworkMove
inline QDataStream &operator<<(QDataStream &out, const NetworkMove &move) {
    out << move.fromRow << move.fromCol << move.toRow << move.toCol;
    return out;
}

inline QDataStream &operator>>(QDataStream &in, NetworkMove &move) {
    in >> move.fromRow >> move.fromCol >> move.toRow >> move.toCol;
    return in;
}

// BoardState
inline QDataStream &operator<<(QDataStream &out, const BoardState &state) {
    for (int i = 0; i < 8; ++i) {
        for (int j = 0; j < 8; ++j) {
            out << state.board[i][j];
        }
    }
    out << state.isWhiteTurn;
    out << state.whitePieces;
    out << state.blackPieces;
    out << state.whiteCaptured;
    out << state.blackCaptured;
    return out;
}

inline QDataStream &operator>>(QDataStream &in, BoardState &state) {
    for (int i = 0; i < 8; ++i) {
        for (int j = 0; j < 8; ++j) {
            in >> state.board[i][j];
        }
    }
    in >> state.isWhiteTurn;
    in >> state.whitePieces;
    in >> state.blackPieces;
    in >> state.whiteCaptured;
    in >> state.blackCaptured;
    return in;
}

#endif // PROTOCOL_H