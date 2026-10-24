#include "EffectsCanvas.h"
#include <QPainter>
#include <QFont>

EffectsCanvas::EffectsCanvas(QWidget* parent) : QWidget(parent) {
    setMinimumSize(250, 170);
}
void EffectsCanvas::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.fillRect(rect(), QColor("#c0c0c0"));
    p.setRenderHint(QPainter::Antialiasing, false);
    p.save();
    p.scale(width()/250.0, height()/170.0);
    p.setPen(QColor("#111111"));
    QFont font("Tahoma", 8); p.setFont(font);
    p.drawText(7, 15, "Effects");
    p.drawText(194, 16, "Speed: --");
    p.drawText(194, 28, "BPM: --");
    p.drawText(194, 40, "Note: --");
    p.drawText(194, 52, "Time: --");
    // The channel grid uses original 9px rows / 14px effect column proportions.
    constexpr int x0=26, y0=68, cellW=14, cellH=9;
    const QString headers[]={"Arp", "Port", "Vib", "Vol", "Pan", "Brk", "Rtg", "Sld"};
    for (int col=0; col<8; ++col) {
        p.save();
        p.translate(x0+col*cellW+10,62);
        p.rotate(-90);
        p.drawText(0,0,headers[col]);
        p.restore();
    }
    for(int ch=0;ch<8;++ch) {
        p.setPen(QColor("#161616"));
        p.drawText(7,y0+ch*cellH+7,QString::number(ch+1));
        for(int col=0;col<8;++col) {
            const QRect cell(x0+col*cellW,y0+ch*cellH,13,8);
            p.fillRect(cell,QColor("#042507"));
            p.fillRect(cell.adjusted(1,2,-1,-3),QColor("#22672e"));
        }
    }
    p.restore();
}
