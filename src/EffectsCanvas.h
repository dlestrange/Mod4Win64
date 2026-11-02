#pragma once
#include <QWidget>
#include "ModuleAudio.h"
class EffectsCanvas final : public QWidget {
 Q_OBJECT
public:
 explicit EffectsCanvas(QWidget* parent = nullptr);
 QSize sizeHint() const override;
 void setSnapshot(const EffectSnapshot& snapshot);
protected:
 void paintEvent(QPaintEvent*) override;
private:
 EffectSnapshot snapshot_;
 QVector<int> heldNotes_;
 QVector<int> heldInstruments_;
 QString lastModule_;
 int lastPattern_ = -1;
 int lastRow_ = -1;
};
