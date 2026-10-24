#include "EffectsWindow.h"
#include "EffectsCanvas.h"
#include <QVBoxLayout>
#include <QMouseEvent>
#include <QEvent>
#include <QGuiApplication>
#include <QScreen>

EffectsWindow::EffectsWindow(QWidget* parent) : QDialog(parent) {
    setWindowTitle("MOD4WIN [2.40] Effects");
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
