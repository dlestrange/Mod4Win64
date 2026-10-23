#pragma once
#include <QWidget>
class EffectsCanvas final : public QWidget {
 Q_OBJECT
public:
 explicit EffectsCanvas(QWidget* parent = nullptr);
 QSize sizeHint() const override { return {250, 170}; }
protected:
 void paintEvent(QPaintEvent*) override;
};
