#include "ClassicWindow.h"
#include "ModuleAudio.h"
#include "ClassicDisplay.h"
#include "SetupDialog.h"
#include "EffectsWindow.h"
#include "SampleDetails.h"

#include <QApplication>
#include <QAbstractItemView>
#include <QDir>
#include <QComboBox>
#include <QStorageInfo>
#include <QFileInfo>
#include <QFrame>
#include <QScreen>
#include <QGuiApplication>
#include <QRandomGenerator>
#include <QScrollBar>
#include <numeric>
#include <algorithm>
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
#include <QButtonGroup>
#include <QPainter>
#include <QPolygonF>
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
// Draw crisp, flat tracker transport symbols. Qt/OS font glyphs vary in
// spacing and weight, especially when this app moves to macOS. The original
// Mod4Win button geometry is retained, without embossed icon highlights.
class TransportButton final : public QPushButton {
public:
    enum class Symbol { Stop, Play, Pause, Previous, Rewind, Forward, Next };
    explicit TransportButton(Symbol symbol,QWidget* parent)
        : QPushButton(parent), symbol_(symbol) {}
    void setPlaybackActive(bool active) {
        if(playbackActive_==active) return;
        playbackActive_=active;
        update();
    }
protected:
    void paintEvent(QPaintEvent* event) override {
        QPushButton::paintEvent(event); // Existing classic button frame.
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing,true);
        // Playback buttons are bright only for their active state.
        // Navigation buttons brighten for the duration of a mouse press.
        const bool momentary=symbol_==Symbol::Previous ||
            symbol_==Symbol::Rewind || symbol_==Symbol::Forward ||
            symbol_==Symbol::Next;
        const bool lit=momentary?isDown():playbackActive_;
        const QColor ink=!isEnabled()?QColor("#376e48")
            :lit?QColor("#49ef68"):QColor("#176b34");
        painter.setPen(Qt::NoPen);
        painter.setBrush(ink);
        const qreal scale=qMin((width()-10)/32.0,(height()-12)/24.0);
        painter.translate(width()/2.0,height()/2.0);
        painter.scale(scale,scale);
        if(isDown()) painter.translate(1,1);
        auto triangle=[&](qreal x,bool right) {
            const qreal direction=right?1.0:-1.0;
            painter.drawPolygon(QPolygonF{
                QPointF(x-direction*6,-9),
                QPointF(x+direction*6,0),
                QPointF(x-direction*6,9)});
        };
        switch(symbol_) {
        case Symbol::Stop:
            painter.drawRect(QRectF(-9,-9,18,18));
            break;
        case Symbol::Play:
            triangle(0,true);
            break;
        case Symbol::Pause:
            painter.drawRect(QRectF(-9,-9,6,18));
            painter.drawRect(QRectF(3,-9,6,18));
            break;
        case Symbol::Previous:
            triangle(3,false);
            painter.drawRect(QRectF(-13,-9,3,18));
            break;
        case Symbol::Rewind:
            triangle(-6,false);
            triangle(6,false);
            break;
        case Symbol::Forward:
            triangle(-6,true);
            triangle(6,true);
            break;
        case Symbol::Next:
            triangle(-3,true);
            painter.drawRect(QRectF(10,-9,3,18));
            break;
        }
    }
private:
    Symbol symbol_;
    bool playbackActive_ = false;
};
// Same discrete 5x7 LEDs as the main player's module title, with a
// recessed classic border. Unlike the main LCD, this heading has no
// filename toggle: QLabel::text() is always the embedded module title.
class InstrumentMatrixTitle final : public QLabel {
public:
    explicit InstrumentMatrixTitle(const QString& value,QWidget* parent)
        : QLabel(value,parent) {}
protected:
    void paintEvent(QPaintEvent*) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing,false);
        p.fillRect(rect(),QColor("#c0c0c0"));
        p.setPen(QColor("#707070"));
        p.drawLine(0,0,width()-1,0);
        p.drawLine(0,0,0,height()-1);
        p.setPen(QColor("#ffffff"));
        p.drawLine(0,height()-1,width()-1,height()-1);
        p.drawLine(width()-1,0,width()-1,height()-1);
        const QRect lcd=rect().adjusted(3,3,-4,-4);
        ClassicDisplay::paintDotMatrix(p,lcd,text());
    }
};
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
    setWindowTitle("Mod4Win64 2026 Beta - Open Files / Jukebox");
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
    setWindowTitle("Mod4Win64 2026 Beta");
    setFixedSize(650, 195);
    setStyleSheet(classicStyle);
    setWindowFlags(Qt::Window);

    audio_ = new ModuleAudio(this);
    jukebox_ = new JukeboxWindow(this);
    info_ = new QDialog(this);
    info_->setWindowTitle("Mod4Win64 2026 Beta - Module Info");
    info_->setWindowFlags(Qt::Window);
    info_->setStyleSheet(classicStyle);
    info_->setFixedSize(540, 410);
    auto* infoLayout = new QVBoxLayout(info_);
    infoLayout->setContentsMargins(8,8,8,8);
    infoLayout->setSpacing(4);
    infoTitle_ = new InstrumentMatrixTitle("MODULE INFO", info_);
    infoTitle_->setFixedHeight(38);
    infoLayout->addWidget(infoTitle_);
    // Compact heading with authentic scroll buttons at either edge.
    auto* headingRow = new QHBoxLayout();
    headingRow->setSpacing(0);
    auto* minus = new QPushButton("-",info_);
    auto* plus = new QPushButton("+",info_);
    for(auto* control : {minus,plus}) {
        control->setFixedSize(19,18);
        control->setStyleSheet("QPushButton{background:#c0c0c0;color:#167c20;"
                               "font:bold 12px Tahoma;border:2px outset #eee;padding:0;}"
                               "QPushButton:pressed{border:2px inset #eee;}");
    }
    auto* instrHeading = new QLabel("Instruments",info_);
    instrHeading->setAlignment(Qt::AlignCenter);
    instrHeading->setFixedHeight(18);
    instrHeading->setStyleSheet(
        "background:#050a05;color:#42f142;border:2px inset #999;"
        "font:bold 13px 'Courier New';");
    headingRow->addWidget(minus);
    headingRow->addWidget(instrHeading,1);
    headingRow->addWidget(plus);
    infoLayout->addLayout(headingRow);
    // Consistent with sample-details navigation: - goes back, + advances.
    connect(minus,&QPushButton::clicked,this,[this] {
        instrumentScrollOffset_=qMax(0,instrumentScrollOffset_-1);
        refreshInstrumentRows();
    });
    connect(plus,&QPushButton::clicked,this,[this] {
        instrumentScrollOffset_=qMin(instrumentScrollOffset_+1,
                                     qMax(0,instrumentNames_.size()-31));
        refreshInstrumentRows();
    });
    infoTable_ = new QTableWidget(16,2,info_);
    infoTable_->horizontalHeader()->hide();
    infoTable_->verticalHeader()->hide();
    infoTable_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    infoTable_->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    infoTable_->verticalHeader()->setDefaultSectionSize(16);
    infoTable_->setSelectionMode(QAbstractItemView::SingleSelection);
    infoTable_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    infoTable_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    infoTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    infoTable_->setShowGrid(false);
    infoTable_->setFocusPolicy(Qt::NoFocus);
    infoTable_->setStyleSheet(
        "QTableWidget{background:#020802;color:#35ff38;border:3px inset #999;"
        "font:bold 11px 'Courier New';}"
        "QTableWidget::item{padding:0 2px;border-bottom:1px solid #394439;}"
        "QTableWidget::item:selected{background:#15792d;color:#fff;}");
    infoLayout->addWidget(infoTable_,1);
    infoSummary_ = new QLabel("NO MODULE LOADED",info_);
    infoSummary_->setAlignment(Qt::AlignCenter);
    infoSummary_->setFixedHeight(55);
    infoSummary_->setStyleSheet(
        "background:#050a05;color:#31ff40;border:3px inset #999;"
        "font:bold 12px 'Courier New';");
    infoLayout->addWidget(infoSummary_);
    // The original opens a small sample-inspection window when an item is selected.
    sampleDialog_ = new QDialog(info_);
    sampleDialog_->setWindowTitle("Instruments");
    sampleDialog_->setWindowFlags(Qt::Window);
    sampleDialog_->setStyleSheet(classicStyle);
    sampleDialog_->setFixedSize(260, 205);
    auto* sampleRoot = new QVBoxLayout(sampleDialog_);
    sampleRoot->setContentsMargins(6,5,6,5);
    sampleRoot->setSpacing(3);
    sampleValues_ = new QLabel(sampleDialog_);
    sampleValues_->setAlignment(Qt::AlignLeft|Qt::AlignVCenter);
    sampleValues_->setWordWrap(true);
    sampleValues_->setFixedHeight(131);
    sampleValues_->setStyleSheet(
        "background:#c0c0c0;color:#101010;font:bold 12px 'Courier New';"
        "border:2px inset #eee;padding:3px;");
    sampleRoot->addWidget(sampleValues_);
    auto* sampleNav = new QHBoxLayout();
    sampleNav->setSpacing(2);
    auto* samplePrev = new QPushButton("-",sampleDialog_);
    auto* sampleNext = new QPushButton("+",sampleDialog_);
    sampleNumber_ = new QLabel("001",sampleDialog_);
    sampleNumber_->setAlignment(Qt::AlignCenter);
    sampleNumber_->setStyleSheet(
        "background:#041006;color:#3aff45;font:bold 12px 'Courier New';"
        "border:2px inset #888;");
    for(auto* control : {samplePrev,sampleNext}) {
        control->setFixedSize(22,21);
        control->setStyleSheet(
            "QPushButton{background:#c0c0c0;color:#148126;"
            "font:bold 13px Tahoma;border:2px outset #eee;padding:0;}"
            "QPushButton:pressed{border:2px inset #eee;}");
    }
    sampleNav->addStretch();
    sampleNav->addWidget(new QLabel("MOD4WIN",sampleDialog_));
    sampleNav->addStretch();
    sampleNav->addWidget(samplePrev);
    sampleNav->addWidget(sampleNumber_);
    sampleNav->addWidget(sampleNext);
    sampleRoot->addLayout(sampleNav);
    sampleName_ = new QLabel(sampleDialog_);
    sampleName_->setAlignment(Qt::AlignCenter);
    sampleName_->setStyleSheet("background:#c0c0c0;color:#101010;"
                               "font:bold 11px 'Courier New';border:2px inset #888;");
    sampleRoot->addWidget(sampleName_);
    connect(samplePrev,&QPushButton::clicked,this,[this]{
        if(sampleIndex_>0) { --sampleIndex_; refreshSampleDetails(); }
    });
    connect(sampleNext,&QPushButton::clicked,this,[this]{
        if(sampleIndex_+1<instrumentNames_.size()) {
            ++sampleIndex_; refreshSampleDetails();
        }
    });
    connect(infoTable_,&QTableWidget::cellClicked,this,[this](int row,int col){
        const int index=instrumentScrollOffset_+row+(col?15:0);
        if((col==0 && row>=15) || index<0 || index>=instrumentNames_.size()) return;
        showSampleDetails(index);
    });
    refreshModuleInfo();

    effects_ = new EffectsWindow(this);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(5, 3, 5, 4);
    root->setSpacing(2);
    auto* header = new QHBoxLayout();
    header->setSpacing(2);
    auto* appName = new QLabel("MOD4WIN64", this);
    // Deliberate Mod4Win64 identity: a bold, extended heading on the
    // original-style Setup/Help toolbar, rather than another LCD row.
    QFont branding("Arial", 16, QFont::Black);
    branding.setStretch(130);
    branding.setLetterSpacing(QFont::AbsoluteSpacing, 1.2);
    appName->setFont(branding);
    appName->setStyleSheet("color:#194d26;");
    appName->setFixedHeight(27);
    header->addWidget(appName);
    header->addStretch();
    auto* setupButton = new QPushButton("Setup", this);
    auto* helpButton = new QPushButton("Help", this);
    setupButton->setFixedSize(54,24);
    helpButton->setFixedSize(54,24);
    header->addWidget(setupButton);
    header->addWidget(helpButton);
    root->addLayout(header);
    auto* setupDialog = new SetupDialog(this);
    connect(setupButton, &QPushButton::clicked, setupDialog, &QDialog::show);
    connect(setupDialog, &SetupDialog::effectsScaleChanged, effects_, &EffectsWindow::setScaleMode);
    connect(helpButton, &QPushButton::clicked, this, [this] {
        QMessageBox::information(this, "Mod4Win64", 
            "Mod4Win64 is a revival of the classic Mod4Win player.\\n"
            "Original developers: Kay Bruns, Uwe Zaenker and Jens Puchert.\\n"
            "Playback is provided by libopenmpt. Classic effects and surround are under development.");
    });


    // Single painted LCD canvas keeps the original relative dimensions together.
    display_ = new ClassicDisplay(this);
    root->addWidget(display_, 1);
    // PREAMP visual and audio controls share one source of truth.
    // Default 0 dB maps to unity gain (no change to the sound).
    connect(display_, &ClassicDisplay::preampGainChanged,
            audio_, &ModuleAudio::setPreampDb);

    // One compact control strip. The transport keys occupy the full height
    // of the two-row command bank: no staggered vertical alignment or
    // arbitrary button widths at different display scales.
    auto* controls = new QHBoxLayout();
    // Match the LCD layout: left section, narrow dividing gap, right section.
    controls->setSpacing(0);
    controls->setContentsMargins(0, 0, 0, 0);
    root->addLayout(controls);
    auto* transportRow = new QHBoxLayout();
    transportRow->setSpacing(2);
    transportRow->setContentsMargins(0,0,0,0);
    // The transport keys end at logical x=408, like the lower LCD.
    // The right command group starts at x=417, like the audio-status pane.
    controls->addLayout(transportRow, 408);
    using Icon = TransportButton::Symbol;
    auto transport = [this, transportRow](Icon symbol,bool enabled=true) {
        auto* button=new TransportButton(symbol,this);
        transportRow->addWidget(button);
        button->setFixedHeight(58);
        button->setMinimumWidth(30);
        button->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed);
        button->setStyleSheet(
            "QPushButton {background:#c0c0c0;border:2px outset #eee;padding:0;}"
            "QPushButton:pressed {border:2px inset #eee;}");
        button->setEnabled(enabled);
        return button;
    };
    stopButton_ = transport(Icon::Stop);
    playButton_ = transport(Icon::Play);
    pauseButton_ = transport(Icon::Pause);
    auto* previous=transport(Icon::Previous);
    auto* rewind=transport(Icon::Rewind);
    auto* forward=transport(Icon::Forward);
    auto* next=transport(Icon::Next);

    auto* commandGrid = new QGridLayout();
    commandGrid->setContentsMargins(0,0,0,0);
    commandGrid->setHorizontalSpacing(2);
    commandGrid->setVerticalSpacing(2);
    controls->addSpacing(9);
    controls->addLayout(commandGrid, 209);
    auto command = [this, commandGrid](const QString& name, int row, int col) {
        auto* button = new QPushButton(name, this);
        // Two 28px rows plus the 2px grid gap exactly match the
        // 58px-high transport keys, including the top and bottom edges.
        button->setStyleSheet(
            "QPushButton {background:#c0c0c0;border:2px outset #f0f0f0;"
            "padding:0px;min-height:0px;}"
            "QPushButton:pressed {border:2px inset #f0f0f0;}");
        button->setFixedHeight(28);
        button->setMinimumWidth(42);
        button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        commandGrid->addWidget(button, row, col);
        return button;
    };
    auto* shuffleButton=command("Shuffle",0,0);
    auto* repeatButton=command("Repeat",0,1);
    auto* scanButton=command("Scan",0,2);
    auto* timeButton=command("Time",0,3);
    const QString modeStyle=
        "QPushButton:checked {background:#9bcfa3;color:#07541a;"
        "font-weight:bold;border:2px inset #eee;}";
    for(auto* button:{shuffleButton,repeatButton,scanButton,timeButton}) {
        button->setCheckable(true);
        button->setStyleSheet(button->styleSheet()+modeStyle);
    }
    auto* open = command("Open",1,0);
    auto* file = command("File",1,1);
    auto* info = command("Info",1,2);
    auto* effects = command("Effects",1,3);
    connect(stopButton_, &QPushButton::clicked, audio_, &ModuleAudio::stop);
    connect(playButton_, &QPushButton::clicked, audio_, &ModuleAudio::play);
    connect(pauseButton_, &QPushButton::clicked, audio_, &ModuleAudio::pauseResume);
    connect(audio_, &ModuleAudio::stateChanged, display_, &ClassicDisplay::setState);
    // Only the active transport state is illuminated. During seeking, all
    // three remain dim; navigation highlights remain strictly press-driven.
    auto* stopIcon=static_cast<TransportButton*>(stopButton_);
    auto* playIcon=static_cast<TransportButton*>(playButton_);
    auto* pauseIcon=static_cast<TransportButton*>(pauseButton_);
    stopIcon->setPlaybackActive(true); // Startup: no module playing.
    connect(audio_,&ModuleAudio::stateChanged,this,
        [stopIcon,playIcon,pauseIcon](const QString& state) {
            stopIcon->setPlaybackActive(state=="STOPPED" ||
                state=="READY" || state=="NO MODULE LOADED");
            playIcon->setPlaybackActive(state=="PLAYING");
            pauseIcon->setPlaybackActive(state=="PAUSED");
        });
    // Previous/next select adjacent queued modules; do not wrap here.
    connect(previous,&QPushButton::clicked,this,[this] { stepPlaylist(-1); });
    connect(next,&QPushButton::clicked,this,[this] { stepPlaylist(1); });

    // Held seek = no sound while depressed. Seek at ~12x real time,
    // immediately on press and at a steady interval until release.
    seekTimer_=new QTimer(this);
    seekTimer_->setInterval(100);
    connect(seekTimer_,&QTimer::timeout,this,[this] {
        if(seekDirection_!=0) audio_->seekBy(1.2*seekDirection_);
    });
    const auto beginHeldSeek=[this](int direction) {
        if(seekDirection_!=0) return;
        audio_->beginSeek();
        if(!audio_->isSeeking()) return;
        seekDirection_=direction;
        audio_->seekBy(1.0*direction);
        seekTimer_->start();
    };
    const auto endHeldSeek=[this] {
        if(seekDirection_==0) return;
        seekTimer_->stop();
        seekDirection_=0;
        audio_->endSeek();
    };
    connect(rewind,&QPushButton::pressed,this,[beginHeldSeek] { beginHeldSeek(-1); });
    connect(forward,&QPushButton::pressed,this,[beginHeldSeek] { beginHeldSeek(1); });
    connect(rewind,&QPushButton::released,this,endHeldSeek);
    connect(forward,&QPushButton::released,this,endHeldSeek);
    // Avoid leaving playback silenced if the window deactivates mid-hold.
    connect(qApp,&QApplication::applicationStateChanged,this,
        [endHeldSeek](Qt::ApplicationState state) {
            if(state!=Qt::ApplicationActive) endHeldSeek();
        });
    connect(open, &QPushButton::clicked, this, [this] { setPanelVisible(jukebox_); });
    connect(file, &QPushButton::clicked, this, [this] { setPanelVisible(jukebox_); });
    connect(info, &QPushButton::clicked, this, [this] { setPanelVisible(info_); });
    connect(effects, &QPushButton::clicked, this, [this] {
        if (effects_->isVisible()) effects_->hide();
        else setPanelVisible(effects_);
    });
    // Modes all update both button state and the original LCD indicators.
    connect(shuffleButton,&QPushButton::clicked,this,[this](bool checked) {
        shuffleEnabled_=checked;
        rebuildPlayOrder();
        syncModeDisplay();
    });
    connect(repeatButton,&QPushButton::clicked,this,
        [this,repeatButton](bool) {
            repeatMode_=(repeatMode_+1)%3;
            repeatButton->setChecked(repeatMode_!=0);
            repeatButton->setToolTip(repeatMode_==2?"Repeat current module":
                repeatMode_==1?"Repeat whole playlist":"Repeat off");
            syncModeDisplay();
        });
    connect(scanButton,&QPushButton::toggled,this,[this](bool checked) {
        introscanEnabled_=checked;
        syncModeDisplay();
    });
    connect(timeButton,&QPushButton::toggled,this,[this](bool checked) {
        remainingTime_=checked;
        syncModeDisplay();
    });
    connect(jukebox_,&JukeboxWindow::queueUpdated,this,[this](int count) {
        lcdSongTotal_=count;
        rebuildPlayOrder();
    });
    connect(audio_,&ModuleAudio::endOfModule,this,&ClassicPlayer::handleModuleEnd);
    connect(jukebox_, &JukeboxWindow::trackChosen, this, &ClassicPlayer::chooseTrack);
    connect(display_,&ClassicDisplay::titleMatrixClicked,this,
            &ClassicPlayer::showQuickPlaylist);

    auto* diagnosticTimer = new QTimer(this);
    diagnosticTimer->setInterval(1000);
    connect(diagnosticTimer, &QTimer::timeout, this, [this] {
        display_->setDiagnostics(audio_->decodedFrames(), audio_->nonzeroSamples());
    });
    diagnosticTimer->start();
    // Refresh the visualisation often enough to catch tracker rows.
    auto* effectsTimer = new QTimer(this);
    effectsTimer->setInterval(50);
    connect(effectsTimer, &QTimer::timeout, this, [this] {
        // Consume the timestamped presentation state ONCE. Sharing it with
        // Effects and the main LCD prevents either window from advancing
        // independently of the sound actually leaving the audio device.
        const EffectSnapshot snapshot=audio_->effectsSnapshot();
        // Introscan plays the first ten seconds of each queued module.
        // Skip only while actively playing; pause and held seek never trigger it.
        if(introscanEnabled_ && audio_->isPlaying() && !audio_->isPaused() &&
           !audio_->isSeeking() && snapshot.elapsedSeconds>=10.0 &&
           !snapshot.moduleIdentity.isEmpty()) {
            if(!advancePlaylist(1,repeatMode_==1)) audio_->stop();
            return;
        }
        display_->setPlaybackCounters(snapshot.elapsedSeconds,
            lcdSongIndex_,lcdSongTotal_,
            snapshot.moduleIdentity.isEmpty()?0:snapshot.order+1,
            snapshot.orderCount);
        if (effects_->isVisible()) effects_->setSnapshot(snapshot);
    });
    effectsTimer->start();
}
void ClassicPlayer::showQuickPlaylist(const QPoint& globalPosition) {
    const QStringList queue=jukebox_->tracks();
    // Native Qt::Popup behaviour automatically dismisses on outside click
    // or Escape. Selection alone never loads a module.
    auto* popup=new QFrame(this,Qt::Popup | Qt::FramelessWindowHint);
    popup->setAttribute(Qt::WA_DeleteOnClose);
    popup->setObjectName("quickPlaylistPopup");
    popup->setStyleSheet(
        "QFrame#quickPlaylistPopup{background:#c0c0c0;"
        "border:3px outset #f0f0f0;}"
        "QListWidget{background:#000000;color:#31ef39;"
        "font:bold 12px 'Courier New';border:2px inset #888;padding:2px;}"
        "QListWidget::item{padding:1px 2px;}"
        "QListWidget::item:selected{background:#28f52f;color:#000000;}"
        "QPushButton{background:#c0c0c0;color:#111;"
        "border:2px outset #efefef;padding:2px;min-height:21px;}"
        "QPushButton:pressed,QPushButton:checked{border:2px inset #efefef;"
        "background:#a8c9a8;font-weight:bold;}");
    auto* layout=new QVBoxLayout(popup);
    layout->setContentsMargins(5,5,5,5);
    layout->setSpacing(3);
    auto* list=new QListWidget(popup);
    list->setSelectionMode(QAbstractItemView::SingleSelection);
    list->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    list->setMinimumSize(206,245);
    layout->addWidget(list);
    auto* sorts=new QHBoxLayout();
    sorts->setSpacing(1);
    layout->addLayout(sorts);
    auto addSortButton=[&](const QString& label) {
        auto* button=new QPushButton(label,popup);
        sorts->addWidget(button);
        return button;
    };
    auto* none=addSortButton("none");
    auto* name=addSortButton("name");
    auto* ext=addSortButton("ext");
    auto* sortGroup=new QButtonGroup(popup);
    sortGroup->setExclusive(true);
    for(auto* button : {none,name,ext}) button->setCheckable(true);
    sortGroup->addButton(none,0); // playlist's original insertion order
    sortGroup->addButton(name,1); // alphabetical filename
    sortGroup->addButton(ext,2);  // extension, then filename
    none->setChecked(true);

    // Store the original playlist index with each item, even when the
    // visual order is sorted. Duplicate filenames remain distinguishable.
    const auto populate=[this,list,queue](int mode) {
        const int oldIndex=list->currentItem()
            ?list->currentItem()->data(Qt::UserRole).toInt()
            :lcdSongIndex_-1;
        QVector<int> indices(queue.size());
        std::iota(indices.begin(),indices.end(),0);
        if(mode!=0) {
            std::stable_sort(indices.begin(),indices.end(),
                [&queue,mode](int a,int b) {
                    const QFileInfo ai(queue.at(a)),bi(queue.at(b));
                    const QString ka=mode==1?ai.fileName():ai.suffix();
                    const QString kb=mode==1?bi.fileName():bi.suffix();
                    const int primary=QString::compare(ka,kb,Qt::CaseInsensitive);
                    if(primary!=0) return primary<0;
                    return QString::compare(ai.fileName(),bi.fileName(),
                                           Qt::CaseInsensitive)<0;
                });
        }
        list->clear();
        for(int index:indices) {
            const QString label=QString("%1:%2")
                .arg(index+1).arg(QFileInfo(queue.at(index)).fileName());
            auto* item=new QListWidgetItem(label,list);
            item->setData(Qt::UserRole,index);
            if(index==oldIndex) list->setCurrentItem(item);
        }
        if(list->count() && !list->currentItem()) list->setCurrentRow(0);
        if(list->currentItem()) list->scrollToItem(list->currentItem());
        list->viewport()->update();
    };
    populate(0);
    // Wire each button directly to rebuilding the visible list. This also
    // makes the behaviour independent of a button-group signal overload.
    // The button group itself controls only the checked/highlight state.
    connect(none,&QPushButton::clicked,popup,[populate] { populate(0); });
    connect(name,&QPushButton::clicked,popup,[populate] { populate(1); });
    connect(ext,&QPushButton::clicked,popup,[populate] { populate(2); });
    connect(list,&QListWidget::itemDoubleClicked,popup,
        [this,popup,queue](QListWidgetItem* item) {
            const int index=item->data(Qt::UserRole).toInt();
            if(index<0 || index>=queue.size()) return;
            const QString path=queue.at(index);
            popup->close();
            chooseTrack(path);
            if(audio_->moduleInfo().filePath==path) {
                lcdSongIndex_=index+1;
                lcdSongTotal_=queue.size();
                const EffectSnapshot snapshot=audio_->effectsSnapshot();
                display_->setPlaybackCounters(snapshot.elapsedSeconds,
                    lcdSongIndex_,lcdSongTotal_,
                    snapshot.order+1,snapshot.orderCount);
            }
        });

    // Anchor near the click, but constrain to the available desktop so
    // the popup stays usable on small screens and scaled displays.
    popup->adjustSize();
    QPoint pos=globalPosition+QPoint(0,5);
    if(QScreen* screen=QGuiApplication::screenAt(globalPosition)) {
        const QRect area=screen->availableGeometry();
        pos.setX(qBound(area.left(),pos.x(),area.right()-popup->width()+1));
        pos.setY(qBound(area.top(),pos.y(),area.bottom()-popup->height()+1));
    }
    popup->move(pos);
    popup->show();
    list->setFocus();
}

void ClassicPlayer::syncModeDisplay() {
    display_->setPlaybackModes(shuffleEnabled_,repeatMode_,
                               introscanEnabled_,remainingTime_);
}
void ClassicPlayer::rebuildPlayOrder() {
    const int count=jukebox_->tracks().size();
    playOrder_.clear();
    for(int i=0;i<count;++i) playOrder_.append(i);
    if(!shuffleEnabled_ || count<2) return;
    // Fisher-Yates, then place the currently selected track first.
    // The order remains stable until Shuffle is toggled or queue changes.
    for(int i=count-1;i>0;--i) {
        const int j=QRandomGenerator::global()->bounded(i+1);
        playOrder_.swapItemsAt(i,j);
    }
    const int current=playOrder_.indexOf(lcdSongIndex_-1);
    if(current>0) playOrder_.swapItemsAt(0,current);
}
bool ClassicPlayer::advancePlaylist(int direction,bool wrap) {
    if(seekDirection_!=0) return false;
    const QStringList playlist=jukebox_->tracks();
    if(playlist.isEmpty()) return false;
    if(playOrder_.size()!=playlist.size()) rebuildPlayOrder();
    const int currentPos=playOrder_.indexOf(lcdSongIndex_-1);
    if(currentPos<0) return false;
    int nextPos=currentPos+direction;
    if(nextPos<0 || nextPos>=playOrder_.size()) {
        if(!wrap) return false;
        nextPos=nextPos<0?playOrder_.size()-1:0;
    }
    const int nextIndex=playOrder_.at(nextPos);
    const QString path=playlist.at(nextIndex);
    chooseTrack(path);
    if(audio_->moduleInfo().filePath!=path) return false;
    // Preserve the selected entry if the queue contains duplicate paths.
    lcdSongIndex_=nextIndex+1;
    lcdSongTotal_=playlist.size();
    const EffectSnapshot snapshot=audio_->effectsSnapshot();
    display_->setPlaybackCounters(snapshot.elapsedSeconds,
        lcdSongIndex_,lcdSongTotal_,snapshot.order+1,snapshot.orderCount);
    return true;
}
void ClassicPlayer::stepPlaylist(int direction) {
    advancePlaylist(direction,repeatMode_==1);
}
void ClassicPlayer::handleModuleEnd() {
    // The audio engine has already stopped the completed track before this
    // signal. Never apply a second stop after loading the next track.
    if(!introscanEnabled_ && repeatMode_==2) {
        audio_->play(); // stopped module is rewound to its beginning
        return;
    }
    // Intro scanning always advances instead of repeating one entry.
    advancePlaylist(1,repeatMode_==1);
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
    // The original title LCD can switch between the module's embedded title
    // and the on-disk filename. Keep both available after load.
    display_->setModuleNames(audio_->moduleTitle(),
                             QFileInfo(path).fileName());
    const QStringList queue=jukebox_->tracks();
    lcdSongTotal_=qMax(1,int(queue.size()));
    const int queuePosition=queue.indexOf(path);
    lcdSongIndex_=queuePosition>=0?queuePosition+1:1;
    currentModuleDuration_=audio_->durationSeconds();
    display_->setModuleDuration(currentModuleDuration_);
    const EffectSnapshot initial=audio_->effectsSnapshot();
    display_->setPlaybackCounters(initial.elapsedSeconds,
        lcdSongIndex_,lcdSongTotal_,initial.order+1,initial.orderCount);
    effects_->setSnapshot(initial);
    refreshModuleInfo();
    audio_->play();
}

void ClassicPlayer::refreshModuleInfo() {
    if(!infoTitle_ || !infoTable_ || !infoSummary_ || !audio_) return;
    const ModuleInfo data=audio_->moduleInfo();
    infoTitle_->setText(data.title.isEmpty()?"MODULE INFO":data.title.toUpper());
    instrumentNames_=data.names;
    instrumentFilePath_=data.filePath;
    if(sampleDialog_ && sampleDialog_->isVisible()) sampleDialog_->hide();
    instrumentScrollOffset_=0;
    refreshInstrumentRows();
    if(data.filePath.isEmpty()) {
        infoSummary_->setText("NO MODULE LOADED");
        return;
    }
    const QString fmt=data.tracker.isEmpty()?data.format:data.tracker;
    const QString format=fmt.isEmpty()?QStringLiteral("Tracker module"):fmt;
    const int slotCount=data.playableInstrumentCount>=0
        ?data.playableInstrumentCount:instrumentNames_.size();
    // Memory footprint of decompressed samples is not available from libopenmpt's
    // public API, so do NOT invent a Mem figure.
    const QString bytes=QString::number((data.fileBytes+1023)/1024);
    const QString line1=QString("%1 channel %2 module with %3 %4")
        .arg(data.channels).arg(format).arg(slotCount)
        .arg(data.instrumentSlots>0?"instruments":"samples");
    const QString line2=QString("File: %1 KB    Patterns: %2")
        .arg(bytes).arg(data.orderCount);
    // Mod4Win compares distinct pattern references with the sequence length,
    // not with the number of stored patterns (which may include unused slots).
    const QString line3=QString("%1 of %2 patterns are different.")
        .arg(data.distinctPatternsInSequence).arg(data.orderCount);
    infoSummary_->setText(line1+"\n"+line2+"\n"+line3);
    infoSummary_->setToolTip(data.filePath);
}

void ClassicPlayer::refreshInstrumentRows() {
    if(!infoTable_) return;
    infoTable_->setRowCount(16);
    // Original viewport: 1–15 at left, 16–31 at right; additional names
    // scroll through the same fixed two-column window.
    for(int row=0;row<16;++row) {
        for(int col=0;col<2;++col) {
            const int index=instrumentScrollOffset_+row+(col?15:0);
            const bool valid=(col==1 || row<15) && index<instrumentNames_.size();
            const QString value=valid
                ?QString("%1 %2").arg(index+1,2).arg(instrumentNames_.at(index))
                :QString{};
            auto* item=new QTableWidgetItem(value);
            item->setFlags(Qt::ItemIsEnabled|Qt::ItemIsSelectable);
            infoTable_->setItem(row,col,item);
        }
    }
}

void ClassicPlayer::showSampleDetails(int index) {
    if(!sampleDialog_ || index<0 || index>=instrumentNames_.size()) return;
    sampleIndex_=index;
    refreshSampleDetails();
    sampleDialog_->show();
    sampleDialog_->raise();
    sampleDialog_->activateWindow();
}

void ClassicPlayer::refreshSampleDetails() {
    if(!sampleValues_ || !sampleName_ || !sampleNumber_) return;
    if(sampleIndex_<0 || sampleIndex_>=instrumentNames_.size()) return;
    const SampleDetails data=SampleInspection::inspect(instrumentFilePath_,sampleIndex_);
    sampleNumber_->setText(QString("%1").arg(sampleIndex_+1,3,10,QChar('0')));
    sampleName_->setText(instrumentNames_.at(sampleIndex_));
    if(!data.supported || !data.hasSample) {
        // Text-only slots and zero-length samples are the same user-facing
        // state. Keep malformed-file / unsupported-format errors distinct.
        const bool emptySlot=data.explanation=="This instrument slot contains no samples"
            || data.explanation=="The first sample contains no audio"
            || data.explanation=="This sample slot contains no audio"
            || data.explanation=="Empty instrument slot";
        if(emptySlot) {
            sampleValues_->setAlignment(Qt::AlignCenter);
            sampleValues_->setText("This instrument slot contains\nno audio");
        } else {
            sampleValues_->setAlignment(Qt::AlignCenter);
            sampleValues_->setText(data.explanation.isEmpty()
                ?QStringLiteral("No PCM sample data"):data.explanation);
        }
        return;
    }
    sampleValues_->setAlignment(Qt::AlignLeft|Qt::AlignVCenter);
    // Sample 1 is shown for multi-sample XM instruments. Additional samples
    // can be supported in a future version without inventing sample values.
    QString text=QString(
        "Length       : %1\n"
        "Loop start   : %2\n"
        "Loop length  : %3\n"
        "Fine tune    : %4\n"
        "Def. volume  : %5\n"
        "Resolution   : %6 bit\n"
        "C-2 Tuning  : %7 Hz")
        .arg(data.length).arg(data.loopStart).arg(data.loopLength)
        .arg(data.fineTune).arg(data.defaultVolume)
        .arg(data.resolution).arg(data.c2Tuning);
    if(data.sampleCount>1)
        text+="\n(first of "+QString::number(data.sampleCount)+" samples)";
    sampleValues_->setText(text);
}
