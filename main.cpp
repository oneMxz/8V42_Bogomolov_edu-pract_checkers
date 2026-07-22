#include <QApplication>
#include <QInputDialog>
#include "ui/mainwindow.h"
#include "ui/selectmodwindow.h"
#include "ui/checkerboard.h"
#include "ui/networkwindow.h"  // ← ДОБАВИТЬ
#include "network/server.h"    // ← ДОБАВИТЬ (если нужен локальный сервер)

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setStyle("Fusion");

    // Создаём окна
    MainMenu menu;
    ModeSelect modeSelect;
    CheckerBoard board;
    NetworkWindow networkWindow;


    Client client;

    // Передаём клиент в NetworkWindow
    networkWindow.setClient(&client);

    // === СВЯЗИ МЕНЮ ===
    QObject::connect(&menu, &MainMenu::newGameRequested, [&]() {
        menu.hide();
        modeSelect.show();
    });

    QObject::connect(&menu, &MainMenu::exitRequested, [&]() {
        app.quit();
    });

    // === ВЫБОР РЕЖИМА ===
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

    // === СЕТЕВОЙ РЕЖИМ ===
    QObject::connect(&modeSelect, &ModeSelect::networkModeSelected, [&]() {
        modeSelect.hide();

        // Запрашиваем имя игрока
        bool ok;
        QString name = QInputDialog::getText(nullptr, "Вход в игру",
                                             "Введите ваше имя:",
                                             QLineEdit::Normal,
                                             "Игрок", &ok);
        if (!ok || name.isEmpty()) {
            name = "Игрок";
        }

        // Запрашиваем адрес сервера
        QString host = QInputDialog::getText(nullptr, "Подключение к серверу",
                                             "Введите адрес сервера:",
                                             QLineEdit::Normal,
                                             "127.0.0.1", &ok);
        if (!ok || host.isEmpty()) {
            host = "127.0.0.1";
        }

        client.setPlayerName(name);
        networkWindow.setPlayerName(name);
        networkWindow.setWindowTitle("Сетевая игра - " + name);

        // ===== СНАЧАЛА УСТАНАВЛИВАЕМ КЛИЕНТ =====
        networkWindow.setClient(&client);

        // ===== ПОТОМ ЗАПУСКАЕМ СЕРВЕР =====
        networkWindow.startServer();

        networkWindow.show();

        // Подключаемся к серверу
        if (!client.connectToServer(host, 5555)) {
            QMessageBox::critical(nullptr, "Ошибка", "Не удалось подключиться к серверу");
            networkWindow.hide();
            modeSelect.show();
        }
    });

    // === ИЗ СЕТЕВОГО ОКНА В ИГРУ ===
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

    // === ВЫХОД ИЗ ИГРЫ В МЕНЮ ===
    QObject::connect(&board, &CheckerBoard::exitToMenuRequested, [&]() {
        board.hide();
        if (board.isNetworkMode()) {
            client.disconnectFromServer();
        }
        menu.show();
    });

    QObject::connect(&modeSelect, &ModeSelect::backRequested, [&]() {
        modeSelect.hide();
        menu.show();
    });

    // === ЗАПУСК ===
    menu.show();
    return app.exec();
}