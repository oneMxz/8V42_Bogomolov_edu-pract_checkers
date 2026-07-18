

#include "selectmodwindow.h"
#include <QPainter>

ModeSelect::ModeSelect(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
    connectSignals();
    setFixedSize(400, 500);
    setWindowTitle("Выбор режима");
}

void ModeSelect::setupUI()
{
    // Заголовок
    titleLabel = new QLabel("Выберите режим игры", this);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setFont(QFont("Arial", 18, QFont::Bold));
    titleLabel->setStyleSheet("color: #2c3e50; padding: 20px;");

    // Кнопка "Против компьютера"
    aiButton = new QPushButton("Против компьютера", this);
    aiButton->setFont(QFont("Arial", 12));
    aiButton->setFixedSize(250, 45);
    aiButton->setStyleSheet(
        "QPushButton {"
        "   background-color: #2ecc71;"
        "   color: white;"
        "   border-radius: 8px;"
        "   font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "   background-color: #27ae60;"
        "}"
        );

    // Кнопка "На одном ПК"
    localButton = new QPushButton("На одном ПК", this);
    localButton->setFont(QFont("Arial", 12));
    localButton->setFixedSize(250, 45);
    localButton->setStyleSheet(
        "QPushButton {"
        "   background-color: #f39c12;"
        "   color: white;"
        "   border-radius: 8px;"
        "   font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "   background-color: #e67e22;"
        "}"
        );

    // Кнопка "По сети"
    networkButton = new QPushButton("По сети (онлайн)", this);
    networkButton->setFont(QFont("Arial", 12));
    networkButton->setFixedSize(250, 45);
    networkButton->setStyleSheet(
        "QPushButton {"
        "   background-color: #9b59b6;"
        "   color: white;"
        "   border-radius: 8px;"
        "   font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "   background-color: #8e44ad;"
        "}"
        );

    // Кнопка "Назад"
    backButton = new QPushButton("← Назад", this);
    backButton->setFont(QFont("Arial", 11));
    backButton->setFixedSize(150, 35);
    backButton->setStyleSheet(
        "QPushButton {"
        "   background-color: #95a5a6;"
        "   color: white;"
        "   border-radius: 8px;"
        "}"
        "QPushButton:hover {"
        "   background-color: #7f8c8d;"
        "}"
        );

    // Верстка
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addStretch();
    layout->addWidget(titleLabel);
    layout->addSpacing(30);
    layout->addWidget(aiButton, 0, Qt::AlignCenter);
    layout->addSpacing(15);
    layout->addWidget(localButton, 0, Qt::AlignCenter);
    layout->addSpacing(15);
    layout->addWidget(networkButton, 0, Qt::AlignCenter);
    layout->addSpacing(40);
    layout->addWidget(backButton, 0, Qt::AlignCenter);
    layout->addStretch();

    setLayout(layout);
    setStyleSheet("background-color: #ecf0f1;");
}

void ModeSelect::connectSignals()
{
    connect(aiButton, &QPushButton::clicked, this, [this]() {
        emit modeSelected(ModeAI);
    });
    connect(localButton, &QPushButton::clicked, this, [this]() {
        emit modeSelected(ModeLocal);
    });
    connect(networkButton, &QPushButton::clicked, this, [this]() {
        emit modeSelected(ModeNetwork);
    });
    connect(backButton, &QPushButton::clicked, this, &ModeSelect::backRequested);
}

void ModeSelect::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter painter(this);
    painter.setPen(QPen(QColor(189, 195, 199), 2));
    painter.drawRect(10, 10, width() - 20, height() - 20);
}