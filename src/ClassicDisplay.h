#pragma once
#include <QWidget>
#include <QString>
class ClassicDisplay final : public QWidget {
    Q_OBJECT
public:
    explicit ClassicDisplay(QWidget* parent = nullptr);
    void setTitle(const QString& title);
    void setModuleNames(const QString& title, const QString& filename);
    void setState(const QString& state);
    void setDiagnostics(quint64 frames, quint64 nonzero);
    void setPlaybackCounters(double seconds, int songIndex, int songTotal,
                             int orderIndex, int orderTotal);
    QSize sizeHint() const override { return {626, 96}; }
signals:
    void titleMatrixClicked(const QPoint& globalPosition);
protected:
    void paintEvent(QPaintEvent*) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
private:
    QString title_ = "NO MODULE LOADED";
    QString filename_;
    bool showFilename_ = false;
    int preampDb_ = 0; // Display-only until the audio preamp is connected.
    bool draggingPreamp_ = false;
    void setPreampFromY(qreal widgetY);
    QString state_ = "STOPPED";
    double elapsedSeconds_ = 0.0;
    int songIndex_ = 0;
    int songTotal_ = 0;
    int orderIndex_ = 0;
    int orderTotal_ = 0;
    quint64 frames_ = 0;
    quint64 nonzero_ = 0;
};
