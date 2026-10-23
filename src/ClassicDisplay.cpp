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
    p.drawText(424, 18, "48kHz 16Bit");
    p.drawText(424, 35, "STEREO");
    p.drawText(424, 52, "CH: -- BPM: ---");
    p.drawText(424, 68, state_.left(16).toUpper());
    p.drawText(424, 83, nonzero_ ? "AUDIO ACTIVE" : "AUDIO IDLE");
    // PREAMP is a user-adjustable gain control in the original.
    // Graphic and 0 dB label only for now; do not imply gain is implemented.
    const QString preamp = "PREAMP";
    const QColor preampColors[] = {
        QColor("#ff3a23"), QColor("#efbd26"), QColor("#45e354"),
        QColor("#45e354"), QColor("#efbd26"), QColor("#e63537") };
    QFont ampFont("Arial", 9, QFont::Black);
    p.setFont(ampFont);
    for (int i = 0; i < preamp.size(); ++i) {
        p.setPen(preampColors[i]);
        p.drawText(584, 14 + i * 11, QString(preamp[i]));
    }
    p.setPen(green);
    p.setFont(small);
    p.drawText(583, 89, "0dB");
}
