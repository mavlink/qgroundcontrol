#ifndef SLUGSPADCAMERACONTROL_H
#define SLUGSPADCAMERACONTROL_H

#include <QWidget>
#include <QGraphicsView>

namespace Ui {
    class SlugsPadCameraControl;
}

class SlugsPadCameraControl : public QWidget //QGraphicsView//
{
    Q_OBJECT

public:
    explicit SlugsPadCameraControl(QWidget *parent = 0);

    ~SlugsPadCameraControl();

public slots:
    void getDeltaPositionPad(int x, int y);



    double getDistPixel(int x1, int y1, int x2, int y2);
    QPointF ObtenerMarcacionDistanciaPixel(double lon1, double lat1, double lon2, double lat2);
    QPointF getPointBy_BearingDistance(double lat1, double lon1, double rumbo, double distancia);




signals:
    void mouseMoveCoord(int x, int y);
    void mousePressCoord(int x, int y);
    void mouseReleaseCoord(int x, int y);
    void dirCursorText(QString dir);
    void distance_Bearing(double dist, double bearing);
     void changeCursorPosition(double bearing, double distance, QString textDir);

protected:
    void mousePressEvent(QMouseEvent* event);
    void mouseReleaseEvent(QMouseEvent* event);
    void mouseMoveEvent(QMouseEvent* event);
    void paintEvent(QPaintEvent *pe);


private:
    Ui::SlugsPadCameraControl *ui;
      bool dragging;
      int x1;
      int y1;
      int xFin;
      int yFin;
      double bearingPad;
      double distancePad;
      QString directionPad;

};

#endif // SLUGSPADCAMERACONTROL_H
