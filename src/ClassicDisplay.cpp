#include "ClassicDisplay.h"
#include <QPainter>
#include <QImage>
#include <QFont>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QSettings>
#include <QPolygon>
#include <QHash>
#include <QtGlobal>
#include <algorithm>
#include <cmath>

namespace {
constexpr int W=626, H=96;
const QColor lit("#40ff46"), dim("#103718"), dark("#030a04");

// 5x7 dot-matrix alphabet. Each bit is a physical illuminated pixel.
// Keep the dot drawing independent of host fonts and device DPI.
const QHash<QChar,QVector<QString>>& font5x7() {
    static const QHash<QChar,QVector<QString>> glyphs = {
        {'A',{"01110","10001","10001","11111","10001","10001","10001"}},
        {'B',{"11110","10001","10001","11110","10001","10001","11110"}},
        {'C',{"01111","10000","10000","10000","10000","10000","01111"}},
        {'D',{"11110","10001","10001","10001","10001","10001","11110"}},
        {'E',{"11111","10000","10000","11110","10000","10000","11111"}},
        {'F',{"11111","10000","10000","11110","10000","10000","10000"}},
        {'G',{"01111","10000","10000","10111","10001","10001","01111"}},
        {'H',{"10001","10001","10001","11111","10001","10001","10001"}},
        {'I',{"11111","00100","00100","00100","00100","00100","11111"}},
        {'J',{"00111","00010","00010","00010","10010","10010","01100"}},
        {'K',{"10001","10010","10100","11000","10100","10010","10001"}},
        {'L',{"10000","10000","10000","10000","10000","10000","11111"}},
        {'M',{"10001","11011","10101","10101","10001","10001","10001"}},
        {'N',{"10001","11001","10101","10011","10001","10001","10001"}},
        {'O',{"01110","10001","10001","10001","10001","10001","01110"}},
        {'P',{"11110","10001","10001","11110","10000","10000","10000"}},
        {'Q',{"01110","10001","10001","10001","10101","10010","01101"}},
        {'R',{"11110","10001","10001","11110","10100","10010","10001"}},
        {'S',{"01111","10000","10000","01110","00001","00001","11110"}},
        {'T',{"11111","00100","00100","00100","00100","00100","00100"}},
        {'U',{"10001","10001","10001","10001","10001","10001","01110"}},
        {'V',{"10001","10001","10001","10001","10001","01010","00100"}},
        {'W',{"10001","10001","10001","10101","10101","10101","01010"}},
        {'X',{"10001","10001","01010","00100","01010","10001","10001"}},
        {'Y',{"10001","10001","01010","00100","00100","00100","00100"}},
        {'Z',{"11111","00001","00010","00100","01000","10000","11111"}},
        {'0',{"01110","10011","10101","10101","11001","10001","01110"}},
        {'1',{"00100","01100","00100","00100","00100","00100","01110"}},
        {'2',{"01110","10001","00001","00010","00100","01000","11111"}},
        {'3',{"11110","00001","00001","01110","00001","00001","11110"}},
        {'4',{"00010","00110","01010","10010","11111","00010","00010"}},
        {'5',{"11111","10000","10000","11110","00001","00001","11110"}},
        {'6',{"01110","10000","10000","11110","10001","10001","01110"}},
        {'7',{"11111","00001","00010","00100","01000","01000","01000"}},
        {'8',{"01110","10001","10001","01110","10001","10001","01110"}},
        {'9',{"01110","10001","10001","01111","00001","00001","01110"}},
        {'-',{"00000","00000","00000","11111","00000","00000","00000"}},
        {'.',{"00000","00000","00000","00000","00000","01100","01100"}},
        {'!',{"00100","00100","00100","00100","00100","00000","00100"}},
        {' ',{"00000","00000","00000","00000","00000","00000","00000"}},
        {'/',{"00001","00001","00010","00100","01000","10000","10000"}},
        {'_',{"00000","00000","00000","00000","00000","00000","11111"}}
    };
    return glyphs;
}

void drawMatrix(QPainter& p,const QRect& area,const QString& value) {
    p.save();
    p.setClipRect(area);
    p.fillRect(area,dark);
    // Unlit diodes stay visible even where there is no text.
    for(int y=area.top()+2;y<area.bottom()-1;y+=3)
        for(int x=area.left()+2;x<area.right()-1;x+=3)
            p.fillRect(x,y,2,2,dim);
    const int pitch=18, width=5*3-1, height=7*3-1;
    const int maxChars=std::max(1,(area.width()-8)/pitch);
    const QString str=value.toUpper().left(maxChars);
    const int x0=area.x()+std::max(2,(area.width()-int(str.size())*pitch)/2);
    const int y0=area.y()+(area.height()-height)/2;
    const auto& glyphs=font5x7();
    for(int k=0;k<str.size();++k) {
        const auto it=glyphs.constFind(str.at(k));
        if(it==glyphs.constEnd()) continue;
        const auto& rows=it.value();
        for(int r=0;r<7;++r)
            for(int c=0;c<5;++c)
                if(rows[r][c]==QLatin1Char('1'))
                    p.fillRect(x0+k*pitch+c*3,y0+r*3,2,2,lit);
    }
    p.restore();
}

// A deliberately chunky seven-segment LED digit: lit and ghost segments.
void digit(QPainter& p,int x,int y,QChar value,int scale=1) {
    auto segment=[&](int n,bool on) {
        static const QRect rects[7]={
            {3,0,9,2},{12,2,2,10},{12,13,2,10},
            {3,23,9,2},{1,13,2,10},{1,2,2,10},{3,11,9,2}
        };
        const QRect r=rects[n];
        p.fillRect(QRect(x+r.x()*scale,y+r.y()*scale,
                         r.width()*scale,r.height()*scale),on?lit:dim);
    };
    int bits=0;
    switch(value.toLatin1()) {
    case '0':bits=0x3f;break;
    case '1':bits=0x06;break;
    case '2':bits=0x5b;break;
    case '3':bits=0x4f;break;
    case '4':bits=0x66;break;
    case '5':bits=0x6d;break;
    case '6':bits=0x7d;break;
    case '7':bits=0x07;break;
    case '8':bits=0x7f;break;
    case '9':bits=0x6f;break;
    case '-':bits=0x40;break;
    default:break;
    }
    // segment order: top, upper right, lower right, bottom, lower left,
    // upper left, middle (standard 7-bit encoding).
    for(int n=0;n<7;++n)segment(n,(bits & (1<<n))!=0);
}
void digits(QPainter& p,int x,int y,QString value,int count,int scale=1) {
    value=value.rightJustified(count,QLatin1Char(' ')).right(count);
    for(int i=0;i<count;++i)digit(p,x+i*17*scale,y,value.at(i),scale);
}
void bevel(QPainter& p,const QRect& r) {
    p.fillRect(r,dark);
    p.setPen(QColor("#ffffff"));
    p.drawLine(r.topLeft(),r.topRight());
    p.drawLine(r.topLeft(),r.bottomLeft());
    p.setPen(QColor("#707070"));
    p.drawLine(r.bottomLeft(),r.bottomRight());
    p.drawLine(r.topRight(),r.bottomRight());
}
}
ClassicDisplay::ClassicDisplay(QWidget* parent) : QWidget(parent) {
    setMinimumSize(350,96);
    showFilename_=QSettings("Mod4Win64","Mod4Win64")
        .value("appearance/showModuleFilename",false).toBool();
}
void ClassicDisplay::setTitle(const QString& title) { title_=title; update(); }
void ClassicDisplay::setModuleNames(const QString& title,const QString& filename) {
    title_=title;
    filename_=filename;
    update();
}
void ClassicDisplay::mousePressEvent(QMouseEvent* event) {
    // Mouse position maps back to the 626x96 logical LCD coordinate space.
    const QPointF logical(event->position().x()*W/width(),
                          event->position().y()*H/height());
    if(event->button()==Qt::LeftButton && QRectF(383,2,25,40).contains(logical)) {
        showFilename_=!showFilename_;
        QSettings("Mod4Win64","Mod4Win64")
            .setValue("appearance/showModuleFilename",showFilename_);
        setToolTip(showFilename_?"Displaying module filename":"Displaying module title");
        update();
        event->accept();
        return;
    }
    if(event->button()==Qt::LeftButton &&
       QRectF(577,2,47,94).contains(logical)) {
        draggingPreamp_=true;
        setPreampFromY(event->position().y());
        event->accept();
        return;
    }
    QWidget::mousePressEvent(event);
}
void ClassicDisplay::setPreampFromY(qreal widgetY) {
    // Forty steps over the full travel, +20 dB at the top, -20 dB at
    // the bottom. The control is visual-only until the gain stage exists.
    const qreal y=widgetY*H/height();
    const int step=std::clamp(int(std::lround((y-9.0)*40.0/61.0)),0,40);
    const int db=20-step;
    if(preampDb_!=db) {
        preampDb_=db;
        update();
    }
}
void ClassicDisplay::mouseMoveEvent(QMouseEvent* event) {
    if(draggingPreamp_) {
        setPreampFromY(event->position().y());
        event->accept();
        return;
    }
    QWidget::mouseMoveEvent(event);
}
void ClassicDisplay::mouseReleaseEvent(QMouseEvent* event) {
    if(draggingPreamp_ && event->button()==Qt::LeftButton) {
        draggingPreamp_=false;
        event->accept();
        return;
    }
    QWidget::mouseReleaseEvent(event);
}
void ClassicDisplay::setState(const QString& state) { state_=state; update(); }
void ClassicDisplay::setDiagnostics(quint64 frames,quint64 nonzero) {
    frames_=frames; nonzero_=nonzero; update();
}
void ClassicDisplay::setPlaybackCounters(double seconds,int songIndex,
                                         int songTotal,int orderIndex,int orderTotal) {
    // Redraw only when the visible whole-second or sequence numbers change.
    const int oldSecond=int(std::max(0.0,elapsedSeconds_));
    const int newSecond=int(std::max(0.0,seconds));
    const bool changed=oldSecond!=newSecond || songIndex_!=songIndex ||
        songTotal_!=songTotal || orderIndex_!=orderIndex || orderTotal_!=orderTotal;
    elapsedSeconds_=seconds;
    songIndex_=songIndex;
    songTotal_=songTotal;
    orderIndex_=orderIndex;
    orderTotal_=orderTotal;
    if(changed) update();
}
void ClassicDisplay::paintEvent(QPaintEvent*) {
    QImage canvas(W,H,QImage::Format_ARGB32_Premultiplied);
    canvas.fill(QColor("#c0c0c0"));
    QPainter p(&canvas);
    p.setRenderHint(QPainter::Antialiasing,false);
    // Narrow title/filename toggle lives between the two display sections.
    // Align the outer edge of the title/toggle assembly with the pattern
    // LCD underneath (logical x=408), leaving the right status pane alone.
    bevel(p,{2,2,406,39});
    bevel(p,{417,2,156,94});
    bevel(p,{2,46,251,48});
    bevel(p,{257,46,151,48});
    bevel(p,{577,2,47,94});

    // The matrix renders true green pixel dots rather than a raster font.
    const QString visibleTitle=showFilename_ && !filename_.isEmpty()
        ?filename_:title_;
    drawMatrix(p,{8,5,373,32},visibleTitle);
    // Small recessed rocker, modelled on the original switch next to the
    // title matrix; its green marker changes end when toggled.
    p.fillRect(QRect(384,3,22,37),QColor("#c0c0c0"));
    p.setPen(QColor("#ffffff"));
    p.drawLine(384,3,406,3);
    p.drawLine(384,3,384,39);
    p.setPen(QColor("#626262"));
    p.drawLine(406,3,406,39);
    p.drawLine(384,39,406,39);
    p.fillRect(QRect(388,6,15,31),QColor("#e0e0e0"));
    p.fillRect(QRect(389,showFilename_?29:8,13,5),QColor("#14672b"));
    p.fillRect(QRect(390,showFilename_?30:9,11,3),lit);

    QFont small("Tahoma",8,QFont::Bold);
    small.setStyleStrategy(QFont::NoAntialias);
    p.setFont(small);
    p.setPen(lit);
    p.drawText(12,57,"MIN");
    p.drawText(73,57,"SEC");
    p.drawText(126,57,"Song");
    p.drawText(194,57,"Total");
    p.drawText(264,57,"Pattern");
    // Three original-era status words in the right edge of the pattern LCD.
    // Keep unselected modes softly visible; playback/playlist logic is wired
    // in a later release. The small '1' is the single-module repeat indicator.
    const auto modeLabel=[&](int y,const QString& name,bool enabled) {
        p.setPen(enabled?lit:QColor("#246b2a"));
        p.drawText(QRect(352,y,53,12),Qt::AlignLeft|Qt::AlignVCenter,name);
    };
    modeLabel(47,"Shuffle",false);
    modeLabel(60,"Repeat",false);
    modeLabel(73,"Introscan",false);
    p.setPen(QColor("#103718"));
    p.drawText(396,72,"1");
    p.setPen(lit);
    // Live numeric displays share the same audible-time snapshot as Effects.
    // The LCD has two physical seven-segment digits per field. Clamp rather
    // than truncate so a long sequence cannot misleadingly wrap to zero.
    const auto twoDigits=[](int value)->QString {
        return QString::number(std::clamp(value,0,99)).rightJustified(2,QLatin1Char('0'));
    };
    const int elapsed=std::max(0,int(elapsedSeconds_));
    digits(p,12,64,twoDigits(elapsed/60),2);
    p.fillRect(46,75,2,2,lit);
    p.fillRect(46,84,2,2,lit);
    digits(p,57,64,twoDigits(elapsed%60),2);
    digits(p,131,64,twoDigits(songIndex_),2);
    p.drawText(169,84,"of");
    digits(p,188,64,twoDigits(songTotal_),2);
    digits(p,264,64,twoDigits(orderIndex_),2);
    p.drawText(299,84,"of");
    digits(p,316,64,twoDigits(orderTotal_),2);

    p.drawText(424,18,"48kHz 16Bit");
    p.drawText(424,35,"STEREO");
    p.drawText(424,52,"CH: -- BPM: ---");
    p.drawText(424,68,state_.left(16).toUpper());
    p.drawText(424,83,nonzero_?"AUDIO ACTIVE":"AUDIO IDLE");

    // PREAMP is a miniature graphical slider. The coloured glyphs remain
    // unchanged at every gain setting; only the bar BEHIND them moves.
    // Draw the letters as bitmaps to keep them identical on macOS/Windows.
    constexpr int trackTop=9, trackTravel=61;
    const int markerY=trackTop+
        int(std::lround((20-preampDb_)*trackTravel/40.0));
    const QColor letterColors[6]={
        QColor("#cd2018"), QColor("#daa21c"), QColor("#21da28"),
        QColor("#138d1c"), QColor("#a38512"), QColor("#bd1b18")};
    static const char* const letterPixels[6][5]={
        {"1111110","1100011","1111110","1100000","1100000"}, // P
        {"1111110","1100011","1111110","1101100","1100011"}, // R
        {"1111111","1100000","1111110","1100000","1111111"}, // E
        {"0111110","1100011","1111111","1100011","1100011"}, // A
        {"1100011","1110111","1111111","1101011","1100011"}, // M
        {"1111110","1100011","1111110","1100000","1100000"}  // P
    };
    // The original uses a ZERO-CENTRED LEVEL FILL, not a travelling
    // backlight strip. At positive gain the region between the centre and
    // slider lights UP; at negative gain it lights DOWN. At 0 dB there is no
    // illuminated region. Colours belong to fixed positions in the scale,
    // and the PREAMP letters are always drawn in their original colours.
    const int zeroY=trackTop+
        int(std::lround(20.0*trackTravel/40.0));
    const int fillTop=std::min(markerY,zeroY);
    const int fillBottom=std::max(markerY,zeroY);
    p.save();
    p.setClipRect(QRect(584,5,36,67));
    for(int y=fillTop;y<fillBottom;++y) {
        QColor band;
        // Top to bottom: red, amber, vivid green, dark green, amber, red.
        // The scale stays put when the slider moves.
        if(y<16) band=QColor("#f13820");
        else if(y<26) band=QColor("#f3d62b");
        else if(y<44) band=QColor("#24f026");
        else if(y<54) band=QColor("#22db29");
        else if(y<63) band=QColor("#e0c52d");
        else band=QColor("#ec3025");
        p.fillRect(585,y,33,1,band);
    }
    p.restore();
    // Narrow recessed metal handle travels over the coloured fill.
    p.fillRect(581,markerY-3,39,7,QColor("#777777"));
    p.fillRect(582,markerY-2,37,5,QColor("#e6e6e6"));
    p.fillRect(583,markerY-1,35,3,QColor("#ffffff"));

    for(int i=0;i<6;++i) {
        const int y0=5+i*11;
        for(int row=0;row<5;++row) {
            for(int col=0;col<7;++col) {
                if(letterPixels[i][row][col]!='1') continue;
                // Seven columns at four pixels each (28 px), centred
                // inside the 47 px PREAMP panel.
                p.fillRect(587+col*4,y0+row*2,4,2,letterColors[i]);
            }
        }
    }
    // Signed gain and unit on one line, as an intentional modernisation.
    // Keep both legible at the original 626x96 logical resolution.
    QFont readout("Tahoma",8,QFont::Bold);
    readout.setStyleStrategy(QFont::NoAntialias);
    p.setFont(readout);
    p.setPen(lit);
    const QString gainText=(preampDb_>0?"+":"")
        +QString::number(preampDb_)+"dB";
    p.drawText(QRect(578,75,44,17),Qt::AlignCenter,gainText);
    p.end();

    QPainter output(this);
    output.setRenderHint(QPainter::SmoothPixmapTransform,false);
    output.drawImage(rect(),canvas);
}
