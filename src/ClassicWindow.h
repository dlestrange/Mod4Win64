#pragma once
#include <QWidget>
#include <QDialog>
#include <QStringList>
class QLabel;
class QListWidget;
class QFileSystemModel;
class QTreeView;
class QTableWidget;
class QTimer;
class ModuleAudio;
class ClassicDisplay;
class EffectsWindow;
class QPushButton;

class JukeboxWindow final : public QDialog {
    Q_OBJECT
public:
    explicit JukeboxWindow(QWidget* parent = nullptr);
    QStringList tracks() const;
signals:
    void trackChosen(const QString& path);
    void queueUpdated(int count);
private:
    void addSelectedFiles();
    void addSelectedDirectoryFiles();
    void removeSelected();
    void shiftCurrent(int offset);
    void updateCount();
    QFileSystemModel* model_ = nullptr;
    QTreeView* directories_ = nullptr;
    QListWidget* files_ = nullptr;
    QListWidget* queue_ = nullptr;
    QLabel* counter_ = nullptr;
};

class ClassicPlayer final : public QWidget {
    Q_OBJECT
public:
    explicit ClassicPlayer(QWidget* parent = nullptr);
private:
    void chooseTrack(const QString& path);
    void setPanelVisible(QWidget* panel);
    QLabel* title_ = nullptr;
    QLabel* song_ = nullptr;
    QLabel* pattern_ = nullptr;
    QLabel* status_ = nullptr;
    ClassicDisplay* display_ = nullptr;
    JukeboxWindow* jukebox_ = nullptr;
    QDialog* info_ = nullptr;
    QLabel* infoTitle_ = nullptr;
    QTableWidget* infoTable_ = nullptr;
    QLabel* infoSummary_ = nullptr;
    void refreshModuleInfo();
    EffectsWindow* effects_ = nullptr;
    ModuleAudio* audio_ = nullptr;
    QPushButton* playButton_ = nullptr;
    QPushButton* pauseButton_ = nullptr;
    QPushButton* stopButton_ = nullptr;
};
