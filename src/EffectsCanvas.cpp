#include "EffectsCanvas.h"
#include <QPainter>
#include <QImage>
#include <QFont>
#include <QFontDatabase>
#include <QPaintDevice>
#include <array>
#include <algorithm>
#include <cmath>

namespace {
// A small self-contained 5x7 bitmap font. Rendering platform fonts at
// 7-9 pixels produces missing stems/counters on some macOS Retina displays,
// particularly after 150% nearest-neighbour enlargement. Explicit bitmaps
// guarantee identical readable letters for every output scale and OS.
using Glyph = std::array<quint8,7>;
Glyph glyphFor(QChar c) {
    switch (c.toUpper().toLatin1()) {
        case '0': return {0b01110, 0b10011, 0b10101, 0b10101, 0b11001, 0b10001, 0b01110};
        case '1': return {0b00100, 0b01100, 0b00100, 0b00100, 0b00100, 0b00100, 0b01110};
        case '2': return {0b01110, 0b10001, 0b00001, 0b00010, 0b00100, 0b01000, 0b11111};
        case '3': return {0b11110, 0b00001, 0b00001, 0b01110, 0b00001, 0b00001, 0b11110};
        case '4': return {0b00010, 0b00110, 0b01010, 0b10010, 0b11111, 0b00010, 0b00010};
        case '5': return {0b11111, 0b10000, 0b10000, 0b11110, 0b00001, 0b00001, 0b11110};
        case '6': return {0b01111, 0b10000, 0b10000, 0b11110, 0b10001, 0b10001, 0b01110};
        case '7': return {0b11111, 0b00001, 0b00010, 0b00100, 0b01000, 0b01000, 0b01000};
        case '8': return {0b01110, 0b10001, 0b10001, 0b01110, 0b10001, 0b10001, 0b01110};
        case '9': return {0b01110, 0b10001, 0b10001, 0b01111, 0b00001, 0b00001, 0b11110};
        case ' ': return {0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000};
        case 'A': return {0b01110, 0b10001, 0b10001, 0b11111, 0b10001, 0b10001, 0b10001};
        case 'B': return {0b11110, 0b10001, 0b10001, 0b11110, 0b10001, 0b10001, 0b11110};
        case 'C': return {0b01111, 0b10000, 0b10000, 0b10000, 0b10000, 0b10000, 0b01111};
        case 'D': return {0b11110, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b11110};
        case 'E': return {0b11111, 0b10000, 0b10000, 0b11110, 0b10000, 0b10000, 0b11111};
        case 'F': return {0b11111, 0b10000, 0b10000, 0b11110, 0b10000, 0b10000, 0b10000};
        case 'G': return {0b01111, 0b10000, 0b10000, 0b10111, 0b10001, 0b10001, 0b01111};
        case 'H': return {0b10001, 0b10001, 0b10001, 0b11111, 0b10001, 0b10001, 0b10001};
        case 'I': return {0b11111, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100, 0b11111};
        case 'J': return {0b00111, 0b00010, 0b00010, 0b00010, 0b10010, 0b10010, 0b01100};
        case 'K': return {0b10001, 0b10010, 0b10100, 0b11000, 0b10100, 0b10010, 0b10001};
        case 'L': return {0b10000, 0b10000, 0b10000, 0b10000, 0b10000, 0b10000, 0b11111};
        case 'M': return {0b10001, 0b11011, 0b10101, 0b10101, 0b10001, 0b10001, 0b10001};
        case 'N': return {0b10001, 0b11001, 0b10101, 0b10011, 0b10001, 0b10001, 0b10001};
        case 'O': return {0b01110, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b01110};
        case 'P': return {0b11110, 0b10001, 0b10001, 0b11110, 0b10000, 0b10000, 0b10000};
        case 'Q': return {0b01110, 0b10001, 0b10001, 0b10001, 0b10101, 0b10010, 0b01101};
        case 'R': return {0b11110, 0b10001, 0b10001, 0b11110, 0b10100, 0b10010, 0b10001};
        case 'S': return {0b01111, 0b10000, 0b10000, 0b01110, 0b00001, 0b00001, 0b11110};
        case 'T': return {0b11111, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100};
        case 'U': return {0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b01110};
        case 'V': return {0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b01010, 0b00100};
        case 'W': return {0b10001, 0b10001, 0b10001, 0b10101, 0b10101, 0b10101, 0b01010};
        case 'X': return {0b10001, 0b10001, 0b01010, 0b00100, 0b01010, 0b10001, 0b10001};
        case 'Y': return {0b10001, 0b10001, 0b01010, 0b00100, 0b00100, 0b00100, 0b00100};
        case 'Z': return {0b11111, 0b00001, 0b00010, 0b00100, 0b01000, 0b10000, 0b11111};
        case ':': return {0b00000, 0b00100, 0b00100, 0b00000, 0b00100, 0b00100, 0b00000};
        case '.': return {0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b01100, 0b01100};
        case '-': return {0b00000, 0b00000, 0b00000, 0b11111, 0b00000, 0b00000, 0b00000};
        case '+': return {0b00000, 0b00100, 0b00100, 0b11111, 0b00100, 0b00100, 0b00000};
        case '#': return {0b01010, 0b11111, 0b01010, 0b01010, 0b11111, 0b01010, 0b00000};
        case '/': return {0b00001, 0b00001, 0b00010, 0b00100, 0b01000, 0b10000, 0b10000};
        case '&': return {0b01100, 0b10010, 0b10100, 0b01000, 0b10101, 0b10010, 0b01101};
        case '(': return {0b00010, 0b00100, 0b01000, 0b01000, 0b01000, 0b00100, 0b00010};
        case ')': return {0b01000, 0b00100, 0b00010, 0b00010, 0b00010, 0b00100, 0b01000};
        case '%': return {0b11001, 0b11010, 0b00010, 0b00100, 0b01000, 0b01011, 0b10011};
        case '?': return {0b01110, 0b10001, 0b00001, 0b00010, 0b00100, 0b00000, 0b00100};
        default: return {0,0,0,0,0,0,0};
    }
}
int pixelTextWidth(const QString& text) {
    return text.isEmpty()?0:text.size()*6-1;
}
void drawPixelText(QPainter& p,int x,int baseline,const QString& text) {
    const int top=baseline-7;
    const QColor colour=p.pen().color();
    for(const QChar c:text) {
        const Glyph glyph=glyphFor(c);
        for(int row=0;row<7;++row)
            for(int col=0;col<5;++col)
                if(glyph[row] & (1u<<(4-col)))
                    p.fillRect(x+col,top+row,1,1,colour);
        x+=6;
    }
}
void drawPixelText(QPainter& p,const QRect& box,Qt::Alignment alignment,const QString& text) {
    const int width=pixelTextWidth(text);
    const int x=(alignment & Qt::AlignHCenter)?box.left()+(box.width()-width)/2
               :(alignment & Qt::AlignRight)?box.right()-width+1:box.left();
    const int top=(alignment & Qt::AlignVCenter)?box.top()+(box.height()-7)/2
                  :(alignment & Qt::AlignBottom)?box.bottom()-6:box.top();
    drawPixelText(p,x,top+7,text);
}

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
    // Paint every detail at the original 1x pixel resolution, independent
    // of the desktop's DPI and display scale. Enlarge the finished bitmap
    // with nearest-neighbour sampling; never scale individual fonts or lines.
    const QSize logical=logicalSize();
    QImage pixels(logical,QImage::Format_ARGB32_Premultiplied);
    QPainter p(&pixels);
    p.setRenderHint(QPainter::Antialiasing, false);
    const int w=logical.width(), h=logical.height();
    // QWidget::rect() is already scaled; the source bitmap is NOT.
    // Filling in source coordinates avoids inconsistent edges at 150%.
    p.fillRect(pixels.rect(), QColor("#c0c0c0"));

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

    // No operating-system font dependencies in this tiny tracker panel.
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
    drawPixelText(p,0,0,"Effects");
    p.restore();
    // Original Mod4Win labels the small recessed channel-number heading.
    drawPixelText(p,QRect(7,64,17,11),Qt::AlignCenter,"Chn");

    // libopenmpt exposes OpenMPT's internal EffectCommand enum, NOT the
    // original MOD/XM effect nibble. These numeric IDs are defined in
    // OpenMPT/soundlib/modcommand.h. A few commands are containers for
    // different subcommands (notably MOD/XM Extended E effects); splitting
    // those into original Mod4Win subcategories requires scanning parameters.
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
        case 13: return "Set Volume";
        case 14: return "Pattern Break";
        case 15: return "Retrig Note";
        case 16: return "Set Speed";
        case 17: return "Set Tempo";
        case 18: return "Tremor";
        case 19: return "MOD Extended";
        case 20: return "S3M Extended";
        case 21: return "Channel Volume";
        case 22: return "Chn Vol Slide";
        case 23: return "Set GlobalVol";
        case 24: return "GlobalVolSlide";
        case 25: return "Key Off";
        case 26: return "Fine Vibrato";
        case 27: return "Panbrello";
        case 28: return "X-Fine Porta";
        case 29: return "Pan Slide";
        case 30: return "Set Env Pos";
        case 31: return "MIDI";
        case 32: return "Smooth MIDI";
        case 33: return "Delay Cut";
        case 34: return "XParam";
        case 35: return "Fine Tune";
        case 36: return "Smooth Tune";
        default: return QString("Effect %1").arg(id,2,16,QChar('0')).toUpper();
        }
    };
    for(int col=0;col<snapshot_.usedCommands.size();++col) {
        p.save();
        p.setClipRect(QRect(effectX+col*14,8,14,65));
        p.translate(effectX+col*14+10,72);
        p.rotate(-90);
        drawPixelText(p,0,0,effectName(snapshot_.usedCommands[col]));
        p.restore();
    }
    const QString speed=snapshot_.channels?QString::number(snapshot_.speed):"--";
    const QString bpm=snapshot_.channels?QString::number(snapshot_.bpm):"--";
    p.setPen(QColor("#101010"));
    drawPixelText(p,w-88,18,"Speed:");
    drawPixelText(p,w-37,18,speed);
    drawPixelText(p,w-88,31,"BPM:");
    drawPixelText(p,w-37,31,bpm);
    // Tracker rows are zero-based internally; Mod4Win displays 1-based notes.
    drawPixelText(p,w-88,44,"Note:");
    drawPixelText(p,w-37,44,snapshot_.channels?QString::number(snapshot_.row+1):"--");
    drawPixelText(p,w-88,57,"Time:");
    const int seconds=std::max(0,static_cast<int>(snapshot_.elapsedSeconds));
    const QString elapsed=QString("%1:%2").arg(seconds/60).arg(seconds%60,2,10,QChar('0'));
    drawPixelText(p,w-37,57,snapshot_.channels?elapsed:"--:--");
    drawPixelText(p,w-88,73,"Note");
    // Align each heading over its outward-growing half-meter.
    drawPixelText(p,QRect(w-70,64,20,10),Qt::AlignCenter,"L");
    drawPixelText(p,QRect(w-48,64,20,10),Qt::AlignCenter,"R");
    drawPixelText(p,w-26,73,"Inst.");

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
        // Use a baseline rather than squeezing glyphs inside an 8-pixel QRect;
        // Tahoma's font metrics were clipping descenders/digit bottoms.
        drawPixelText(p,8,y+7,QString("%1").arg(ch+1,2,10,QChar(' ')));
        for(int col=0;col<snapshot_.usedCommands.size();++col) {
            const QRect cell(effectX+col*effectWidth,y,13,8);
            // Consistent green inset: avoid the previous cool-coloured strip
            // showing along the bottom of each cell when enlarged.
            p.fillRect(cell,QColor("#0b3b10"));
            const bool active=ch<snapshot_.currentCommands.size() &&
                snapshot_.currentCommands[ch]==snapshot_.usedCommands[col];
            p.fillRect(cell.adjusted(1,1,-1,-1),active?QColor("#4fff50"):QColor("#238b2e"));
        }
        // A channel remains visible while EITHER stereo side has a lit bar.
        // Only hide note + instrument when BOTH sides have gone dark (AND).
        // Use exactly the same eight-segment quantisation as the meters below,
        // so the text cannot linger after the last visual bar disappears.
        const auto barCount=[](float level)->int {
            const float safe=std::isfinite(level)?std::clamp(level,0.0f,1.0f):0.0f;
            return static_cast<int>(std::lround(safe*8.0f));
        };
        const float left=ch<snapshot_.vuLeft.size()?snapshot_.vuLeft[ch]:0.0f;
        const float right=ch<snapshot_.vuRight.size()?snapshot_.vuRight[ch]:0.0f;
        const bool channelActive=(barCount(left)>0 || barCount(right)>0);
        const int note=channelActive && ch<heldNotes_.size()?heldNotes_[ch]:0;
        const int inst=channelActive && ch<heldInstruments_.size()?heldInstruments_[ch]:0;
        p.setPen(QColor("#161616"));
        drawPixelText(p,QRect(w-89,y,17,8),Qt::AlignCenter,noteLabel(note));
        const int centre=w-49;
        const auto meter=[&](bool left,float level) {
            constexpr int segments=8, step=2;
            const int lit=barCount(level);
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
        meter(true,left);
        meter(false,right);
        p.fillRect(centre,y,1,8,QColor("#b0b0b0"));
        p.setPen(QColor("#111111"));
        drawPixelText(p,QRect(w-25,y,16,8),Qt::AlignCenter,inst?QString::number(inst):"---");
    }
    p.end();
    QPainter output(this);
    output.setRenderHint(QPainter::SmoothPixmapTransform,false);
    // Restrict the destination to our exact scaled content size rather than
    // relying on QWidget::rect(), which can differ during a resize/layout
    // event. Qt's nearest-neighbour path preserves the 1x pixel grid even
    // at fractional factors such as 150% and on Retina screens.
    output.drawImage(QRect(QPoint(0,0),sizeHint()),pixels,pixels.rect());

}
