#ifndef QGCMAPTOOLBAR_H
#define QGCMAPTOOLBAR_H

#include <QWidget>
#include <QMenu>

class QGCMapWidget;

namespace Ui {
    class QGCMapToolBar;
}

class QGCMapToolBar : public QWidget
{
    Q_OBJECT

public:
    explicit QGCMapToolBar(QWidget *parent = 0);
    ~QGCMapToolBar();

    void setMap(QGCMapWidget* map);

public slots:
    void tileLoadStart();
    void tileLoadEnd();
    void tileLoadProgress(int progress);
    void setUAVTrailTime();
    void setUAVTrailDistance();

protected:
    QGCMapWidget* map;
    QMenu optionsMenu;
    QMenu trailPlotMenu;

private:
    Ui::QGCMapToolBar *ui;
};

#endif // QGCMAPTOOLBAR_H
