#ifndef QGCMAPTOOLBAR_H
#define QGCMAPTOOLBAR_H

#include <QWidget>

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

protected:
    QGCMapWidget* map;

private:
    Ui::QGCMapToolBar *ui;
};

#endif // QGCMAPTOOLBAR_H
