#pragma once

#include <QMainWindow>
#include <QStringList>

class QLabel;
class QListWidget;
class QPushButton;

class PlayerWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit PlayerWindow(QWidget* parent = nullptr);

private:
    void addFiles();
    void removeSelected();
    void moveSelected(int offset);
    void refreshStatus();

    QListWidget* playlist_ = nullptr;
    QLabel* status_ = nullptr;
    QLabel* trackTitle_ = nullptr;
    QPushButton* playButton_ = nullptr;
};
