#ifndef SLUGSVIDEOCAMCONTROL_H
#define SLUGSVIDEOCAMCONTROL_H

#include <QWidget>
#include <QMouseEvent>
#include <QGraphicsView>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsScene>
#include "SlugsPadCameraControl.h"


#define DELTA 1000

namespace Ui {
    class SlugsVideoCamControl;
}

class SlugsVideoCamControl : public QWidget

{
    Q_OBJECT

public:
    explicit SlugsVideoCamControl(QWidget *parent = 0);
    ~SlugsVideoCamControl();

public slots:
    void changeViewCamBorderAtMapStatus(bool status);
    void getDeltaPositionPad(int x, int y);
    double getDistPixel(int x1, int y1, int x2, int y2);
    QPointF ObtenerMarcacionDistanciaPixel(double lon1, double lat1, double lon2, double lat2);

signals:
    void changeCamPosition(double dist, double dir, QString textDir);
    void viewCamBorderAtMap(bool status);

protected:
    virtual void mousePressEvent(QMouseEvent* event);
    virtual void mouseReleaseEvent(QMouseEvent* event);
    void mouseMoveEvent(QMouseEvent* event);
    //virtual void paintEvent(QPaintEvent *pe);
    void paintEvent(QPaintEvent *pe);


private:
    Ui::SlugsVideoCamControl *ui;
    bool dragging;

    int x1;
    int y1;
     QPoint tL;
      QPoint bR;

       SlugsPadCameraControl* padCamera;


};

#endif // SLUGSVIDEOCAMCONTROL_H
