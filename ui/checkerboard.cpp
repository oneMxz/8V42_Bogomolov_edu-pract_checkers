#include "checkerboard.h"
#include <QPainter>
#include <QMouseEvent>
#include <QMessageBox>

CheckerBoard::CheckerBoard(QWidget *parent)
    : QWidget(parent)
    , animProgress(0.0f)
    , isAnimating(false)
    , hoverPos(-1, -1)
{
    setFixedSize(600, 680);
    setMouseTracking(true);

    animTimer = new QTimer(this);
    animTimer->setInterval(16);
    connect(animTimer, &QTimer::timeout, this, &CheckerBoard::animate);
    // ===== СОЗДАЕМ КНОПКУ =====
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
    resetGame(); // ← теперь resetGame публичный
}

void CheckerBoard::resetGame()
{
    m_logic.reset();
    isAnimating = false;
    animTimer->stop();
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
void CheckerBoard::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);

    int size = width() / 8;
    int y = 8 * size;
    int h = height() - y;

    // Позиционируем кнопку в левой части информационной панели
    if (exitButton) {
        exitButton->setGeometry(10, y + 8, 80, 28);
    }
}

void CheckerBoard::updateBoard()
{
    update();
}

void CheckerBoard::onExitButtonClicked()
{
    QMessageBox msgBox;
    msgBox.setWindowTitle("Выход в меню");
    msgBox.setText("Вы уверены, что хотите выйти в главное меню?");
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.button(QMessageBox::Yes)->setText("Да");
    msgBox.button(QMessageBox::No)->setText("Нет");

    int reply = msgBox.exec();

    if (reply == QMessageBox::Yes) {
        emit exitToMenuRequested();
        this->hide();
    }
}

// ============================================================
// СОБЫТИЯ МЫШИ
// ============================================================

void CheckerBoard::mousePressEvent(QMouseEvent *event)
{
    QPoint pos = event->pos();

    if (isAnimating) return;
    if (m_logic.isGameOver()) {
        resetGame();
        return;
    }

    QPoint cell = getCell(event->pos());
    int row = cell.y();
    int col = cell.x();

    if (!m_logic.isValid(row, col) || !m_logic.isBlack(row, col)) return;

    int piece = m_logic.getCell(row, col);

    // Проверяем клик по доступному ходу
    for (const GameLogic::Move &move : m_logic.getAvailableMoves()) {
        if (move.to == cell) {
            if (m_logic.makeMove(move)) {
                startAnimation(move);
                update();
            }
            return;
        }
    }

    // Выбор шашки
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
    if (m_logic.isValid(cell.y(), cell.x()) && m_logic.isBlack(cell.y(), cell.x())) {
        hoverPos = cell;
    } else {
        hoverPos = QPoint(-1, -1);
    }
    update();
}

// ============================================================
// АНИМАЦИЯ
// ============================================================

void CheckerBoard::animate()
{
    animProgress += 0.1f;
    if (animProgress >= 1.0f) {
        animProgress = 1.0f;
        isAnimating = false;
        animTimer->stop();
    }
    update();
}

// ============================================================
// ОТРИСОВКА
// ============================================================

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

    if (m_logic.isGameOver()) drawGameOver(p);
}

void CheckerBoard::drawBoard(QPainter &p)
{
    int size = width() / 8;

    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) {
            QRect rect(c * size, r * size, size, size);
            if ((r + c) % 2 == 1) {
                p.fillRect(rect, QColor(181, 136, 99));
            } else {
                p.fillRect(rect, QColor(240, 217, 181));
            }
        }
    }

    p.setPen(QPen(Qt::black, 2));
    p.drawRect(0, 0, 8 * size, 8 * size);
}

void CheckerBoard::drawCheckers(QPainter &p)
{
    int size = width() / 8;
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

    int size = width() / 8;
    QRect rect(selected.x() * size, selected.y() * size, size, size);

    p.fillRect(rect, QColor(255, 255, 0, 80));
    p.setPen(QPen(Qt::yellow, 2));
    p.drawRect(rect);
}

void CheckerBoard::drawMoves(QPainter &p)
{
    int size = width() / 8;

    for (const GameLogic::Move &move : m_logic.getAvailableMoves()) {
        QRect rect(move.to.x() * size + size/4,
                   move.to.y() * size + size/4,
                   size/2,
                   size/2);

        if (!move.captured.isEmpty()) {
            p.setBrush(QColor(255, 0, 0, 180));
        } else {
            p.setBrush(QColor(0, 255, 0, 180));
        }

        p.setPen(Qt::NoPen);
        p.drawEllipse(rect);
    }
}

void CheckerBoard::drawHover(QPainter &p)
{
    if (hoverPos.x() == -1 || isAnimating) return;

    int size = width() / 8;
    QRect rect(hoverPos.x() * size, hoverPos.y() * size, size, size);
    p.fillRect(rect, QColor(255, 255, 255, 40));
}

void CheckerBoard::drawInfo(QPainter &p)
{
    int size = width() / 8;
    int y = 8 * size;
    int h = height() - y;

    p.fillRect(0, y, width(), h, QColor(45, 45, 45));
    p.setPen(QPen(Qt::gray, 1));
    p.drawLine(0, y, width(), y);

    p.setPen(Qt::white);
    p.setFont(QFont("Arial", 10));

    p.drawText(width() - 120, y + 20, m_logic.isWhiteTurn() ? "Ход белых" : "Ход черных");

    // Белые
    int wx = width() / 2 - 130;
    p.setBrush(Qt::white);
    p.setPen(QPen(Qt::black, 1));
    p.drawEllipse(wx, y + 10, 18, 18);
    p.setPen(Qt::white);
    p.drawText(wx + 25, y + h/2 + 4, "Побито: " + QString::number(m_logic.getBlackCaptured()));

    // Черные
    int bx = width() / 2 + 30;
    p.setBrush(Qt::black);
    p.setPen(QPen(Qt::white, 1));
    p.drawEllipse(bx, y + 10, 18, 18);
    p.setPen(Qt::white);
    p.drawText(bx + 25, y + h/2 + 4, "Побито: " + QString::number(m_logic.getWhiteCaptured()));

    p.setPen(QColor(200, 200, 200));
    p.drawText(width() - 120, y + h/2 + 4,
               QString("Б: %1  Ч: %2").arg(m_logic.countPieces(true)).arg(m_logic.countPieces(false)));
}

void CheckerBoard::drawGameOver(QPainter &p)
{
    int size = width() / 8;
    int infoHeight = height() - 8 * size;

    p.fillRect(0, 0, width(), height() - infoHeight, QColor(0, 0, 0, 180));

    p.setPen(Qt::white);
    p.setFont(QFont("Arial", 28, QFont::Bold));

    QString winner = m_logic.isWhiteTurn() ? "Черные победили!" : "Белые победили!";
    p.drawText(QRect(0, 0, width(), height() - infoHeight), Qt::AlignCenter, winner);

    p.setFont(QFont("Arial", 14));
    p.drawText(QRect(0, height()/2 + 40, width(), 30),
               Qt::AlignCenter, "Кликните для новой игры");
}

// ============================================================
// ВСПОМОГАТЕЛЬНЫЕ
// ============================================================

QPoint CheckerBoard::getCell(const QPoint &pos) const
{
    int size = width() / 8;
    return QPoint(pos.x() / size, pos.y() / size);
}