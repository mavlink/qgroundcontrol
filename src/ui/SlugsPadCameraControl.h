#ifndef SLUGSPADCAMERACONTROL_H
#define SLUGSPADCAMERACONTROL_H

#include <QtGui/QWidget>
#include <QGraphicsView>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QDebug>
#include <qmath.h>
#include <QPainter>
#include "UASManager.h"

namespace Ui
{
class SlugsPadCameraControl;
}

class SlugsPadCameraControl : public QWidget //QGraphicsView//
{
    Q_OBJECT

public:
    explicit SlugsPadCameraControl(QWidget *parent = 0);

    ~SlugsPadCameraControl();

    enum MotionCamera {
        UP,
        DOWN,
        LEFT,
        RIGHT,
        RIGHT_UP,
        RIGHT_DOWN,
        LEFT_UP,
        LEFT_DOWN,
        NONE
    };

public slots:
    void getDeltaPositionPad(int x, int y);
    QPointF ObtenerMarcacionDistanciaPixel(double lon1, double lat1, double lon2, double lat2);
    void activeUasSet(UASInterface *uas);

signals:
    void changeMotionCamera(MotionCamera);

protected:
    void mousePressEvent(QMouseEvent* event);
    void mouseReleaseEvent(QMouseEvent* event);
    void mouseMoveEvent(QMouseEvent* event);
    void keyPressEvent(QKeyEvent *event);
    //void paintEvent(QPaintEvent *pe);


private:
    Ui::SlugsPadCameraControl *ui;
    bool dragging;
    int x1;
    int y1;
    int xFin;
    int yFin;
    QString directionPad;
    MotionCamera motion;
    UASInterface* activeUAS;
    QPoint movePad;

};

#endif // SLUGSPADCAMERACONTROL_H
