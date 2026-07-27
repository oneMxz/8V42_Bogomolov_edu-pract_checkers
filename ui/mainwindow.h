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
    void newGameRequested();
    void exitRequested();
protected:
    void paintEvent(QPaintEvent *event) override;

private:
    void setupUI();
    void connectSignals();

    QPushButton *newGameButton;
    QPushButton *exitButton;
    QLabel *titleLabel;
};

#endif 