#pragma once
#include <QWidget>
#include <QDialog>
#include <QStringList>
#include <QVector>
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
    void removeQueueEntry(int index);
    void reorderQueue(const QVector<int>& newToOld);
signals:
    void trackChosen(const QString& path);
    void queueUpdated(int count);
    void queueEntryRemoved(int oldIndex);
    void queueReordered(const QVector<int>& newToOld);
private:
    void addSelectedFiles();
    void addSelectedDirectoryFiles();
    void removeSelected();
    void shiftCurrent(int offset);
    void updateCount();
    void savePlaylist() const;
    void restorePlaylist();
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
    void stepPlaylist(int direction);
    bool advancePlaylist(int direction, bool wrap);
    void rebuildPlayOrder();
    void syncModeDisplay();
    void handleModuleEnd();
    void showQuickPlaylist(const QPoint& globalPosition);
    void showQuickPlaylistEditor();
    void refreshPlaylistCounters();
    void setPanelVisible(QWidget* panel);
    QLabel* title_ = nullptr;
    QLabel* song_ = nullptr;
    QLabel* pattern_ = nullptr;
    QLabel* status_ = nullptr;
    ClassicDisplay* display_ = nullptr;
    int lcdSongIndex_ = 0; // 1-based selected playlist entry
    int lcdSongTotal_ = 0;
    JukeboxWindow* jukebox_ = nullptr;
    QDialog* info_ = nullptr;
    QLabel* infoTitle_ = nullptr;
    QTableWidget* infoTable_ = nullptr;
    QLabel* infoSummary_ = nullptr;
    void refreshModuleInfo();
    void refreshInstrumentRows();
    void showSampleDetails(int index);
    void refreshSampleDetails();
    QDialog* sampleDialog_ = nullptr;
    QLabel* sampleValues_ = nullptr;
    QLabel* sampleName_ = nullptr;
    QLabel* sampleNumber_ = nullptr;
    int sampleIndex_ = 0;
    QString instrumentFilePath_;
    int instrumentScrollOffset_ = 0;
    QStringList instrumentNames_;
    EffectsWindow* effects_ = nullptr;
    ModuleAudio* audio_ = nullptr;
    QPushButton* playButton_ = nullptr;
    QPushButton* pauseButton_ = nullptr;
    QPushButton* stopButton_ = nullptr;
    QTimer* seekTimer_ = nullptr;
    int seekDirection_ = 0;
    bool shuffleEnabled_ = false;
    int repeatMode_ = 0; // off -> all -> one
    bool introscanEnabled_ = false;
    bool remainingTime_ = false;
    QVector<int> playOrder_;
    double currentModuleDuration_ = 0.0;
};
