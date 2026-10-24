#pragma once
#include <QDialog>
#include <QPoint>
#include "ModuleAudio.h"
class EffectsCanvas;
class EffectsWindow final : public QDialog {
    Q_OBJECT
public:
    explicit EffectsWindow(QWidget* parent = nullptr);
    void setPanelSize(int width, int height);
 void setSnapshot(const EffectSnapshot& snapshot);
protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
private:
    bool handleMouse(QMouseEvent* event);
    EffectsCanvas* canvas_ = nullptr;
    bool dragging_ = false;
    QPoint dragOffset_;
};
