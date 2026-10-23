#include "SetupDialog.h"
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QLabel>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QCheckBox>
#include <QPushButton>
#include <QMessageBox>

SetupDialog::SetupDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle("MOD4WIN64 - Setup");
    setMinimumWidth(450);
    setStyleSheet("QDialog,QGroupBox,QLabel,QCheckBox { background:#c0c0c0; color:#151515; }"
                  "QGroupBox { border:2px groove #dedede; margin-top:12px; padding:9px; }"
                  "QGroupBox::title { subcontrol-origin:margin; left:8px; }");
    auto* layout = new QVBoxLayout(this);
    auto* current = new QGroupBox("Current playback engine", this);
    auto* form = new QFormLayout(current);
    form->addRow("Output:", new QLabel("Qt Multimedia / system default audio device", current));
    form->addRow("Format:", new QLabel("48 kHz, stereo, signed 16-bit PCM", current));
    form->addRow("Decoder:", new QLabel("libopenmpt", current));
    layout->addWidget(current);

    auto* upcoming = new QGroupBox("Classic playback options (not yet implemented)", this);
    auto* options = new QVBoxLayout(upcoming);
    auto* surround = new QCheckBox("Original Mod4Win surround processing", upcoming);
    auto* ido = new QCheckBox("IDO / cubic interpolation modes", upcoming);
    auto* preamp = new QCheckBox("Adjustable PREAMP gain", upcoming);
    for (auto* control : {surround, ido, preamp}) {
        control->setEnabled(false);
        options->addWidget(control);
    }
    options->addWidget(new QLabel("These options will be enabled only after the DSP is implemented and tested.", upcoming));
    layout->addWidget(upcoming);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);
}
