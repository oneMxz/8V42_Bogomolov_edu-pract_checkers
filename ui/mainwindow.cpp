#include "mainwindow.h"
#include <QPainter>
#include <QFont>

MainMenu::MainMenu(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
    connectSignals();
    setFixedSize(400, 500);
    setWindowTitle("Шашки");
}

void MainMenu::setupUI()
{
    // Заголовок
    titleLabel = new QLabel("⛂ ШАШКИ ⛂", this);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setFont(QFont("Arial", 28, QFont::Bold));
    titleLabel->setStyleSheet("color: #2c3e50; padding: 20px;");

    // Кнопка "Новая игра"
    newGameButton = new QPushButton("Новая игра", this);
    newGameButton->setFont(QFont("Arial", 14));
    newGameButton->setFixedSize(200, 50);
    newGameButton->setStyleSheet(
        "QPushButton {"
        "   background-color: #3498db;"
        "   color: white;"
        "   border-radius: 10px;"
        "   font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "   background-color: #2980b9;"
        "}"
    );

    // Кнопка "Выход"
    exitButton = new QPushButton("Выход", this);
    exitButton->setFont(QFont("Arial", 14));
    exitButton->setFixedSize(200, 50);
    exitButton->setStyleSheet(
        "QPushButton {"
        "   background-color: #e74c3c;"
        "   color: white;"
        "   border-radius: 10px;"
        "   font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "   background-color: #c0392b;"
        "}"
    );

    // Верстка
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addStretch();
    layout->addWidget(titleLabel);
    layout->addStretch();
    layout->addWidget(newGameButton, 0, Qt::AlignCenter);
    layout->addSpacing(20);
    layout->addWidget(exitButton, 0, Qt::AlignCenter);
    layout->addStretch();

    setLayout(layout);

    // Стиль фона
    setStyleSheet("background-color: #ecf0f1;");
}

void MainMenu::connectSignals()
{
    connect(newGameButton, &QPushButton::clicked, this, &MainMenu::newGameRequested);
    connect(exitButton, &QPushButton::clicked, this, &MainMenu::exitRequested);
}

void MainMenu::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    // Можно добавить декоративную рамку или шахматный узор
    QPainter painter(this);
    painter.setPen(QPen(QColor(189, 195, 199), 2));
    painter.drawRect(10, 10, width() - 20, height() - 20);
}