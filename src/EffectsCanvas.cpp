#include "EffectsCanvas.h"
#include <QPainter>
#include <QImage>
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
QSize EffectsCanvas::logicalSize() const {
 const int channels=std::max(1,snapshot_.channels);
 return QSize(124+14*snapshot_.usedCommands.size(),90+9*channels);
}
QSize EffectsCanvas::sizeHint() const {
 const QSize sz=logicalSize();
 return QSize(qRound(sz.width()*scale_),qRound(sz.height()*scale_));
}
void EffectsCanvas::setScale(double scale) {
 scale_=std::clamp(scale,1.0,3.0);
 updateGeometry();
 update();
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
    // Paint once at the original logical resolution, then enlarge the whole
    // bitmap with nearest-neighbour sampling. No individual font/line scaling.
    const QSize logical=logicalSize();
    QImage pixels(logical,QImage::Format_ARGB32_Premultiplied);
    QPainter p(&pixels);
    p.setRenderHint(QPainter::Antialiasing, false);
    const int w=logical.width(), h=logical.height();
    p.fillRect(rect(), QColor("#c0c0c0"));

    auto frame=[&](const QRect& r, bool sunken) {
        const QColor light("#ffffff"), dark("#666666");
        p.setPen(sunken?dark:light);
        p.drawLine(r.left(),r.top(),r.right(),r.top());
        p.drawLine(r.left(),r.top(),r.left(),r.bottom());
        p.setPen(sunken?light:dark);
        p.drawLine(r.left(),r.bottom(),r.right(),r.bottom());
        p.drawLine(r.right(),r.top(),r.right(),r.bottom());
    };
    frame(QRect(0,0,w-1,h-1),false);

    QFont tiny("Tahoma",7);
    tiny.setStyleStrategy(QFont::NoAntialias);
    p.setFont(tiny);
    p.setPen(QColor("#101010"));

    // All geometry is derived from SOURCE/ana.pas: effects start at x=27,
    // 14 pixels per selected effect; channels start at y=79, 9 pixels apart.
    // The right-hand columns remain a fixed distance from the panel edge.
    constexpr int effectX=29, rowY=79, effectWidth=14, rowHeight=9;
    const int effectsWidth=effectWidth*snapshot_.usedCommands.size();
    frame(QRect(6,6,19,55),true);
    frame(QRect(6,64,19,11),true);
    frame(QRect(27,6,w-120,69),true);
    frame(QRect(w-91,6,83,55),true);
    frame(QRect(w-91,64,83,11),true);

    // Restore dark text after painting the light bevel edges.
    p.setPen(QColor("#101010"));
    p.save();
    p.translate(20,59);
    p.rotate(-90);
    p.drawText(0,0,"Effects");
    p.restore();

    // These names follow common libopenmpt effect command IDs. An exact
    // original-era category mapping remains a separate compatibility pass.
    auto effectName=[](int id)->QString {
        switch(id) {
        case 1: return "Arpeggio";
        case 2: return "Port Up";
        case 3: return "Port Down";
        case 4: return "Port To Note";
        case 5: return "Vibrato";
        case 6: return "Port + VolSlid";
        case 7: return "Vib + VolSlid";
        case 8: return "Tremolo";
        case 9: return "Set Panning";
        case 10: return "Sample Offset";
        case 11: return "Vol Slide";
        case 12: return "Position Jump";
        case 13: return "Pattern Break";
        case 14: return "Extended";
        case 15: return "Set Speed";
        case 16: return "Set GlobalVol";
        case 17: return "GlobalVolSlide";
        case 20: return "Pan Slide";
        default: return QString("Effect %1").arg(id,2,16,QChar('0')).toUpper();
        }
    };
    for(int col=0;col<snapshot_.usedCommands.size();++col) {
        p.save();
        p.setClipRect(QRect(effectX+col*14,8,14,65));
        p.translate(effectX+col*14+10,72);
        p.rotate(-90);
        p.drawText(0,0,effectName(snapshot_.usedCommands[col]));
        p.restore();
    }
    const QString speed=snapshot_.channels?QString::number(snapshot_.speed):"--";
    const QString bpm=snapshot_.channels?QString::number(snapshot_.bpm):"--";
    p.setPen(QColor("#101010"));
    p.drawText(w-88,18,"Speed:");
    p.drawText(w-37,18,speed);
    p.drawText(w-88,31,"BPM:");
    p.drawText(w-37,31,bpm);
    // Tracker rows are zero-based internally; Mod4Win displays 1-based notes.
    p.drawText(w-88,44,"Note:");
    p.drawText(w-37,44,snapshot_.channels?QString::number(snapshot_.row+1):"--");
    p.drawText(w-88,57,"Time:");
    const int seconds=std::max(0,static_cast<int>(snapshot_.elapsedSeconds));
    const QString elapsed=QString("%1:%2").arg(seconds/60).arg(seconds%60,2,10,QChar('0'));
    p.drawText(w-37,57,snapshot_.channels?elapsed:"--:--");
    p.drawText(w-88,73,"Note");
    p.drawText(w-65,73,"L  R");
    p.drawText(w-26,73,"Inst.");

    if(snapshot_.channels>0) {
        const int bodyHeight=snapshot_.channels*rowHeight+4;
        frame(QRect(6,77,19,bodyHeight),true);
        frame(QRect(27,77,w-120,bodyHeight),true);
        frame(QRect(w-91,77,19,bodyHeight),true);
        frame(QRect(w-70,77,41,bodyHeight),true);
        frame(QRect(w-27,77,19,bodyHeight),true);
    }
    for(int ch=0;ch<snapshot_.channels;++ch) {
        const int y=rowY+ch*rowHeight;
        p.setPen(QColor("#111111"));
        p.drawText(QRect(8,y,16,8),Qt::AlignRight,QString::number(ch+1));
        for(int col=0;col<snapshot_.usedCommands.size();++col) {
            const QRect cell(effectX+col*effectWidth,y,13,8);
            p.fillRect(cell,QColor("#062307"));
            const bool active=ch<snapshot_.currentCommands.size() &&
                snapshot_.currentCommands[ch]==snapshot_.usedCommands[col];
            p.fillRect(cell.adjusted(1,1,-1,-2),active?QColor("#4fff50"):QColor("#238b2e"));
        }
        const int note=ch<heldNotes_.size()?heldNotes_[ch]:0;
        const int inst=ch<heldInstruments_.size()?heldInstruments_[ch]:0;
        p.setPen(QColor("#161616"));
        p.drawText(QRect(w-89,y,17,8),Qt::AlignCenter,noteLabel(note));
        const int centre=w-49;
        const auto meter=[&](bool left,float level) {
            constexpr int segments=8, step=2;
            const float safe=std::isfinite(level)?std::clamp(level,0.0f,1.0f):0.0f;
            const int lit=static_cast<int>(std::lround(safe*segments));
            for(int i=0;i<segments;++i) {
                const int x=left?centre-2-(i+1)*step:centre+2+i*step;
                p.fillRect(x,y,step,8,QColor("#080808"));
                if(i<lit) {
                    const QColor colour=i<4?QColor("#28d82d"):
                        (i<6?QColor("#ead72e"):QColor("#ed392c"));
                    p.fillRect(x,y+1,step,6,colour);
                }
            }
        };
        meter(true,ch<snapshot_.vuLeft.size()?snapshot_.vuLeft[ch]:0);
        meter(false,ch<snapshot_.vuRight.size()?snapshot_.vuRight[ch]:0);
        p.fillRect(centre,y,1,8,QColor("#b0b0b0"));
        p.setPen(QColor("#111111"));
        p.drawText(QRect(w-25,y,16,8),Qt::AlignCenter,inst?QString::number(inst):"---");
    }
    p.end();
    QPainter output(this);
    output.setRenderHint(QPainter::SmoothPixmapTransform,false);
    output.drawImage(rect(),pixels);

}
