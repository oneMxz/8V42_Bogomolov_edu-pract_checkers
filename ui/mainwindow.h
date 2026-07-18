#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QLabel>

class MainMenu : public QWidget
{
    Q_OBJECT

public:
    explicit MainMenu(QWidget *parent = nullptr);

signals:
    void newGameRequested();    // Запрос на новую игру
    void exitRequested();       // Запрос на выход

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QPushButton *newGameButton;
    QPushButton *exitButton;
    QLabel *titleLabel;

    void setupUI();
    void connectSignals();
};

#endif 