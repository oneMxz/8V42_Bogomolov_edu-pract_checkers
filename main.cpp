#include <QApplication>
#include <QInputDialog>
#include <QMessageBox>
#include "ui/mainwindow.h"
#include "ui/selectmodwindow.h"
#include "ui/checkerboard.h"
#include "ui/networkwindow.h"
#include "network/client.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setStyle("Fusion");

    MainMenu menu;
    ModeSelect modeSelect;
    CheckerBoard board;
    NetworkWindow networkWindow;
    Client client;

    board.setClient(&client);
    networkWindow.setClient(&client);

    QObject::connect(&menu, &MainMenu::newGameRequested, [&]() {
        menu.hide();
        modeSelect.show();
    });

    QObject::connect(&menu, &MainMenu::exitRequested, [&]() {
        app.quit();
    });

    //Выбор режима игры
    QObject::connect(&modeSelect, &ModeSelect::modeSelected, [&](ModeSelect::GameMode mode) {
        modeSelect.hide();

        switch (mode) {
        case ModeSelect::ModeAI:
            board.setWindowTitle("Шашки - против компьютера");
            board.setAIMode(true, false);
            board.setNetworkMode(false);
            board.resetGame();
            board.show();
            break;

        case ModeSelect::ModeLocal:
            board.setWindowTitle("Шашки - два игрока");
            board.setAIMode(false);
            board.setNetworkMode(false);
            board.resetGame();
            board.show();
            break;

        default:
            break;
        }
    });

    //Сетевой режим
    QObject::connect(&modeSelect, &ModeSelect::networkModeSelected, [&]() {
        modeSelect.hide();

        bool ok;
        QString name = QInputDialog::getText(nullptr, "Вход в игру",
                                             "Введите ваше имя:",
                                             QLineEdit::Normal, "Игрок", &ok);
        if (!ok || name.isEmpty())
            name = "Игрок";

        QString host = QInputDialog::getText(nullptr, "Подключение к серверу",
                                             "Введите адрес сервера:",
                                             QLineEdit::Normal, "127.0.0.1", &ok);
        if (!ok || host.isEmpty())
            host = "127.0.0.1";

        client.setPlayerName(name);
        networkWindow.setPlayerName(name);
        networkWindow.setWindowTitle("Сетевая игра - " + name);

        networkWindow.startServer();
        networkWindow.show();

        if (!client.connectToServer(host, 5555)) {
            QMessageBox::critical(nullptr, "Ошибка", "Не удалось подключиться к серверу");
            networkWindow.hide();
            modeSelect.show();
        } else {
            networkWindow.show();
        }
    });

    //Из сетевого окна в игру
    QObject::connect(&networkWindow, &NetworkWindow::joinRoomRequested, [&]() {
        networkWindow.hide();
        board.setWindowTitle("Шашки - по сети");
        board.setAIMode(false);
        board.setNetworkMode(true);
        board.setPlayerName(client.getPlayerName());
        board.resetGame();
        board.show();
    });

    QObject::connect(&networkWindow, &NetworkWindow::backToMenuRequested, [&]() {
        networkWindow.hide();
        client.disconnectFromServer();
        menu.show();
    });

    //Выход из игры в меню (локальный режим)
    QObject::connect(&board, &CheckerBoard::exitToMenuRequested, [&]() {
        board.hide();
        if (board.isNetworkMode())
            client.disconnectFromServer();
        menu.show();
    });

    //Выход из игры в лобби (сетевой режим)
    QObject::connect(&board, &CheckerBoard::exitToLobbyRequested, [&]() {
        board.hide();
        networkWindow.resetState();
        networkWindow.show();
    });

    //Возврат из выбора режима в меню
    QObject::connect(&modeSelect, &ModeSelect::backRequested, [&]() {
        modeSelect.hide();
        menu.show();
    });

    menu.show();
    return app.exec();
}