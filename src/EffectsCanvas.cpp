#include "EffectsCanvas.h"
#include <QPainter>
#include <QFont>
#include <algorithm>
EffectsCanvas::EffectsCanvas(QWidget* parent) : QWidget(parent) {}
QSize EffectsCanvas::sizeHint() const {
 const int channels = std::max(1, snapshot_.channels);
 return QSize(124 + 14 * snapshot_.usedCommands.size(), 90 + 9 * channels);
}
void EffectsCanvas::setSnapshot(const EffectSnapshot& snapshot) {
 snapshot_=snapshot;
 update();
}
void EffectsCanvas::paintEvent(QPaintEvent*) {
 QPainter p(this);
 p.fillRect(rect(), QColor("#c0c0c0"));
 p.setRenderHint(QPainter::Antialiasing,false);
 p.setPen(QColor("#111111"));
 QFont font("Tahoma",8); p.setFont(font);
 p.drawText(7,15,"Effects");
 const int statusX=width()-92;
 p.drawText(statusX,15,QString("Speed: %1").arg(snapshot_.channels ? QString::number(snapshot_.speed) : "--"));
 p.drawText(statusX,27,QString("BPM: %1").arg(snapshot_.channels ? QString::number(snapshot_.bpm) : "--"));
 p.drawText(statusX,39,QString("Pat: %1").arg(snapshot_.channels ? QString::number(snapshot_.pattern) : "--"));
 p.drawText(statusX,51,QString("Row: %1").arg(snapshot_.channels ? QString::number(snapshot_.row) : "--"));
 constexpr int x0=25, y0=69, cellW=14, cellH=9;
 for(int col=0;col<snapshot_.usedCommands.size();++col) {
    const QString label=QString::number(snapshot_.usedCommands[col],16).toUpper().rightJustified(2,'0');
    p.save();
    p.translate(x0+col*cellW+10,62);
    p.rotate(-90);p.drawText(0,0,label);p.restore();
 }
 for(int ch=0;ch<snapshot_.channels;++ch) {
    p.setPen(QColor("#161616"));
    p.drawText(5,y0+ch*cellH+7,QString::number(ch+1));
    for(int col=0;col<snapshot_.usedCommands.size();++col) {
       const QRect cell(x0+col*cellW,y0+ch*cellH,13,8);
       p.fillRect(cell,QColor("#042507"));
       const bool active=ch<snapshot_.currentCommands.size() &&
          snapshot_.currentCommands[ch]==snapshot_.usedCommands[col];
       p.fillRect(cell.adjusted(1,2,-1,-3),active ? QColor("#4dff54") : QColor("#22672e"));
    }
 }
}
