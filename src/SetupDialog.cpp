#include "SetupDialog.h"
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QPushButton>
#include <QComboBox>
#include <QSettings>
#include <QSlider>
#include <QSpinBox>
#include <QSignalBlocker>
#include <algorithm>

void SetupDialog::setOutputMode(int mode) {
    if(!outputMode_) return;
    const int index=outputMode_->findData(std::clamp(mode,0,2));
    if(index>=0) {
        const QSignalBlocker blocker(outputMode_);
        outputMode_->setCurrentIndex(index);
    }
}

SetupDialog::SetupDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle("MOD4WIN64 - Setup");
    setMinimumWidth(480);
    setStyleSheet(
        "QDialog,QGroupBox,QLabel {background:#c0c0c0;color:#151515;}"
        "QGroupBox {border:2px groove #dedede;margin-top:12px;padding:9px;}"
        "QGroupBox::title {subcontrol-origin:margin;left:8px;}");
    QSettings settings("Mod4Win64","Mod4Win64");
    auto* layout=new QVBoxLayout(this);
    auto* current=new QGroupBox("Current playback engine",this);
    auto* form=new QFormLayout(current);
    form->addRow("Output:",new QLabel("Qt Multimedia / system default audio device",current));
    form->addRow("Format:",new QLabel("48 kHz, stereo output, signed 16-bit PCM",current));
    form->addRow("Decoder:",new QLabel("libopenmpt",current));
    layout->addWidget(current);

    auto* classic=new QGroupBox("Classic playback options",this);
    auto* options=new QFormLayout(classic);
    outputMode_=new QComboBox(classic);
    outputMode_->addItem("Mono",0);
    outputMode_->addItem("Stereo",1);
    outputMode_->addItem("Surround (classic-style spatial)",2);
    setOutputMode(settings.value("audio/outputMode",1).toInt());
    options->addRow("Output mode:",outputMode_);
    connect(outputMode_,&QComboBox::currentIndexChanged,this,[this](int i) {
        const int mode=outputMode_->itemData(i).toInt();
        QSettings("Mod4Win64","Mod4Win64").setValue("audio/outputMode",mode);
        emit outputModeChanged(mode);
    });

    auto* interpolation=new QComboBox(classic);
    interpolation->addItem("OpenMPT Default (recommended)",0);
    interpolation->addItem("Linear",2);
    interpolation->addItem("Cubic",4);
    interpolation->addItem("Sinc (8-tap)",8);
    const int interpolationIndex=interpolation->findData(
        settings.value("audio/interpolation",0).toInt());
    interpolation->setCurrentIndex(interpolationIndex<0?0:interpolationIndex);
    options->addRow("Interpolation:",interpolation);
    connect(interpolation,&QComboBox::currentIndexChanged,this,
        [this,interpolation](int i) {
            const int taps=interpolation->itemData(i).toInt();
            QSettings("Mod4Win64","Mod4Win64").setValue("audio/interpolation",taps);
            emit interpolationChanged(taps);
        });

    auto* stereoRow=new QWidget(classic);
    auto* stereoLayout=new QHBoxLayout(stereoRow);
    stereoLayout->setContentsMargins(0,0,0,0);
    auto* stereoSlider=new QSlider(Qt::Horizontal,stereoRow);
    stereoSlider->setRange(0,200);
    stereoSlider->setSingleStep(10);
    stereoSlider->setPageStep(20);
    auto* stereoValue=new QLabel(stereoRow);
    stereoValue->setMinimumWidth(45);
    const int separation=std::clamp(
        settings.value("audio/stereoSeparation",100).toInt(),0,200);
    stereoSlider->setValue(separation);
    stereoValue->setText(QString("%1%").arg(separation));
    stereoLayout->addWidget(stereoSlider,1);
    stereoLayout->addWidget(stereoValue);
    options->addRow("Stereo separation:",stereoRow);
    connect(stereoSlider,&QSlider::valueChanged,this,
        [this,stereoValue](int percent) {
            stereoValue->setText(QString("%1%").arg(percent));
            QSettings("Mod4Win64","Mod4Win64")
                .setValue("audio/stereoSeparation",percent);
            emit stereoSeparationChanged(percent);
        });

    auto* ramping=new QComboBox(classic);
    ramping->addItem("OpenMPT Default (recommended)",-1);
    ramping->addItem("Off (may cause clicks)",0);
    for(int strength=1;strength<=10;++strength)
        ramping->addItem(QString("Strength %1").arg(strength),strength);
    const int rampIndex=ramping->findData(
        settings.value("audio/rampStrength",-1).toInt());
    ramping->setCurrentIndex(rampIndex<0?0:rampIndex);
    options->addRow("Volume ramping:",ramping);
    connect(ramping,&QComboBox::currentIndexChanged,this,
        [this,ramping](int i) {
            const int strength=ramping->itemData(i).toInt();
            QSettings("Mod4Win64","Mod4Win64")
                .setValue("audio/rampStrength",strength);
            emit rampStrengthChanged(strength);
        });

    auto* preamp=new QSpinBox(classic);
    preamp->setRange(-20,20);
    preamp->setSingleStep(1);
    preamp->setSuffix(" dB");
    preamp->setValue(std::clamp(
        settings.value("audio/defaultPreampDb",0).toInt(),-20,20));
    options->addRow("Default PREAMP:",preamp);
    connect(preamp,&QSpinBox::valueChanged,this,[this](int db) {
        QSettings("Mod4Win64","Mod4Win64")
            .setValue("audio/defaultPreampDb",db);
        emit defaultPreampChanged(db);
    });
    options->addRow(new QLabel(
        "The main player's PREAMP remains the live gain control.",classic));
    layout->addWidget(classic);

    auto* appearance=new QGroupBox("Appearance",this);
    auto* appearanceLayout=new QFormLayout(appearance);
    auto* effectScale=new QComboBox(appearance);
    effectScale->addItem("Auto (based on screen size)","auto");
    effectScale->addItem("100% (original size)","100");
    effectScale->addItem("150%","150");
    effectScale->addItem("200%","200");
    effectScale->addItem("300%","300");
    const int scaleIndex=effectScale->findData(
        settings.value("appearance/effectsScale","auto").toString());
    effectScale->setCurrentIndex(scaleIndex>=0?scaleIndex:0);
    appearanceLayout->addRow("Effects panel scale:",effectScale);
    layout->addWidget(appearance);
    connect(effectScale,&QComboBox::currentIndexChanged,this,
        [this,effectScale](int i) {
            const QString mode=effectScale->itemData(i).toString();
            QSettings("Mod4Win64","Mod4Win64")
                .setValue("appearance/effectsScale",mode);
            emit effectsScaleChanged(mode);
        });

    auto* buttons=new QDialogButtonBox(QDialogButtonBox::Close,this);
    connect(buttons,&QDialogButtonBox::rejected,this,&QDialog::reject);
    layout->addWidget(buttons);
}
