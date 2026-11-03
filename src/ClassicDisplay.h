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
    QSize sizeHint() const override { return {626, 96}; }
protected:
    void paintEvent(QPaintEvent*) override;
    void mousePressEvent(QMouseEvent* event) override;
private:
    QString title_ = "NO MODULE LOADED";
    QString filename_;
    bool showFilename_ = false;
    QString state_ = "STOPPED";
    quint64 frames_ = 0;
    quint64 nonzero_ = 0;
};
