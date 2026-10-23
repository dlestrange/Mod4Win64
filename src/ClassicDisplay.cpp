#include "ClassicDisplay.h"
#include <QPainter>
#include <QFont>
#include <QPaintEvent>
#include <QtGlobal>

ClassicDisplay::ClassicDisplay(QWidget* parent) : QWidget(parent) {
    setMinimumSize(350, 96);
}
void ClassicDisplay::setTitle(const QString& title) { title_ = title; update(); }
void ClassicDisplay::setState(const QString& state) { state_ = state; update(); }
void ClassicDisplay::setDiagnostics(quint64 frames, quint64 nonzero) {
    frames_ = frames; nonzero_ = nonzero; update();
}
void ClassicDisplay::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, false);
    // Paint at the original-era logical resolution, scaling up as a single canvas.
    p.scale(width() / 626.0, height() / 96.0);
    auto bevel = [&](const QRect& r) {
        p.fillRect(r, QColor("#050a05"));
        p.setPen(QColor("#ffffff")); p.drawLine(r.topLeft(), r.topRight());
        p.drawLine(r.topLeft(), r.bottomLeft());
        p.setPen(QColor("#707070")); p.drawLine(r.bottomLeft(), r.bottomRight());
        p.drawLine(r.topRight(), r.bottomRight());
    };
    const QColor green("#3dff42");
    p.fillRect(QRect(0, 0, 626, 96), QColor("#c0c0c0"));
    bevel({2, 2, 406, 39});
    bevel({417, 2, 156, 94});
    bevel({2, 46, 251, 48});
    bevel({257, 46, 151, 48});
    bevel({577, 2, 47, 94});
    p.setPen(green);
    QFont matrix("Consolas", 17, QFont::Bold);
    matrix.setStyleStrategy(QFont::NoAntialias);
    p.setFont(matrix);
    p.drawText(QRect(8, 4, 394, 34), Qt::AlignCenter, title_.left(27).toUpper());
    QFont small("Consolas", 10, QFont::Bold);
    small.setStyleStrategy(QFont::NoAntialias);
    p.setFont(small);
    p.drawText(12, 60, "MIN  SEC     Song     Total");
    p.drawText(12, 83, "--:--       --  of   --");
    p.drawText(264, 60, "Pattern");
    p.drawText(264, 82, "-- of --");
    p.drawText(424, 24, "48kHz 16Bit");
    p.drawText(424, 42, "STEREO");
    p.drawText(424, 61, state_.left(16).toUpper());
    p.drawText(424, 80, nonzero_ ? "PCM ACTIVE" : "PCM IDLE");
    // The PUMP column is deliberately graphic, not dependent on an installed font.
    const QString pump = "PUMP";
    const QColor pumpColors[] = {QColor("#f34221"),QColor("#dfe323"),QColor("#44df4e"),QColor("#e2333a")};
    QFont pumpFont("Arial", 12, QFont::Black);
    p.setFont(pumpFont);
    for (int i=0; i<4; ++i) {
        p.setPen(pumpColors[i]);
        p.drawText(583, 19+i*16, QString(pump[i]));
    }
    p.setPen(green);
    p.setFont(small);
    p.drawText(582, 88, "-0dB");
}
