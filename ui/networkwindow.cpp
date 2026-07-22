#include "networkwindow.h"
#include <QMessageBox>

NetworkWindow::NetworkWindow(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
    connectSignals();
    setWindowTitle("Сетевая игра");
    setFixedSize(500, 600);
}

void NetworkWindow::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);
    mainLayout->setContentsMargins(15, 15, 15, 15);

    // Заголовок
    QLabel *titleLabel = new QLabel("🌐 Сетевая игра", this);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setFont(QFont("Arial", 18, QFont::Bold));
    mainLayout->addWidget(titleLabel);

    // Статус
    m_statusLabel = new QLabel("Подключение к серверу...", this);
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_statusLabel->setStyleSheet("color: #888; font-size: 12px;");
    mainLayout->addWidget(m_statusLabel);

    // Список комнат
    QLabel *roomLabel = new QLabel("Доступные комнаты:", this);
    roomLabel->setFont(QFont("Arial", 11, QFont::Bold));
    mainLayout->addWidget(roomLabel);

    m_roomList = new QListWidget(this);
    m_roomList->setStyleSheet(
        "QListWidget {"
        "   background-color: #2a2a2a;"
        "   color: #ccc;"
        "   border: 1px solid #444;"
        "   border-radius: 5px;"
        "   padding: 5px;"
        "}"
        "QListWidget::item:selected {"
        "   background-color: #3498db;"
        "}"
        );
    m_roomList->setMinimumHeight(150);
    mainLayout->addWidget(m_roomList);

    // Кнопки управления комнатами
    QHBoxLayout *roomButtonsLayout = new QHBoxLayout();
    m_createRoomButton = new QPushButton("Создать комнату", this);
    m_createRoomButton->setStyleSheet(
        "QPushButton {"
        "   background-color: #2ecc71;"
        "   color: white;"
        "   border: none;"
        "   border-radius: 5px;"
        "   padding: 8px;"
        "   font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "   background-color: #27ae60;"
        "}"
        );
    m_joinRoomButton = new QPushButton("Войти в комнату", this);
    m_joinRoomButton->setStyleSheet(
        "QPushButton {"
        "   background-color: #3498db;"
        "   color: white;"
        "   border: none;"
        "   border-radius: 5px;"
        "   padding: 8px;"
        "   font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "   background-color: #2980b9;"
        "}"
        );
    roomButtonsLayout->addWidget(m_createRoomButton);
    roomButtonsLayout->addWidget(m_joinRoomButton);
    mainLayout->addLayout(roomButtonsLayout);

    // Чат
    QLabel *chatLabel = new QLabel("Чат:", this);
    chatLabel->setFont(QFont("Arial", 11, QFont::Bold));
    mainLayout->addWidget(chatLabel);

    m_chatDisplay = new QTextEdit(this);
    m_chatDisplay->setReadOnly(true);
    m_chatDisplay->setStyleSheet(
        "QTextEdit {"
        "   background-color: #1a1a1a;"
        "   color: #ccc;"
        "   border: 1px solid #444;"
        "   border-radius: 5px;"
        "   padding: 5px;"
        "   font-size: 12px;"
        "}"
        );
    m_chatDisplay->setMinimumHeight(150);
    mainLayout->addWidget(m_chatDisplay);

    // Поле ввода сообщения
    QHBoxLayout *inputLayout = new QHBoxLayout();
    m_chatInput = new QLineEdit(this);
    m_chatInput->setPlaceholderText("Введите сообщение...");
    m_chatInput->setStyleSheet(
        "QLineEdit {"
        "   background-color: #2a2a2a;"
        "   color: #ccc;"
        "   border: 1px solid #444;"
        "   border-radius: 5px;"
        "   padding: 8px;"
        "}"
        );
    m_sendButton = new QPushButton("Отправить", this);
    m_sendButton->setStyleSheet(
        "QPushButton {"
        "   background-color: #3498db;"
        "   color: white;"
        "   border: none;"
        "   border-radius: 5px;"
        "   padding: 8px 15px;"
        "   font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "   background-color: #2980b9;"
        "}"
        );
    inputLayout->addWidget(m_chatInput);
    inputLayout->addWidget(m_sendButton);
    mainLayout->addLayout(inputLayout);

    // Кнопка "Назад"
    m_backButton = new QPushButton("← Назад в меню", this);
    m_backButton->setStyleSheet(
        "QPushButton {"
        "   background-color: #e74c3c;"
        "   color: white;"
        "   border: none;"
        "   border-radius: 5px;"
        "   padding: 10px;"
        "   font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "   background-color: #c0392b;"
        "}"
        );
    mainLayout->addWidget(m_backButton);

    setLayout(mainLayout);
    setStyleSheet("background-color: #1e1e1e;");
}

void NetworkWindow::connectSignals()
{
    connect(m_createRoomButton, &QPushButton::clicked, this, &NetworkWindow::onCreateRoomClicked);
    connect(m_joinRoomButton, &QPushButton::clicked, this, &NetworkWindow::onJoinRoomClicked);
    connect(m_sendButton, &QPushButton::clicked, this, &NetworkWindow::onSendMessageClicked);
    connect(m_chatInput, &QLineEdit::returnPressed, this, &NetworkWindow::onSendMessageClicked);
    connect(m_backButton, &QPushButton::clicked, this, &NetworkWindow::onBackClicked);
}

void NetworkWindow::setClient(Client *client)
{
    m_client = client;
    if (!m_client) {
        qDebug() << "Client is null!";
        return;
    }
    qDebug() << "Client set successfully, isConnected:" << m_client->isConnected();

    connect(m_client, &Client::connected, this, [this]() {
        m_statusLabel->setText("✅ Подключено к серверу");
        m_statusLabel->setStyleSheet("color: #2ecc71; font-size: 12px;");
        if (m_client) {
            m_client->sendConnect(m_playerName);
        }
    });

    connect(m_client, &Client::disconnected, this, [this]() {
        m_statusLabel->setText("❌ Отключено от сервера");
        m_statusLabel->setStyleSheet("color: #e74c3c; font-size: 12px;");
    });

    connect(m_client, &Client::roomList, this, &NetworkWindow::onRoomList);
    connect(m_client, &Client::roomCreated, this, &NetworkWindow::onRoomCreated);
    connect(m_client, &Client::roomJoined, this, &NetworkWindow::onRoomJoined);
    connect(m_client, &Client::playerJoined, this, &NetworkWindow::onPlayerJoined);
    connect(m_client, &Client::playerLeft, this, &NetworkWindow::onPlayerLeft);
    connect(m_client, &Client::chatMessage, this, &NetworkWindow::onChatMessage);
    connect(m_client, &Client::gameStarted, this, &NetworkWindow::onGameStarted);
    connect(m_client, &Client::error, this, &NetworkWindow::onConnectionError);
}

void NetworkWindow::setPlayerName(const QString &name)
{
    m_playerName = name;
}

void NetworkWindow::onCreateRoomClicked()
{
    if (!m_client) {
        QMessageBox::warning(this, "Ошибка", "Клиент не инициализирован!");
        return;
    }

    if (!m_client->isConnected()) {
        QMessageBox::warning(this, "Ошибка", "Нет подключения к серверу");
        return;
    }

    m_client->sendCreateRoom();
    m_statusLabel->setText("⏳ Создание комнаты...");
}

void NetworkWindow::onJoinRoomClicked()
{
    if (!m_client || !m_client->isConnected()) {
        QMessageBox::warning(this, "Ошибка", "Нет подключения к серверу");
        return;
    }

    QListWidgetItem *item = m_roomList->currentItem();
    if (!item) {
        QMessageBox::information(this, "Внимание", "Выберите комнату из списка");
        return;
    }

    quint16 roomId = item->data(Qt::UserRole).toUInt();
    m_client->sendJoinRoom(roomId);
    m_statusLabel->setText("⏳ Вход в комнату...");
}

void NetworkWindow::onSendMessageClicked()
{
    QString message = m_chatInput->text().trimmed();
    if (message.isEmpty()) return;

    if (m_client && m_client->isConnected()) {
        m_client->sendChatMessage(message);
        addChatMessage(m_playerName, message);
        m_chatInput->clear();
    }
}

void NetworkWindow::onBackClicked()
{
    if (m_client) {
        m_client->disconnectFromServer();
    }
    emit backToMenuRequested();
}

void NetworkWindow::onRoomList(const QVector<QPair<quint16, QString>> &rooms)
{
    qDebug() << "=== NetworkWindow::onRoomList ===";
    qDebug() << "  Rooms count:" << rooms.size();

    m_roomList->clear();
    for (const auto &room : rooms) {
        qDebug() << "  Room:" << room.first << "players:" << room.second;
        QListWidgetItem *item = new QListWidgetItem(
            QString("Комната %1 (%2 игроков)").arg(room.first).arg(room.second)
            );
        item->setData(Qt::UserRole, room.first);
        m_roomList->addItem(item);
    }
}

void NetworkWindow::onRoomJoined(quint16 roomId, const QString &playerName)
{
    m_currentRoomId = roomId;
    m_statusLabel->setText(QString("✅ В комнате %1. Игроков: %2").arg(roomId).arg(playerName));
    addChatMessage("Система", QString("%1 присоединился к комнате").arg(playerName));
}

void NetworkWindow::onRoomCreated(quint16 roomId)
{
    m_currentRoomId = roomId;
    m_statusLabel->setText(QString("✅ Комната %1 создана, ожидаем игроков...").arg(roomId));
    addChatMessage("Система", "Вы создали комнату. Ожидайте второго игрока.");
    // Можно автоматически войти в комнату (но сервер уже добавил игрока)
    // Если хотите, чтобы после создания сразу перейти к доске, эмитируйте сигнал:
    // emit joinRoomRequested(roomId);
}

void NetworkWindow::onPlayerJoined(const QString &playerName)
{
    addChatMessage("Система", QString("%1 присоединился к комнате").arg(playerName));
}

void NetworkWindow::onPlayerLeft(const QString &playerName)
{
    addChatMessage("Система", QString("%1 вышел из комнаты").arg(playerName));
}

void NetworkWindow::onChatMessage(const QString &sender, const QString &message)
{
    addChatMessage(sender, message);
}

void NetworkWindow::onGameStarted()
{
    QMessageBox::information(this, "Игра началась", "Игра началась! Переход к доске...");
    emit joinRoomRequested(m_currentRoomId);
}

void NetworkWindow::onConnectionError(const QString &message)
{
    QMessageBox::critical(this, "Ошибка подключения", message);
    m_statusLabel->setText("❌ Ошибка: " + message);
}

void NetworkWindow::addChatMessage(const QString &sender, const QString &message)
{
    QString formatted = QString("[%1] %2").arg(sender, message);
    m_chatDisplay->append(formatted);
}

void NetworkWindow::updateRoomList(const QVector<QPair<quint16, QString>> &rooms)
{
    onRoomList(rooms);
}

void NetworkWindow::startServer()
{
    if (m_server) {
        addChatMessage("Система", "Сервер уже запущен!");
        return;
    }

    m_server = new Server(this);
    if (!m_server->startServer(5555)) {
        addChatMessage("Ошибка", "Не удалось запустить сервер на порту 5555!");
        return;
    }

    qDebug() << "Server started on port 5555, listening:" << m_server->isListening();
    addChatMessage("Система", "✅ Сервер запущен на порту 5555!");
}