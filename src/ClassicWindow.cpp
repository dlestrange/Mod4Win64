#include "ClassicWindow.h"
#include "ModuleAudio.h"
#include "ClassicDisplay.h"

#include <QApplication>
#include <QAbstractItemView>
#include <QDir>
#include <QComboBox>
#include <QStorageInfo>
#include <QFileInfo>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
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
#include <QTimer>
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
    auto* drives = new QComboBox(this);
    drives->setToolTip("Choose a local, removable or mapped network drive");
    dirColumn->addWidget(drives);
    auto* pathLabel = new QLabel(this);
    pathLabel->setWordWrap(true);
    dirColumn->addWidget(pathLabel);
    model_ = new QFileSystemModel(this);
    model_->setFilter(QDir::AllDirs | QDir::NoDotAndDotDot | QDir::Drives);
    model_->setRootPath(QDir::rootPath());
    directories_ = new QTreeView(this);
    directories_->setModel(model_);
    for (int c = 1; c < model_->columnCount(); ++c) directories_->hideColumn(c);
    directories_->setHeaderHidden(true);
    // Drive-specific tree roots are selected below.
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
    auto* bottom = new QHBoxLayout();
    root->addLayout(bottom);
    bottom->addStretch();
    auto* ok = actionButton("OK", bottom, this);
    auto* save = actionButton("Save", bottom, this);
    auto* quit = actionButton("Quit", bottom, this);
    auto* help = actionButton("Help", bottom, this);

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
    auto selectFolder = [this, refreshFiles, pathLabel](const QString& path) {
        const QModelIndex idx = model_->index(path);
        if (!idx.isValid()) return;
        directories_->setCurrentIndex(idx);
        directories_->scrollTo(idx);
        pathLabel->setText(QDir::toNativeSeparators(path));
        refreshFiles(path);
        updateCount();
    };
    connect(drives, &QComboBox::currentIndexChanged, this, [this, drives, selectFolder](int index) {
        if (index < 0) return;
        const QString path = drives->itemData(index).toString();
        directories_->setRootIndex(model_->index(path));
        selectFolder(path);
    });
    connect(directories_, &QTreeView::clicked, this, [this, selectFolder](const QModelIndex& idx) {
        const QString path = model_->filePath(idx);
        if (QFileInfo(path).isDir()) selectFolder(path);
    });
    connect(up, &QPushButton::clicked, this, [this, drives, selectFolder] {
        const QString root = drives->currentData().toString();
        const QModelIndex idx = directories_->currentIndex();
        const QString current = idx.isValid() ? model_->filePath(idx) : root;
        if (QDir::cleanPath(current).compare(QDir::cleanPath(root), Qt::CaseInsensitive) == 0) return;
        selectFolder(QFileInfo(current).dir().absolutePath());
    });
    connect(add, &QPushButton::clicked, this, &JukeboxWindow::addSelectedFiles);
    connect(addAll, &QPushButton::clicked, this, &JukeboxWindow::addSelectedDirectoryFiles);
    connect(remove, &QPushButton::clicked, this, &JukeboxWindow::removeSelected);
    connect(moveUp, &QPushButton::clicked, this, [this] { shiftCurrent(-1); });
    connect(moveDown, &QPushButton::clicked, this, [this] { shiftCurrent(1); });
    connect(files_, &QListWidget::itemDoubleClicked, this, [this] { addSelectedFiles(); });
    connect(queue_, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem* item) {
        emit trackChosen(item->data(Qt::UserRole).toString());
        hide();
    });
    connect(ok, &QPushButton::clicked, this, [this] {
        if (queue_->count() == 0) {
            QMessageBox::information(this, "Playlist empty", "Add at least one module to the playlist first.");
            return;
        }
        QListWidgetItem* selected = queue_->currentItem();
        if (!selected) selected = queue_->item(0);
        emit trackChosen(selected->data(Qt::UserRole).toString());
        hide();
    });
    connect(save, &QPushButton::clicked, this, [this] {
        if (queue_->count() == 0) {
            QMessageBox::information(this, "Playlist empty", "Add modules before saving a playlist.");
            return;
        }
        const QString filename = QFileDialog::getSaveFileName(
            this, "Save jukebox playlist", QDir::homePath() + "/Mod4Win64.m3u",
            "M3U playlists (*.m3u)");
        if (filename.isEmpty()) return;
        QFile output(filename);
        if (!output.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QMessageBox::warning(this, "Cannot save playlist", output.errorString());
            return;
        }
        QTextStream stream(&output);
        stream << "#EXTM3U\n";
        for (const QString& path : tracks()) stream << QDir::toNativeSeparators(path) << "\n";
        output.close();
        if (output.error() != QFile::NoError)
            QMessageBox::warning(this, "Cannot save playlist", output.errorString());
        else
            QMessageBox::information(this, "Playlist saved", "Saved " + QString::number(queue_->count()) + " modules.");
    });
    connect(quit, &QPushButton::clicked, this, &QWidget::hide);
    connect(help, &QPushButton::clicked, this, [this] {
        QMessageBox::information(this, "Jukebox Help",
            "Choose a drive, browse folders, select modules and click Add >>.\n"
            "Double-click a playlist entry or click OK to load and play it.\n"
            "Save writes a standard M3U playlist. Quit closes the jukebox without deleting its entries.");
    });

    // Populate a selectable list of filesystem roots (e.g. C:, D:, USB volumes).
    for (const QFileInfo& drive : QDir::drives()) {
        const QString path = drive.absoluteFilePath();
        const QStorageInfo storage(path);
        QString label = QDir::toNativeSeparators(path);
        if (!storage.name().isEmpty()) label += " (" + storage.name() + ")";
        drives->addItem(label, path);
    }
    const QString home = QDir::homePath();
    int initial = 0;
    for (int i = 0; i < drives->count(); ++i) {
        if (home.startsWith(drives->itemData(i).toString(), Qt::CaseInsensitive)) {
            initial = i;
            break;
        }
    }
    if (drives->count() > 0) {
        drives->setCurrentIndex(initial);
        const QString rootPath = drives->currentData().toString();
        directories_->setRootIndex(model_->index(rootPath));
        selectFolder(home.startsWith(rootPath, Qt::CaseInsensitive) ? home : rootPath);
    }
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
    counter_->setText(QString("Filelist: %1    Playlist: %2 modules")
        .arg(files_->count()).arg(queue_->count()));
    emit queueUpdated(queue_->count());
}

ClassicPlayer::ClassicPlayer(QWidget* parent) : QWidget(parent) {
    setWindowTitle("MOD4WIN [2.40 Last Beta] - Mod4Win64 v0.6");
    setFixedSize(650, 170);
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
    root->setContentsMargins(5, 3, 5, 4);
    root->setSpacing(2);

    // Single painted LCD canvas keeps the original relative dimensions together.
    display_ = new ClassicDisplay(this);
    root->addWidget(display_, 1);

    auto* buttons = new QHBoxLayout();
    buttons->setContentsMargins(0, 0, 0, 0);
    buttons->setSpacing(1);
    root->addLayout(buttons);
    auto transport = [this, buttons](const QString& symbol, bool enabled = true) {
        auto* button = actionButton(symbol, buttons, this);
        button->setFixedHeight(27);
        button->setStyleSheet("QPushButton {color:#008e21;font:bold 17px Arial;"
                              "background:#c0c0c0;border:2px outset #eee;padding:0;}");
        button->setEnabled(enabled);
        return button;
    };
    stopButton_ = transport("■");
    playButton_ = transport("▶");
    pauseButton_ = transport("Ⅱ");
    transport("|◀", false);
    transport("◀◀", false);
    transport("▶▶", false);
    transport("▶|", false);
    auto command = [this, buttons](const QString& name) {
        auto* button = actionButton(name, buttons, this);
        button->setFixedHeight(27);
        return button;
    };
    for (const QString& label : {"Shuffle", "Repeat", "Scan", "Time"})
        command(label)->setEnabled(false);
    auto* open = command("Open");
    auto* file = command("File");
    auto* info = command("Info");
    auto* effects = command("Effects");
    connect(stopButton_, &QPushButton::clicked, audio_, &ModuleAudio::stop);
    connect(playButton_, &QPushButton::clicked, audio_, &ModuleAudio::play);
    connect(pauseButton_, &QPushButton::clicked, audio_, &ModuleAudio::pauseResume);
    connect(audio_, &ModuleAudio::stateChanged, display_, &ClassicDisplay::setState);
    connect(open, &QPushButton::clicked, this, [this] { setPanelVisible(jukebox_); });
    connect(file, &QPushButton::clicked, this, [this] { setPanelVisible(jukebox_); });
    connect(info, &QPushButton::clicked, this, [this] { setPanelVisible(info_); });
    connect(effects, &QPushButton::clicked, this, [this] {
        if (effects_->isVisible()) effects_->hide();
        else setPanelVisible(effects_);
    });
    connect(jukebox_, &JukeboxWindow::trackChosen, this, &ClassicPlayer::chooseTrack);

    auto* diagnosticTimer = new QTimer(this);
    diagnosticTimer->setInterval(1000);
    connect(diagnosticTimer, &QTimer::timeout, this, [this] {
        display_->setDiagnostics(audio_->decodedFrames(), audio_->nonzeroSamples());
    });
    diagnosticTimer->start();
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
    display_->setTitle(audio_->moduleTitle().toUpper());
    audio_->play();
}
