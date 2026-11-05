#pragma once
#include <QDialog>
class QComboBox;
class SetupDialog final : public QDialog {
    Q_OBJECT
public:
    explicit SetupDialog(QWidget* parent = nullptr);
    void setOutputMode(int mode);
signals:
    void effectsScaleChanged(const QString& mode);
    void outputModeChanged(int mode);
    void interpolationChanged(int taps);
    void stereoSeparationChanged(int percent);
    void rampStrengthChanged(int strength);
    void defaultPreampChanged(int db);
private:
    QComboBox* outputMode_ = nullptr;
};
