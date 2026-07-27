#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <QString>
#include <QDataStream>
#include <QVector>
#include <QTcpSocket>

enum class MessageType : quint8 {
    // Клиент → Сервер
    Connect = 1,        // Отправить имя игрока
    CreateRoom = 2,     //Создать игровую комнату
    JoinRoom = 3,       // Присоединиться к комнате
    LeaveRoom = 4,      // Покинуть комнату
    MakeMove = 5,       // Сделать ход
    ChatMessage = 6,    // Отправить сообщение в чат
    StartGame = 7,      // Запросить начало игры (не используется)

    // Сервер → Клиент
    ConnectionAccepted = 10,   //Подключение принято
    ConnectionRejected = 11,   //Подключение отклонено
    RoomCreated = 12,          //Комната создана
    RoomJoined = 13,           //Игрок присоединился к комнате
    RoomFull = 14,             //Комната заполнена
    GameStarted = 15,          //Игра началась
    GameState = 16,            //Состояние доски
    YourTurn = 17,             //Ваш ход
    OpponentTurn = 18,         //Ход соперника
    GameOver = 19,             //Игра окончена
    ChatBroadcast = 20,        //Широковещательное сообщение чата
    PlayerJoined = 21,         //Игрок присоединился к комнате
    PlayerLeft = 22,           //Игрок покинул комнату
    ColorAssigned = 23,        //Назначен цвет (белые/чёрные)
    TimerUpdate = 24,          //Обновление таймера
    RoomList = 25,             //Список комнат

    Error = 99                 // Общая ошибка
};


struct NetworkMove {
    int fromRow;
    int fromCol;
    int toRow;
    int toCol;
};


struct BoardState {
    int board[8][8];          //Доска (значения клеток)
    bool isWhiteTurn;         //Чей ход (true – белые)
    int whitePieces;          //Количество белых шашек
    int blackPieces;          //Количество чёрных шашек
    int whiteCaptured;        //Количество белых, побитых чёрными
    int blackCaptured;        //Количество чёрных, побитых белыми
    bool hasContinuation;     //Есть ли продолжение рубки
    int selectedRow;          //Выбранная строка
    int selectedCol;          //Выбранный столбец
};

//Сериализация
inline QDataStream &operator<<(QDataStream &out, const NetworkMove &move) {
    out << move.fromRow << move.fromCol << move.toRow << move.toCol;
    return out;
}

inline QDataStream &operator>>(QDataStream &in, NetworkMove &move) {
    in >> move.fromRow >> move.fromCol >> move.toRow >> move.toCol;
    return in;
}

inline QDataStream &operator<<(QDataStream &out, const BoardState &state) {
    for (int i = 0; i < 8; ++i)
        for (int j = 0; j < 8; ++j)
            out << state.board[i][j];
    out << state.isWhiteTurn;
    out << state.whitePieces;
    out << state.blackPieces;
    out << state.whiteCaptured;
    out << state.blackCaptured;
    out << state.hasContinuation;
    out << state.selectedRow;
    out << state.selectedCol;
    return out;
}

inline QDataStream &operator>>(QDataStream &in, BoardState &state) {
    for (int i = 0; i < 8; ++i)
        for (int j = 0; j < 8; ++j)
            in >> state.board[i][j];
    in >> state.isWhiteTurn;
    in >> state.whitePieces;
    in >> state.blackPieces;
    in >> state.whiteCaptured;
    in >> state.blackCaptured;
    in >> state.hasContinuation;
    in >> state.selectedRow;
    in >> state.selectedCol;
    return in;
}

namespace SocketUtils {
inline void sendMessage(QTcpSocket *socket, quint8 type, const QByteArray &data = QByteArray())
{
    if (!socket || socket->state() != QTcpSocket::ConnectedState) return;

    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_6_0);

    out << quint32(data.size() + 1); // длина данных + 1 байт для типа
    out << type;
    out.writeRawData(data.constData(), data.size());

    socket->write(block);
    socket->flush();
}

inline bool readMessage(QTcpSocket *socket, quint8 &type, QByteArray &data)
{
    if (!socket || socket->bytesAvailable() < sizeof(quint32)) {
        return false;
    }

    QDataStream in(socket);
    in.setVersion(QDataStream::Qt_6_0);

    quint32 blockSize;
    in >> blockSize;

    if (socket->bytesAvailable() < blockSize) {
        return false;
    }

    in >> type;
    data.resize(blockSize - 1);
    in.readRawData(data.data(), blockSize - 1);
    return true;
}

}
#endif // PROTOCOL_H