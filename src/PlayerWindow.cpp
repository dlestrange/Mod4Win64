#include "PlayerWindow.h"

#include <QAbstractItemView>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QMenuBar>
#include <QMessageBox>
#include <QPushButton>
#include <QStatusBar>
#include <QVBoxLayout>
#include <QWidget>

PlayerWindow::PlayerWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle("Mod4Win64 - Module Player (pre-alpha)");
    resize(660, 450);

    auto* fileMenu = menuBar()->addMenu("&File");
    auto* openAction = fileMenu->addAction("&Add modules...");
    connect(openAction, &QAction::triggered, this, &PlayerWindow::addFiles);
    fileMenu->addSeparator();
    connect(fileMenu->addAction("E&xit"), &QAction::triggered, this, &QWidget::close);

    auto* helpMenu = menuBar()->addMenu("&Help");
    connect(helpMenu->addAction("&About"), &QAction::triggered, this, [this] {
        QMessageBox::about(this, "About Mod4Win64",
            "Mod4Win64 0.1 pre-alpha\n"
            "A preservation-inspired modern module player.\n\n"
            "This build demonstrates the interface and playlist only. "
            "Audio playback is not implemented yet.");
    });

    auto* central = new QWidget(this);
    auto* layout = new QVBoxLayout(central);
    layout->setSpacing(8);

    trackTitle_ = new QLabel("MOD4WIN64     NO MODULE LOADED", central);
    trackTitle_->setObjectName("display");
    trackTitle_->setMinimumHeight(55);
    layout->addWidget(trackTitle_);

    auto* controls = new QHBoxLayout;
    auto* previous = new QPushButton("|<", central);
    playButton_ = new QPushButton(">", central);
    auto* stop = new QPushButton("[]", central);
    auto* next = new QPushButton(">|", central);
    for (auto* button : {previous, playButton_, stop, next}) {
        button->setFixedWidth(70);
        controls->addWidget(button);
    }
    controls->addStretch();
    layout->addLayout(controls);

    auto* heading = new QLabel("JUKEBOX  /  PLAYLIST", central);
    layout->addWidget(heading);

    playlist_ = new QListWidget(central);
    playlist_->setSelectionMode(QAbstractItemView::ExtendedSelection);
    playlist_->setAlternatingRowColors(false);
    layout->addWidget(playlist_);
    connect(playlist_, &QListWidget::currentRowChanged, this, [this](int row) {
        trackTitle_->setText(row < 0 ? "MOD4WIN64     NO MODULE LOADED"
            : QString("MOD4WIN64     %1").arg(QFileInfo(playlist_->item(row)->text()).fileName().toUpper()));
    });

    auto* actions = new QHBoxLayout;
    auto* add = new QPushButton("Add files...", central);
    auto* remove = new QPushButton("Remove", central);
    auto* up = new QPushButton("Move up", central);
    auto* down = new QPushButton("Move down", central);
    for (auto* button : {add, remove, up, down}) actions->addWidget(button);
    layout->addLayout(actions);
    connect(add, &QPushButton::clicked, this, &PlayerWindow::addFiles);
    connect(remove, &QPushButton::clicked, this, &PlayerWindow::removeSelected);
    connect(up, &QPushButton::clicked, this, [this] { moveSelected(-1); });
    connect(down, &QPushButton::clicked, this, [this] { moveSelected(1); });

    // The playback controls deliberately remain disabled until the audio engine exists.
    for (auto* button : {previous, playButton_, stop, next}) button->setEnabled(false);

    setCentralWidget(central);
    status_ = new QLabel(this);
    statusBar()->addPermanentWidget(status_);
    refreshStatus();

    setStyleSheet(
        "QMainWindow, QWidget { background: #c0c0c0; color: #111; }"
        "QLabel#display { background: #050b05; color: #63ee63;"
        " font-family: Consolas, monospace; font-size: 17px;"
        " font-weight: bold; padding: 10px; border: 3px inset #888; }"
        "QListWidget { background: #030703; color: #6af26a;"
        " font-family: Consolas, monospace; font-size: 13px;"
        " border: 3px inset #888; }"
        "QListWidget::item:selected { background: #1a7b38; color: white; }"
        "QPushButton { background: #c0c0c0; border: 2px outset #eee;"
        " padding: 5px; min-height: 22px; }"
        "QPushButton:pressed { border: 2px inset #eee; }"
    );
}

void PlayerWindow::addFiles() {
    const QStringList files = QFileDialog::getOpenFileNames(this,
        "Add module files", QString(),
        "Tracker modules (*.mod *.s3m *.xm *.it *.669 *.mtm *.stm *.ult *.far *.okt *.med);;All files (*)");
    for (const auto& file : files) playlist_->addItem(file);
    if (playlist_->currentRow() < 0 && playlist_->count() > 0)
        playlist_->setCurrentRow(0);
    refreshStatus();
}

void PlayerWindow::removeSelected() {
    qDeleteAll(playlist_->selectedItems());
    refreshStatus();
}

void PlayerWindow::moveSelected(int offset) {
    const int index = playlist_->currentRow();
    const int destination = index + offset;
    if (index < 0 || destination < 0 || destination >= playlist_->count()) return;
    auto* item = playlist_->takeItem(index);
    playlist_->insertItem(destination, item);
    playlist_->setCurrentRow(destination);
}

void PlayerWindow::refreshStatus() {
    status_->setText(QString("%1 module(s)  |  playback not yet implemented").arg(playlist_->count()));
}
