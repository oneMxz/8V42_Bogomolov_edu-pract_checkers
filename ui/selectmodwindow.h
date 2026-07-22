
#ifndef SELECTMODWINDOW_H
#define SELECTMODWINDOW_H

#include <QWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QLabel>

class ModeSelect : public QWidget
{
    Q_OBJECT

public:
    enum GameMode {
        ModeAI,         // Против компьютера
        ModeLocal,      // На одном ПК
        ModeNetwork     // По сети
    };

    explicit ModeSelect(QWidget *parent = nullptr);

signals:
    void modeSelected(GameMode mode);
    void networkModeSelected();
    void backRequested();

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QPushButton *aiButton;
    QPushButton *localButton;
    QPushButton *networkButton;
    QPushButton *backButton;
    QLabel *titleLabel;

    void setupUI();
    void connectSignals();
};

#endif