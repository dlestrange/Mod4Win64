#include "EffectsWindow.h"
#include "EffectsCanvas.h"
#include <QVBoxLayout>
#include <QMouseEvent>
#include <QEvent>
#include <QGuiApplication>
#include <QScreen>
#include <QSettings>

EffectsWindow::EffectsWindow(QWidget* parent) : QDialog(parent) {
    setWindowTitle("Mod4Win64 2026 Beta - Effects");
    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint);
    setModal(false);
    setStyleSheet("EffectsWindow { background:#c0c0c0; }");
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0,0,0,0);
    layout->setSpacing(0);
    canvas_ = new EffectsCanvas(this);
    canvas_->installEventFilter(this);
    layout->addWidget(canvas_);
    setFixedSize(canvas_->sizeHint());
    QSettings settings("Mod4Win64","Mod4Win64");
    setScaleMode(settings.value("appearance/effectsScale","auto").toString());
}
void EffectsWindow::setPanelSize(int width, int height) {
    if (width < 1 || height < 1) return;
    canvas_->setFixedSize(width,height);
    setFixedSize(width,height);
}
bool EffectsWindow::handleMouse(QMouseEvent* event) {
    if (event->type() == QEvent::MouseButtonPress && event->button() == Qt::LeftButton) {
        dragging_ = true;
        dragOffset_ = event->globalPosition().toPoint() - frameGeometry().topLeft();
        event->accept();
        return true;
    }
    if (event->type() == QEvent::MouseMove && dragging_ && (event->buttons() & Qt::LeftButton)) {
        move(event->globalPosition().toPoint() - dragOffset_);
        event->accept();
        return true;
    }
    if (event->type() == QEvent::MouseButtonRelease && event->button() == Qt::LeftButton && dragging_) {
        dragging_ = false;
        event->accept();
        return true;
    }
    return false;
}
bool EffectsWindow::eventFilter(QObject* watched, QEvent* event) {
    if (watched == canvas_ &&
        (event->type() == QEvent::MouseButtonPress ||
         event->type() == QEvent::MouseMove ||
         event->type() == QEvent::MouseButtonRelease)) {
        if (handleMouse(static_cast<QMouseEvent*>(event))) return true;
    }
    return QDialog::eventFilter(watched,event);
}
void EffectsWindow::mousePressEvent(QMouseEvent* event) {
    if (!handleMouse(event)) QDialog::mousePressEvent(event);
}
void EffectsWindow::mouseMoveEvent(QMouseEvent* event) {
    if (!handleMouse(event)) QDialog::mouseMoveEvent(event);
}
void EffectsWindow::mouseReleaseEvent(QMouseEvent* event) {
    if (!handleMouse(event)) QDialog::mouseReleaseEvent(event);
}

void EffectsWindow::setSnapshot(const EffectSnapshot& snapshot) {
    canvas_->setSnapshot(snapshot);
    const QSize newSize=canvas_->sizeHint();
    if(newSize!=size()) setPanelSize(newSize.width(),newSize.height());
}

void EffectsWindow::setScaleMode(const QString& mode) {
    double scale=1.0;
    if(mode=="150") scale=1.5;
    else if(mode=="200") scale=2.0;
    else if(mode=="300") scale=3.0;
    else if(mode=="auto") {
        const QScreen* screen=QGuiApplication::screenAt(frameGeometry().center());
        if(!screen) screen=QGuiApplication::primaryScreen();
        // Qt uses device-independent pixels: base the enlargement on the
        // usable logical desktop width rather than multiplying DPR again.
        const int screenWidth=screen?screen->availableGeometry().width():1280;
        scale=screenWidth>=2200?2.0:(screenWidth>=1600?1.5:1.0);
    }
    canvas_->setScale(scale);
    const QSize size=canvas_->sizeHint();
    setPanelSize(size.width(),size.height());
}
