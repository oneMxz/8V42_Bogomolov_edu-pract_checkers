#include "checkerboard.h"
#include <QPainter>
#include <QMouseEvent>
#include <QMessageBox>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QTimer>
#include <QDir>

CheckerBoard::CheckerBoard(QWidget *parent)
    : QWidget(parent), animProgress(0.0f), isAnimating(false), hoverPos(-1, -1)
{
    setFixedSize(BOARD_SIZE + 220, BOARD_SIZE + 100);
    setMouseTracking(true);

    animTimer = new QTimer(this);
    animTimer->setInterval(16);
    connect(animTimer, &QTimer::timeout, this, &CheckerBoard::animate);

    exitButton = new QPushButton("Выйти", this);
    exitButton->setFont(QFont("Arial", 9, QFont::Bold));
    exitButton->setStyleSheet(
        "QPushButton {"
        "   background-color: #4a4a4a;"
        "   color: #ff9999;"
        "   border: 1px solid #666666;"
        "   border-radius: 4px;"
        "   padding: 4px 8px;"
        "}"
        "QPushButton:hover {"
        "   background-color: #5a5a5a;"
        "   color: #ff6666;"
        "}"
        "QPushButton:pressed {"
        "   background-color: #3a3a3a;"
        "}"
        );
    connect(exitButton, &QPushButton::clicked, this, &CheckerBoard::onExitButtonClicked);

    saveButton = new QPushButton("Сохранить", this);
    saveButton->setFont(QFont("Arial", 9, QFont::Bold));
    saveButton->setStyleSheet(
        "QPushButton {"
        "   background-color: #2ecc71;"
        "   color: white;"
        "   border: 1px solid #27ae60;"
        "   border-radius: 4px;"
        "   padding: 4px 8px;"
        "}"
        "QPushButton:hover {"
        "   background-color: #27ae60;"
        "}"
        );
    connect(saveButton, &QPushButton::clicked, this, &CheckerBoard::onSaveButtonClicked);

    int chatLeft = BOARD_SIZE + 10;
    m_chatDisplay = new QTextEdit(this);
    m_chatDisplay->setReadOnly(true);
    m_chatDisplay->setStyleSheet(
        "QTextEdit {"
        "   background-color: #1a1a1a;"
        "   color: #cccccc;"
        "   border: 1px solid #444;"
        "   border-radius: 4px;"
        "   font-size: 11px;"
        "   padding: 4px;"
        "}"
        );
    m_chatDisplay->setGeometry(chatLeft, 50, 190, 450);

    m_chatInput = new QLineEdit(this);
    m_chatInput->setPlaceholderText("Сообщение...");
    m_chatInput->setStyleSheet(
        "QLineEdit {"
        "   background-color: #1a1a1a;"
        "   color: #cccccc;"
        "   border: 1px solid #444;"
        "   border-radius: 4px;"
        "   padding: 4px;"
        "   font-size: 11px;"
        "}"
        );
    m_chatInput->setGeometry(chatLeft, 505, 160, 28);

    m_sendButton = new QPushButton("➤", this);
    m_sendButton->setStyleSheet(
        "QPushButton {"
        "   background-color: #3498db;"
        "   color: white;"
        "   border: none;"
        "   border-radius: 4px;"
        "   font-size: 14px;"
        "}"
        "QPushButton:hover {"
        "   background-color: #2980b9;"
        "}"
        );
    m_sendButton->setGeometry(chatLeft + 165, 505, 28, 28);

    connect(m_sendButton, &QPushButton::clicked, this, &CheckerBoard::onSendMessageClicked);
    connect(m_chatInput, &QLineEdit::returnPressed, this, &CheckerBoard::onSendMessageClicked);

    m_timer = new QTimer(this);
    m_timer->setInterval(1000);
    connect(m_timer, &QTimer::timeout, this, &CheckerBoard::onTimerTick);

    resetGame();
    m_moveHistory.clear();

    m_chatDisplay->setVisible(false);
    m_chatInput->setVisible(false);
    m_sendButton->setVisible(false);
}

void CheckerBoard::resetGame()
{
    m_logic.reset();
    isAnimating = false;
    animTimer->stop();
    update();
    m_moveHistory.clear();
    m_moveCounter = 0;
    if (m_timer && !m_networkMode) {
        m_logic.startTimer(300);
        m_timer->start();
    } else if (m_timer && m_networkMode) {
        m_timer->stop();
    }
    update();
}

void CheckerBoard::startAnimation(const GameLogic::Move &move)
{
    animFrom = move.from;
    animTo = move.to;
    animCaptured = move.captured;
    animProgress = 0.0f;
    isAnimating = true;
    animTimer->start();
}

void CheckerBoard::setAIMode(bool enabled, bool aiIsWhite)
{
    m_aiMode = enabled;
    m_aiIsWhite = aiIsWhite;
}

void CheckerBoard::makeAIMove()
{
    if (!m_aiMode || m_logic.isGameOver())
        return;

    bool aiTurn = (m_aiIsWhite && m_logic.isWhiteTurn()) ||
                  (!m_aiIsWhite && !m_logic.isWhiteTurn());
    if (!aiTurn)
        return;

    // Собираем все возможные ходы для фигур AI
    QVector<GameLogic::Move> allAIMoves;
    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            int piece = m_logic.getCell(r, c);
            bool isMine = (m_aiIsWhite && m_logic.isWhite(piece)) ||
                          (!m_aiIsWhite && m_logic.isBlackPiece(piece));
            if (isMine)
                allAIMoves.append(m_logic.generateMoves(r, c));
        }
    }
    m_logic.setAvailableMoves(allAIMoves);

    GameLogic::Move aiMove = m_gameAI.getBestMove(m_logic, m_aiIsWhite);
    if (aiMove.isValid()) {
        if (m_logic.makeMove(aiMove)) {
            startAnimation(aiMove);
            update();
            recordMove(aiMove);
            // Если после хода игра не завершена, AI продолжает (принудительная рубка)
            if (m_aiMode && !m_logic.isGameOver())
                makeAIMove();
        }
    }
}

void CheckerBoard::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    int y = BOARD_SIZE + 8;
    exitButton->setGeometry(10, y, 80, 28);
    saveButton->setGeometry(10, y + 32, 80, 28);
}

void CheckerBoard::updateBoard()
{
    update();
}

void CheckerBoard::onExitButtonClicked()
{
    QMessageBox msgBox;
    msgBox.setWindowTitle("Выход в лобби");
    msgBox.setText("Вы уверены, что хотите покинуть игру?");
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.button(QMessageBox::Yes)->setText("Да");
    msgBox.button(QMessageBox::No)->setText("Нет");

    int reply = msgBox.exec();
    if (reply == QMessageBox::Yes) {
        if (m_networkMode && m_client)
            m_client->sendLeaveRoom();
        emit exitToLobbyRequested();
        hide();
    }
}

void CheckerBoard::onSaveButtonClicked()
{
    if (m_moveHistory.isEmpty()) {
        QMessageBox::information(this, "Сохранение", "Нет ходов для сохранения.");
        return;
    }
    saveGameHistory();
    QMessageBox::information(this, "Сохранение", "Партия сохранена в файл checkers_game.txt");
}

// ===== Обработка событий мыши =====

void CheckerBoard::mousePressEvent(QMouseEvent *event)
{
    if (isAnimating) return;
    if (m_logic.isGameOver()) {
        resetGame();
        return;
    }

    if (m_aiMode) {
        bool aiTurn = (m_aiIsWhite && m_logic.isWhiteTurn()) ||
                      (!m_aiIsWhite && !m_logic.isWhiteTurn());
        if (aiTurn) return;
    }

    if (m_networkMode && !m_isMyTurn) return;

    QPoint cell = getCell(event->pos());
    int row = cell.y();
    int col = cell.x();

    if (!m_logic.isValid(row, col) || !m_logic.isBlack(row, col)) return;

    int piece = m_logic.getCell(row, col);

    // Проверяем, не кликнули ли по доступному ходу
    for (const GameLogic::Move &availMove : m_logic.getAvailableMoves()) {
        if (availMove.to == cell) {
            if (m_networkMode) {
                if (m_client) {
                    m_client->sendMakeMove(availMove.from.y(), availMove.from.x(),
                                           availMove.to.y(), availMove.to.x());
                }
                m_isMyTurn = false;
                m_logic.setSelected(QPoint(-1, -1));
                m_logic.setAvailableMoves(QVector<GameLogic::Move>());
                update();
                return;
            }

            if (m_logic.makeMove(availMove)) {
                startAnimation(availMove);
                update();
                recordMove(availMove);
                if (m_aiMode && !m_logic.isGameOver())
                    makeAIMove();
            }
            return;
        }
    }

    // Выбор своей шашки
    bool isMine = (m_logic.isWhiteTurn() && m_logic.isWhite(piece)) ||
                  (!m_logic.isWhiteTurn() && m_logic.isBlackPiece(piece));

    if (isMine) {
        m_logic.setSelected(cell);
        QVector<GameLogic::Move> moves = m_logic.generateMoves(row, col);
        m_logic.setAvailableMoves(moves);
        update();
    } else {
        m_logic.setSelected(QPoint(-1, -1));
        update();
    }
}

void CheckerBoard::mouseMoveEvent(QMouseEvent *event)
{
    QPoint cell = getCell(event->pos());
    if (m_logic.isValid(cell.y(), cell.x()) && m_logic.isBlack(cell.y(), cell.x()))
        hoverPos = cell;
    else
        hoverPos = QPoint(-1, -1);
    update();
}

// ===== Анимация =====

void CheckerBoard::animate()
{
    animProgress += 0.05f;
    if (animProgress >= 1.0f) {
        animProgress = 1.0f;
        isAnimating = false;
        animTimer->stop();
    }
    update();
}

// ===== Отрисовка =====

void CheckerBoard::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    drawBoard(p);
    drawCheckers(p);
    drawSelection(p);
    drawMoves(p);
    drawHover(p);
    drawInfo(p);

    if (m_logic.isGameOver())
        drawGameOver(p);
}

void CheckerBoard::drawBoard(QPainter &p)
{
    int size = BOARD_SIZE / 8;

    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) {
            QRect rect(c * size, r * size, size, size);
            if ((r + c) % 2 == 1)
                p.fillRect(rect, QColor(181, 136, 99));
            else
                p.fillRect(rect, QColor(240, 217, 181));
        }
    }

    p.setPen(QPen(Qt::black, 2));
    p.drawRect(0, 0, BOARD_SIZE, BOARD_SIZE);
}

void CheckerBoard::drawCheckers(QPainter &p)
{
    int size = BOARD_SIZE / 8;
    int margin = size / 8;

    const int (&board)[8][8] = m_logic.getBoard();

    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) {
            int type = board[r][c];
            if (type == GameLogic::Empty) continue;

            QPoint pos(c, r);

            if (isAnimating) {
                if (animFrom == QPoint(c, r)) {
                    float x = animFrom.x() + (animTo.x() - animFrom.x()) * animProgress;
                    float y = animFrom.y() + (animTo.y() - animFrom.y()) * animProgress;
                    pos = QPoint(int(x), int(y));
                } else if (animTo == QPoint(c, r)) {
                    if (animProgress < 0.3) continue;
                }
            }

            QRect rect(pos.x() * size + margin,
                       pos.y() * size + margin,
                       size - 2 * margin,
                       size - 2 * margin);

            if (m_logic.isWhite(type)) {
                p.setBrush(Qt::white);
                p.setPen(QPen(Qt::black, 1));
            } else {
                p.setBrush(Qt::black);
                p.setPen(QPen(Qt::white, 1));
            }

            p.drawEllipse(rect);

            if (m_logic.isKing(type)) {
                p.setPen(Qt::red);
                p.setFont(QFont("Arial", size / 2, QFont::Bold));
                p.drawText(rect, Qt::AlignCenter, "♔");
            }
        }
    }
}

void CheckerBoard::drawSelection(QPainter &p)
{
    QPoint selected = m_logic.getSelected();
    if (selected.x() == -1) return;

    int size = BOARD_SIZE / 8;
    QRect rect(selected.x() * size, selected.y() * size, size, size);

    p.fillRect(rect, QColor(255, 255, 0, 80));
    p.setPen(QPen(Qt::yellow, 2));
    p.drawRect(rect);
}

void CheckerBoard::drawMoves(QPainter &p)
{
    int size = BOARD_SIZE / 8;

    for (const GameLogic::Move &move : m_logic.getAvailableMoves()) {
        QRect rect(move.to.x() * size + size/4,
                   move.to.y() * size + size/4,
                   size/2,
                   size/2);

        if (!move.captured.isEmpty())
            p.setBrush(QColor(255, 0, 0, 180));
        else
            p.setBrush(QColor(0, 255, 0, 180));

        p.setPen(Qt::NoPen);
        p.drawEllipse(rect);
    }
}

void CheckerBoard::drawHover(QPainter &p)
{
    if (hoverPos.x() == -1 || isAnimating) return;

    int size = BOARD_SIZE / 8;
    QRect rect(hoverPos.x() * size, hoverPos.y() * size, size, size);
    p.fillRect(rect, QColor(255, 255, 255, 40));
}

void CheckerBoard::drawInfo(QPainter &p)
{
    int y = BOARD_SIZE;
    int h = height() - y;

    p.fillRect(0, y, width(), h, QColor(45, 45, 45));
    p.setPen(QPen(Qt::gray, 1));
    p.drawLine(0, y, width(), y);

    p.setPen(Qt::white);
    p.setFont(QFont("Arial", 10));

    p.drawText(width() - 120, y + 20, m_logic.isWhiteTurn() ? "Ход белых" : "Ход черных");

    QString timeText = QString("⏱ %1:%2  %3:%4")
                           .arg(m_logic.getWhiteTime() / 60, 2, 10, QChar('0'))
                           .arg(m_logic.getWhiteTime() % 60, 2, 10, QChar('0'))
                           .arg(m_logic.getBlackTime() / 60, 2, 10, QChar('0'))
                           .arg(m_logic.getBlackTime() % 60, 2, 10, QChar('0'));
    p.drawText(width() - 120, y + h/2 + 4, timeText);

    int wx = width() / 2 - 130;
    p.setBrush(Qt::white);
    p.setPen(QPen(Qt::black, 1));
    p.drawEllipse(wx, y + 10, 18, 18);
    p.setPen(Qt::white);
    p.drawText(wx + 25, y + h/2 + 4, "Побито: " + QString::number(m_logic.getBlackCaptured()));

    int bx = width() / 2 + 30;
    p.setBrush(Qt::black);
    p.setPen(QPen(Qt::white, 1));
    p.drawEllipse(bx, y + 10, 18, 18);
    p.setPen(Qt::white);
    p.drawText(bx + 25, y + h/2 + 4, "Побито: " + QString::number(m_logic.getWhiteCaptured()));
}

void CheckerBoard::drawGameOver(QPainter &p)
{
    int infoHeight = height() - BOARD_SIZE;
    p.fillRect(0, 0, width(), height() - infoHeight, QColor(0, 0, 0, 180));

    p.setPen(Qt::white);
    p.setFont(QFont("Arial", 28, QFont::Bold));

    QString winner = m_logic.isWhiteTurn() ? "Черные победили!" : "Белые победили!";
    p.drawText(QRect(0, 0, width(), height() - infoHeight), Qt::AlignCenter, winner);

    p.setFont(QFont("Arial", 14));
    p.drawText(QRect(0, height()/2 + 40, width(), 30),
               Qt::AlignCenter, "Кликните для новой игры");
}

// ===== Вспомогательные методы =====

QPoint CheckerBoard::getCell(const QPoint &pos) const
{
    int size = BOARD_SIZE / 8;
    return QPoint(pos.x() / size, pos.y() / size);
}

void CheckerBoard::saveGameHistory()
{
    QString path = QDir::homePath() + "/Documents/checkers_game.txt";
    QFile file(path);
    if (!file.open(QIODevice::Append | QIODevice::Text)) return;

    QTextStream out(&file);
    out << "=== Партия " << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << " ===\n";
    out << "Режим: " << (m_aiMode ? "Против компьютера" : "Два игрока") << "\n";
    out << "Победитель: " << (m_logic.isWhiteTurn() ? "Черные" : "Белые") << "\n";
    out << "Счёт: " << m_logic.getWhiteCaptured() << "-" << m_logic.getBlackCaptured() << "\n";
    out << "Ходы:\n";
    for (const QString &move : m_moveHistory) {
        out << move << "\n";
    }
    out << "========================\n\n";
    file.close();
}

void CheckerBoard::recordMove(const GameLogic::Move &move)
{
    QString from = QString("%1%2").arg(QChar('a' + move.from.x())).arg(8 - move.from.y());
    QString to   = QString("%1%2").arg(QChar('a' + move.to.x())).arg(8 - move.to.y());
    QString capture = move.isCapture() ? " (сруб)" : "";
    QString notation = QString("%1-%2%3").arg(from).arg(to).arg(capture);
    m_moveCounter++;
    m_moveHistory.append(QString("%1. %2").arg(m_moveCounter).arg(notation));
}

void CheckerBoard::onTimerTick()
{
    if (!m_timer || !m_timer->isActive()) return;
    m_logic.updateTimer();
    update();
    if (m_logic.isTimerExpired()) {
        m_timer->stop();
        QString winner;
        if (m_logic.getWhiteCaptured() > m_logic.getBlackCaptured())
            winner = "Белые";
        else if (m_logic.getBlackCaptured() > m_logic.getWhiteCaptured())
            winner = "Черные";
        else
            winner = "Ничья";
        m_logic.setGameOver(true);
        update();
    }
}

void CheckerBoard::setNetworkMode(bool enabled)
{
    m_networkMode = enabled;
    m_chatDisplay->setVisible(enabled);
    m_chatInput->setVisible(enabled);
    m_sendButton->setVisible(enabled);
    if (enabled && m_timer)
        m_timer->stop();
}

void CheckerBoard::setPlayerName(const QString &name)
{
    m_playerName = name;
}

// ===== Сетевые слоты =====

void CheckerBoard::onConnected()
{
    addSystemMessage("Подключено к серверу");
    if (m_client)
        m_client->sendConnect(m_playerName);
}

void CheckerBoard::onDisconnected()
{
    addSystemMessage("Отключено от сервера");
    m_networkMode = false;
    m_isMyTurn = false;
}

void CheckerBoard::onRoomCreated(quint16 roomId)
{
    Q_UNUSED(roomId)
}

void CheckerBoard::onRoomJoined(quint16 roomId, const QString &playerName)
{
    Q_UNUSED(playerName)
    addSystemMessage(QString("Вы вошли в комнату %1").arg(roomId));
}

void CheckerBoard::onPlayerJoined(const QString &playerName)
{
    addSystemMessage(QString("%1 присоединился к комнате").arg(playerName));
}

void CheckerBoard::onPlayerLeft(const QString &playerName)
{
    addSystemMessage(QString("%1 покинул комнату").arg(playerName));
}

void CheckerBoard::onGameStarted()
{
    addSystemMessage("Игра началась!");
    m_logic.reset();
    update();
}

void CheckerBoard::onGameState(const BoardState &state)
{
    // Копируем состояние доски
    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            m_logic.setCell(r, c, state.board[r][c]);
        }
    }

    m_logic.setWhiteTurn(state.isWhiteTurn);
    m_logic.setWhiteCaptured(state.whiteCaptured);
    m_logic.setBlackCaptured(state.blackCaptured);

    if (m_networkMode) {
        bool myTurn = (state.isWhiteTurn && m_myColorIsWhite) ||
                      (!state.isWhiteTurn && !m_myColorIsWhite);
        m_isMyTurn = myTurn;
    }

    if (state.hasContinuation) {
        QPoint sel(state.selectedCol, state.selectedRow);
        m_logic.setSelected(sel);
        QVector<GameLogic::Move> moves = m_logic.generateMoves(sel.y(), sel.x());
        m_logic.setAvailableMoves(moves);
    } else {
        m_logic.setSelected(QPoint(-1, -1));
        m_logic.setAvailableMoves(QVector<GameLogic::Move>());
    }

    update();
}

void CheckerBoard::onYourTurn()
{
    // m_isMyTurn уже установлен в onGameState
    addSystemMessage("Ваш ход!");
}

void CheckerBoard::onOpponentTurn()
{
    addSystemMessage("Ход соперника...");
}

void CheckerBoard::onGameOver(const QString &winner)
{
    addSystemMessage(QString("Игра окончена! %1 победил!").arg(winner));
    QMessageBox::information(this, "Игра окончена", winner + " победил!");
    if (m_networkMode && m_client) {
        m_client->sendLeaveRoom();
        QTimer::singleShot(1500, this, [this]() {
            emit exitToLobbyRequested();
            hide();
        });
    }
}

void CheckerBoard::onChatMessage(const QString &sender, const QString &message)
{
    addChatMessage(sender, message);
}

void CheckerBoard::onColorAssigned(bool isWhite)
{
    m_myColorIsWhite = isWhite;
    QString color = isWhite ? "белых" : "чёрных";
    addSystemMessage(QString("Вы играете за %1").arg(color));
}

void CheckerBoard::onTimerUpdate(int whiteTime, int blackTime)
{
    m_logic.setWhiteTime(whiteTime);
    m_logic.setBlackTime(blackTime);
    update();
}

void CheckerBoard::addChatMessage(const QString &sender, const QString &message)
{
    QString formatted = QString("[%1] %2").arg(sender, message);
    m_chatDisplay->append(formatted);
}

void CheckerBoard::onSendMessageClicked()
{
    QString message = m_chatInput->text().trimmed();
    if (message.isEmpty()) return;

    if (m_networkMode && m_client)
        m_client->sendChatMessage(message);
    m_chatInput->clear();
}

void CheckerBoard::addSystemMessage(const QString &message)
{
    QString formatted = QString("🔔 %1").arg(message);
    m_chatDisplay->append(formatted);
}

void CheckerBoard::onNetworkError(const QString &message)
{
    addSystemMessage("Ошибка: " + message);
    QMessageBox::critical(this, "Ошибка сети", message);
}

void CheckerBoard::setClient(Client *client)
{
    if (m_client == client) return;
    m_client = client;

    if (m_client) {
        connect(m_client, &Client::connected, this, &CheckerBoard::onConnected);
        connect(m_client, &Client::disconnected, this, &CheckerBoard::onDisconnected);
        connect(m_client, &Client::error, this, &CheckerBoard::onNetworkError);
        connect(m_client, &Client::roomCreated, this, &CheckerBoard::onRoomCreated);
        connect(m_client, &Client::roomJoined, this, &CheckerBoard::onRoomJoined);
        connect(m_client, &Client::gameStarted, this, &CheckerBoard::onGameStarted);
        connect(m_client, &Client::gameState, this, &CheckerBoard::onGameState);
        connect(m_client, &Client::yourTurn, this, &CheckerBoard::onYourTurn);
        connect(m_client, &Client::opponentTurn, this, &CheckerBoard::onOpponentTurn);
        connect(m_client, &Client::gameOver, this, &CheckerBoard::onGameOver);
        connect(m_client, &Client::chatMessage, this, &CheckerBoard::onChatMessage);
        connect(m_client, &Client::colorAssigned, this, &CheckerBoard::onColorAssigned);
        connect(m_client, &Client::timerUpdate, this, &CheckerBoard::onTimerUpdate);
    }
}