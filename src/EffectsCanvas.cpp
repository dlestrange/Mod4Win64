#include "EffectsCanvas.h"
#include <QPainter>
#include <QFont>
#include <algorithm>
#include <cmath>

namespace {
QString noteLabel(int note) {
    if (note < 1 || note > 120) return "---";
    static const char* names[]={"C-","C#","D-","D#","E-","F-","F#","G-","G#","A-","A#","B-"};
    const int n=note-1;
    return QString("%1%2").arg(names[n%12]).arg(n/12);
}
}
EffectsCanvas::EffectsCanvas(QWidget* parent) : QWidget(parent) {}
QSize EffectsCanvas::sizeHint() const {
    const int channels=std::max(1,snapshot_.channels);
    return QSize(124+14*snapshot_.usedCommands.size(), 90+9*channels);
}
void EffectsCanvas::setSnapshot(const EffectSnapshot& snapshot) {
    const bool newModule=(snapshot.moduleIdentity!=lastModule_ || heldNotes_.size()!=snapshot.channels);
    if(newModule) {
        heldNotes_.fill(0,snapshot.channels);
        heldInstruments_.fill(0,snapshot.channels);
        lastModule_=snapshot.moduleIdentity;
        lastPattern_=-1;
        lastRow_=-1;
    }
    // Remember pattern events across empty rows. These are last-seen values,
    // not a claim about voices currently sounding in the mixing engine.
    if(snapshot.channels>0 && (newModule || snapshot.pattern!=lastPattern_ || snapshot.row!=lastRow_)) {
        for(int ch=0;ch<snapshot.channels;++ch) {
            if(ch<snapshot.notes.size() && snapshot.notes[ch]>0 && snapshot.notes[ch]<=120)
                heldNotes_[ch]=snapshot.notes[ch];
            if(ch<snapshot.instruments.size() && snapshot.instruments[ch]>0)
                heldInstruments_[ch]=snapshot.instruments[ch];
        }
        lastPattern_=snapshot.pattern;
        lastRow_=snapshot.row;
    }
    snapshot_=snapshot;
    update();
}
void EffectsCanvas::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing,false);
    p.fillRect(rect(),QColor("#c0c0c0"));
    p.setPen(QColor("#ffffff"));
    p.drawLine(0,0,width()-1,0);p.drawLine(0,0,0,height()-1);
    p.setPen(QColor("#666666"));
    p.drawLine(0,height()-1,width()-1,height()-1);
    p.drawLine(width()-1,0,width()-1,height()-1);
    QFont font("Tahoma",7);font.setStyleStrategy(QFont::NoAntialias);p.setFont(font);
    p.setPen(QColor("#111111"));
    p.drawText(6,14,"Effects");
    const int statusX=width()-94;
    p.drawText(statusX,14,QString("Speed %1").arg(snapshot_.channels ? QString::number(snapshot_.speed) : "--"));
    p.drawText(statusX,26,QString("BPM %1").arg(snapshot_.channels ? QString::number(snapshot_.bpm) : "--"));
    p.drawText(statusX,38,QString("Pat %1").arg(snapshot_.channels ? QString::number(snapshot_.pattern) : "--"));
    p.drawText(statusX,50,QString("Row %1").arg(snapshot_.channels ? QString::number(snapshot_.row) : "--"));
    constexpr int x0=25,y0=69,cellW=14,cellH=9;
    const int sideX=x0+cellW*snapshot_.usedCommands.size()+3;
    // Row legend: per-row event note/instrument, live L/R output levels.
    p.drawText(sideX,63,"Note  L  R Ins");
    for(int col=0;col<snapshot_.usedCommands.size();++col) {
        const QString label=QString::number(snapshot_.usedCommands[col],16).toUpper().rightJustified(2,'0');
        p.save();p.translate(x0+col*cellW+10,62);p.rotate(-90);
        p.drawText(0,0,label);p.restore();
    }
    for(int ch=0;ch<snapshot_.channels;++ch) {
        const int y=y0+ch*cellH;
        p.setPen(QColor("#101010"));
        p.drawText(4,y+7,QString::number(ch+1));
        for(int col=0;col<snapshot_.usedCommands.size();++col) {
            const QRect cell(x0+col*cellW,y,13,8);
            p.fillRect(cell,QColor("#042507"));
            const bool active=ch<snapshot_.currentCommands.size() &&
                snapshot_.currentCommands[ch]==snapshot_.usedCommands[col];
            p.fillRect(cell.adjusted(1,2,-1,-3),active ? QColor("#4dff54") : QColor("#22672e"));
        }
        const int note=ch<heldNotes_.size()?heldNotes_[ch]:0;
        const int inst=ch<heldInstruments_.size()?heldInstruments_[ch]:0;
        p.setPen(QColor("#111111"));
        p.drawText(sideX,y+7,noteLabel(note));
        const auto meter=[&](int x,float val) {
            const QRect cell(x,y+1,12,7);
            p.fillRect(cell,QColor("#082109"));
            const int length=static_cast<int>(std::clamp(val,0.0f,1.0f)*11.0f);
            if(length>0) p.fillRect(x,y+2,length,5,QColor("#43ee4d"));
        };
        meter(sideX+30,ch<snapshot_.vuLeft.size()?snapshot_.vuLeft[ch]:0);
        meter(sideX+45,ch<snapshot_.vuRight.size()?snapshot_.vuRight[ch]:0);
        p.setPen(QColor("#101010"));
        p.drawText(sideX+62,y+7,inst?QString::number(inst):"--");
    }
}
