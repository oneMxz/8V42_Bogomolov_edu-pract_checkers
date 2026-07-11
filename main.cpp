#include <QApplication>
#include "ui/checkerboard.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setStyle("Fusion");

    CheckerBoard board;
    board.setWindowTitle("Шашки");
    board.show();

    return app.exec();
}