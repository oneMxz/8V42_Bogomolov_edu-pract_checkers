#include <QApplication>
#include "ui/mainwindow.h"
#include "ui/selectmodwindow.h"
#include "ui/checkerboard.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setStyle("Fusion");

    // Создаем главное меню
    MainMenu menu;
    ModeSelect modeSelect;
    CheckerBoard board;

    // Связываем меню и выбор режима
    QObject::connect(&menu, &MainMenu::newGameRequested, [&]() {
        menu.hide();
        modeSelect.show();
    });

    QObject::connect(&menu, &MainMenu::exitRequested, [&]() {
        app.quit();
    });

    // Связываем выбор режима с игрой
    QObject::connect(&modeSelect, &ModeSelect::modeSelected, [&](ModeSelect::GameMode mode) {
        modeSelect.hide();
        
        switch (mode) {
        case ModeSelect::ModeAI:
            board.setWindowTitle("Шашки - против компьютера");
            // TODO: Включить AI режим
            break;
        case ModeSelect::ModeLocal:
            board.setWindowTitle("Шашки - два игрока");
            // Локальный режим (уже работает)
            break;
        case ModeSelect::ModeNetwork:
            board.setWindowTitle("Шашки - по сети");
            // TODO: Включить сетевой режим
            break;
        }
        
        board.resetGame();
        board.show();
    });

    QObject::connect(&modeSelect, &ModeSelect::backRequested, [&]() {
        modeSelect.hide();
        menu.show();
    });

    // ===== НОВОЕ: выход из игры в меню =====
    QObject::connect(&board, &CheckerBoard::exitToMenuRequested, [&]() {
        board.hide();
        menu.show();
    });

    // Показываем меню
    menu.show();

    return app.exec();
}