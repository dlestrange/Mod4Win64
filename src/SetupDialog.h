#pragma once
#include <QDialog>
class QLabel;
class QCheckBox;
class SetupDialog final : public QDialog {
    Q_OBJECT
public:
    explicit SetupDialog(QWidget* parent = nullptr);
};
