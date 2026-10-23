#include "ClassicWindow.h"
#include "ModuleAudio.h"

#include <QApplication>
#include <QAbstractItemView>
#include <QDir>
#include <QFileInfo>
#include <QFileSystemModel>
#include <QGridLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QTreeView>
#include <QVBoxLayout>

namespace {
constexpr auto green = "#55ee55";
constexpr auto displayStyle =
    "QLabel {background:#050a05;color:#55ee55;"
    "font-family:Consolas,'Courier New',monospace;font-weight:bold;"
    "border:3px inset #999;padding:5px;}";
constexpr auto classicStyle =
    "QWidget {background:#c0c0c0;color:#101010;font-family:Tahoma,Arial;font-size:11px;}"
    "QPushButton {background:#c0c0c0;border:2px outset #f0f0f0;"
    "padding:3px;min-height:21px;}"
    "QPushButton:pressed {border:2px inset #f0f0f0;}"
    "QListWidget,QTreeView,QTableWidget {background:#050905;color:#55ee55;"
    "font-family:Consolas,'Courier New',monospace;border:3px inset #999;}"
    "QListWidget::item:selected,QTreeView::item:selected,QTableWidget::item:selected"
    "{background:#167a30;color:#fff;}";
QLabel* led(const QString& value, QWidget* parent, int height = 32) {
    auto* label = new QLabel(value, parent);
    label->setStyleSheet(displayStyle);
    label->setMinimumHeight(height);
    label->setAlignment(Qt::AlignVCenter | Qt::AlignCenter);
    return label;
}
QPushButton* actionButton(const QString& text, QLayout* layout, QWidget* parent) {
    auto* button = new QPushButton(text, parent);
    layout->addWidget(button);
    return button;
}
bool isModule(const QString& name) {
    static const QStringList suffixes = {
        "mod","s3m","xm","it","669","mtm","stm","ult","far","okt","med","mptm"
    };
    return suffixes.contains(QFileInfo(name).suffix().toLower());
}
}

JukeboxWindow::JukeboxWindow(QWidget* parent) : QDialog(parent) {
    setWindowTitle("MOD4WIN [2.40] - Open Files / Jukebox");
    setWindowFlags(Qt::Window);
    resize(830, 440);
    setStyleSheet(classicStyle);

    auto* root = new QVBoxLayout(this);
    auto* cols = new QHBoxLayout();
    root->addLayout(cols, 1);

    auto* fileColumn = new QVBoxLayout();
    auto* dirColumn = new QVBoxLayout();
    auto* queueColumn = new QVBoxLayout();
    cols->addLayout(fileColumn, 4);
    cols->addLayout(dirColumn, 3);
    cols->addLayout(queueColumn, 4);

    fileColumn->addWidget(new QLabel("Filelist", this));
    files_ = new QListWidget(this);
    files_->setSelectionMode(QAbstractItemView::ExtendedSelection);
    fileColumn->addWidget(files_, 1);
    auto* fileActions = new QHBoxLayout();
    fileColumn->addLayout(fileActions);
    auto* add = actionButton("Add  >>", fileActions, this);
    auto* addAll = actionButton("All  >>", fileActions, this);

    dirColumn->addWidget(new QLabel("Directories", this));
    model_ = new QFileSystemModel(this);
    model_->setFilter(QDir::AllDirs | QDir::NoDotAndDotDot | QDir::Drives);
    model_->setRootPath(QDir::rootPath());
    directories_ = new QTreeView(this);
    directories_->setModel(model_);
    for (int c = 1; c < model_->columnCount(); ++c) directories_->hideColumn(c);
    directories_->setHeaderHidden(true);
    directories_->setRootIndex(model_->index(QDir::homePath()));
    dirColumn->addWidget(directories_, 1);
    auto* up = actionButton("Parent directory", dirColumn, this);

    queueColumn->addWidget(new QLabel("Playlist", this));
    queue_ = new QListWidget(this);
    queue_->setSelectionMode(QAbstractItemView::ExtendedSelection);
    queueColumn->addWidget(queue_, 1);
    auto* queueActions = new QHBoxLayout();
    queueColumn->addLayout(queueActions);
    auto* remove = actionButton("Remove", queueActions, this);
    auto* moveUp = actionButton("Up", queueActions, this);
    auto* moveDown = actionButton("Down", queueActions, this);
    counter_ = new QLabel(this);
    root->addWidget(counter_);
    auto* play = actionButton("Select module / Play (when audio engine is ready)", root, this);
    play->setEnabled(false);

    auto refreshFiles = [this](const QString& dir) {
        files_->clear();
        const QFileInfoList entries = QDir(dir).entryInfoList(
            QDir::Files | QDir::Readable, QDir::Name | QDir::IgnoreCase);
        for (const QFileInfo& info : entries) {
            if (!isModule(info.fileName())) continue;
            auto* item = new QListWidgetItem(info.fileName(), files_);
            item->setData(Qt::UserRole, info.absoluteFilePath());
        }
    };
    connect(directories_, &QTreeView::clicked, this, [this, refreshFiles](const QModelIndex& idx) {
        const QString path = model_->filePath(idx);
        if (QFileInfo(path).isDir()) refreshFiles(path);
    });
    connect(up, &QPushButton::clicked, this, [this, refreshFiles] {
        const QModelIndex idx = directories_->currentIndex();
        QString current = idx.isValid() ? model_->filePath(idx) : QDir::homePath();
        current = QFileInfo(current).dir().absolutePath();
        directories_->setCurrentIndex(model_->index(current));
        refreshFiles(current);
    });
    connect(add, &QPushButton::clicked, this, &JukeboxWindow::addSelectedFiles);
    connect(addAll, &QPushButton::clicked, this, &JukeboxWindow::addSelectedDirectoryFiles);
    connect(remove, &QPushButton::clicked, this, &JukeboxWindow::removeSelected);
    connect(moveUp, &QPushButton::clicked, this, [this] { shiftCurrent(-1); });
    connect(moveDown, &QPushButton::clicked, this, [this] { shiftCurrent(1); });
    connect(files_, &QListWidget::itemDoubleClicked, this, [this] { addSelectedFiles(); });
    connect(queue_, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem* item) {
        emit trackChosen(item->data(Qt::UserRole).toString());
    });

    const QModelIndex home = model_->index(QDir::homePath());
    directories_->setCurrentIndex(home);
    refreshFiles(QDir::homePath());
    updateCount();
}

QStringList JukeboxWindow::tracks() const {
    QStringList result;
    for (int i = 0; i < queue_->count(); ++i)
        result << queue_->item(i)->data(Qt::UserRole).toString();
    return result;
}
void JukeboxWindow::addSelectedFiles() {
    for (QListWidgetItem* item : files_->selectedItems()) {
        auto* next = new QListWidgetItem(item->text(), queue_);
        next->setData(Qt::UserRole, item->data(Qt::UserRole));
    }
    updateCount();
}
void JukeboxWindow::addSelectedDirectoryFiles() {
    for (int i = 0; i < files_->count(); ++i) {
        auto* item = files_->item(i);
        auto* next = new QListWidgetItem(item->text(), queue_);
        next->setData(Qt::UserRole, item->data(Qt::UserRole));
    }
    updateCount();
}
void JukeboxWindow::removeSelected() {
    qDeleteAll(queue_->selectedItems());
    updateCount();
}
void JukeboxWindow::shiftCurrent(int offset) {
    const int i = queue_->currentRow(), dest = i + offset;
    if (i < 0 || dest < 0 || dest >= queue_->count()) return;
    auto* item = queue_->takeItem(i);
    queue_->insertItem(dest, item);
    queue_->setCurrentRow(dest);
}
void JukeboxWindow::updateCount() {
    counter_->setText(QString("Filelist: %1    Playlist: %2 modules    (Audio not implemented yet)")
        .arg(files_->count()).arg(queue_->count()));
    emit queueUpdated(queue_->count());
}

ClassicPlayer::ClassicPlayer(QWidget* parent) : QWidget(parent) {
    setWindowTitle("MOD4WIN [2.40 Last Beta] - Mod4Win64 v0.2");
    setFixedSize(846, 208);
    setStyleSheet(classicStyle);
    setWindowFlags(Qt::Window);

    audio_ = new ModuleAudio(this);
    jukebox_ = new JukeboxWindow(this);
    info_ = new QDialog(this);
    info_->setWindowTitle("MOD4WIN [2.40] Module Info");
    info_->setWindowFlags(Qt::Window);
    info_->setStyleSheet(classicStyle);
    info_->resize(780, 480);
    auto* infoLayout = new QVBoxLayout(info_);
    infoLayout->addWidget(led("MODULE INFO", info_, 38));
    auto* instruments = new QTableWidget(31, 2, info_);
    instruments->setHorizontalHeaderLabels({"Instruments 1-15", "Instruments 16-31"});
    instruments->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    instruments->verticalHeader()->hide();
    instruments->setEditTriggers(QAbstractItemView::NoEditTriggers);
    infoLayout->addWidget(instruments);
    infoLayout->addWidget(led("NO MODULE LOADED", info_, 65));

    effects_ = new QDialog(this);
    effects_->setWindowTitle("MOD4WIN [2.40] Effects");
    effects_->setWindowFlags(Qt::Window);
    effects_->setStyleSheet(classicStyle);
    effects_->resize(450, 370);
    auto* effectsLayout = new QVBoxLayout(effects_);
    effectsLayout->addWidget(new QLabel("Effects     Arpeggio   Porta   Vibrato   Volume   Note", effects_));
    auto* grid = new QTableWidget(16, 3, effects_);
    grid->setHorizontalHeaderLabels({"Channel", "Effect activity", "Note"});
    grid->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    grid->verticalHeader()->hide();
    grid->setEditTriggers(QAbstractItemView::NoEditTriggers);
    for (int i = 0; i < 16; ++i) {
        grid->setItem(i, 0, new QTableWidgetItem(QString::number(i + 1)));
        grid->setItem(i, 1, new QTableWidgetItem("................"));
        grid->setItem(i, 2, new QTableWidgetItem("---"));
    }
    effectsLayout->addWidget(grid);
    effectsLayout->addWidget(new QLabel("Live visualisation awaits libopenmpt integration.", effects_));

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(6, 6, 6, 6);
    root->setSpacing(4);

    auto* top = new QHBoxLayout();
    root->addLayout(top);
    top->addWidget(new QLabel("MOD4WIN [2.40 Last Beta]", this));
    top->addStretch();
    auto* mixer = actionButton("Mixer", top, this);
    auto* setup = actionButton("Setup", top, this);
    auto* help = actionButton("Help", top, this);
    mixer->setEnabled(false);
    setup->setEnabled(false);
    connect(help, &QPushButton::clicked, this, [this] {
        QMessageBox::information(this, "Mod4Win64 v0.2",
            "Initial libopenmpt playback; live effects and complete transport controls are planned.");
    });

    title_ = led("SOLACE.S3M", this, 39);
    title_->setText("NO MODULE LOADED");
    root->addWidget(title_);
    auto* meters = new QHBoxLayout();
    root->addLayout(meters);
    song_ = led("SONG  --:-- / --:--", this);
    pattern_ = led("PATTERN  -- / --", this);
    status_ = led("44 kHz    16 Bit     STOPPED", this);
    meters->addWidget(song_, 4);
    meters->addWidget(pattern_, 3);
    meters->addWidget(status_, 4);

    auto* buttons = new QHBoxLayout();
    root->addLayout(buttons);
    stopButton_ = actionButton("■", buttons, this);
    playButton_ = actionButton("▶", buttons, this);
    pauseButton_ = actionButton("Ⅱ", buttons, this);
    for (const QString& label : {"|◀", "◀◀", "▶▶", "▶|"})
        actionButton(label, buttons, this)->setEnabled(false);
    connect(stopButton_, &QPushButton::clicked, audio_, &ModuleAudio::stop);
    connect(playButton_, &QPushButton::clicked, this, [this] {
        if (audio_->isPaused()) audio_->pauseResume();
        else audio_->play();
    });
    connect(pauseButton_, &QPushButton::clicked, audio_, &ModuleAudio::pauseResume);
    connect(audio_, &ModuleAudio::stateChanged, this, [this](const QString& state) {
        status_->setText("48 kHz  16 Bit  " + state);
    });
    buttons->addSpacing(8);
    for (const QString& label : {"Shuffle", "Repeat", "Scan", "Time"})
        actionButton(label, buttons, this)->setEnabled(false);
    auto* open = actionButton("Open", buttons, this);
    auto* file = actionButton("File", buttons, this);
    auto* info = actionButton("Info", buttons, this);
    auto* effects = actionButton("Effects", buttons, this);
    connect(open, &QPushButton::clicked, this, [this] { setPanelVisible(jukebox_); });
    connect(file, &QPushButton::clicked, this, [this] { setPanelVisible(jukebox_); });
    connect(info, &QPushButton::clicked, this, [this] { setPanelVisible(info_); });
    connect(effects, &QPushButton::clicked, this, [this] { setPanelVisible(effects_); });
    connect(jukebox_, &JukeboxWindow::trackChosen, this, &ClassicPlayer::chooseTrack);

    auto* note = new QLabel("MOD4WIN64 0.3 - Initial libopenmpt playback | effects coming next", this);
    root->addWidget(note);
}
void ClassicPlayer::setPanelVisible(QWidget* panel) {
    panel->show();
    panel->raise();
    panel->activateWindow();
}
void ClassicPlayer::chooseTrack(const QString& path) {
    QString error;
    if (!audio_->load(path, &error)) {
        QMessageBox::warning(this, "Cannot open module", error);
        return;
    }
    title_->setText(audio_->moduleTitle().toUpper());
    audio_->play();
}
