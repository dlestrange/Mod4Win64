#pragma once
#include <QWidget>
#include <QString>
class QPainter;
class QRect;
class ClassicDisplay final : public QWidget {
    Q_OBJECT
public:
    explicit ClassicDisplay(QWidget* parent = nullptr);
    // Share the exact main LCD 5x7 pixel renderer with other panels.
    static void paintDotMatrix(QPainter& painter, const QRect& area, const QString& title);
    void setTitle(const QString& title);
    void setModuleNames(const QString& title, const QString& filename);
    void setState(const QString& state);
    void setDiagnostics(quint64 frames, quint64 nonzero);
    void setPlaybackCounters(double seconds, int songIndex, int songTotal,
                             int orderIndex, int orderTotal);
    void setPlaybackModes(bool shuffle, int repeatMode, bool introScan, bool remaining);
    void setModuleDuration(double seconds);
    QSize sizeHint() const override { return {626, 96}; }
signals:
    void titleMatrixClicked(const QPoint& globalPosition);
    void preampGainChanged(int decibels);
protected:
    void paintEvent(QPaintEvent*) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
private:
    QString title_ = "NO MODULE LOADED";
    QString filename_;
    bool showFilename_ = false;
    int preampDb_ = 0; // -20 to +20 dB, mirrored to audio by signal.
    bool draggingPreamp_ = false;
    void setPreampFromY(qreal widgetY);
    QString state_ = "STOPPED";
    double elapsedSeconds_ = 0.0;
    double moduleDuration_ = 0.0;
    bool shuffle_ = false;
    int repeatMode_ = 0; // 0=off, 1=all, 2=one
    bool introScan_ = false;
    bool remaining_ = false;
    int songIndex_ = 0;
    int songTotal_ = 0;
    int orderIndex_ = 0;
    int orderTotal_ = 0;
    quint64 frames_ = 0;
    quint64 nonzero_ = 0;
};
