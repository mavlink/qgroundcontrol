#include "SlugsPadCameraControl.h"
#include "ui_SlugsPadCameraControl.h"
#include <QMouseEvent>
#include <QDebug>
#include <qmath.h>
#include <QPainter>

SlugsPadCameraControl::SlugsPadCameraControl(QWidget *parent) :
   QWidget(parent), //QGraphicsView(parent),
    ui(new Ui::SlugsPadCameraControl),
     dragging(0)
{
    ui->setupUi(this);
    x1= 0;
    y1 = 0;
    bearingPad = 0;
    distancePad = 0;
    directionPad = "no";

}

SlugsPadCameraControl::~SlugsPadCameraControl()
{
    delete ui;
}

void SlugsPadCameraControl::mouseMoveEvent(QMouseEvent *event)
{
    //emit mouseMoveCoord(event->x(),event->y());

    if(dragging)
    {

      // getDeltaPositionPad(event->x(), event->y());

    }


}

void SlugsPadCameraControl::mousePressEvent(QMouseEvent *event)
{
    //emit mousePressCoord(event->x(),event->y());
    dragging = true;
    x1 = event->x();
    y1 = event->y();

}

void SlugsPadCameraControl::mouseReleaseEvent(QMouseEvent *event)
{
     dragging = false;
    //emit mouseReleaseCoord(event->x(),event->y());
    getDeltaPositionPad(event->x(), event->y());

     xFin = event->x();
     yFin = event->y();


}

void SlugsPadCameraControl::paintEvent(QPaintEvent *pe)
{
    Q_UNUSED(pe);
    QPainter painter(this);
    painter.setPen(Qt::blue);
    painter.setFont(QFont("Arial", 30));

//    QRectF rectangle(tL.x(), tL.y(), ui->padCamContro_frame->width(), ui->padCamContro_frame->height());
//     int startAngle = 30 * 16;
//     int spanAngle = 120 * 16;

    painter.drawLine(QPoint(ui->frame->width()/2,ui->frame->geometry().topLeft().y()),
                     QPoint(ui->frame->width()/2,ui->frame->geometry().bottomRight().y()));

    painter.drawLine(QPoint(ui->frame->geometry().topLeft().x(),ui->frame->height()/2),
                     QPoint(ui->frame->geometry().bottomRight().x(),ui->frame->height()/2));

    painter.setPen(Qt::white);

    //QPointF coordTemp = getPointBy_BearingDistance(ui->frame->width()/2,ui->frame->height()/2,bearingPad,distancePad);

    painter.drawLine(QPoint(ui->frame->width()/2,ui->frame->height()/2),
                     QPoint(xFin,yFin));


   // painter.drawLine(QPoint());
    //painter.drawLines(padLines);


    // painter.drawPie(rectangle, startAngle, spanAngle);

    //painter.drawText(rect(), Qt::AlignCenter, "Qt");
}

void SlugsPadCameraControl::getDeltaPositionPad(int x2, int y2)
{

    QString dir = "nd";
    QPointF localMeasures = ObtenerMarcacionDistanciaPixel(y1,x1,y2,x2);

    double bearing = localMeasures.x();
    double dist = getDistPixel(y1,x1,y2,x2);

    // this only convert real bearing to frame widget bearing
    bearing = bearing +90;
    if(bearing>= 360) bearing = bearing - 360;



    if(((bearing > 330)&&(bearing < 360)) || ((bearing >= 0)&&(bearing <= 30)))
    {
         emit dirCursorText("up");
        //bearing = 315;
        dir = "up";
    }
    else
    {
        if((bearing > 30)&&(bearing <= 60) )
        {
            emit dirCursorText("right up");
            //bearing = 315;
            dir = "right up";
        }
        else
        {
            if((bearing > 60)&&(bearing <= 105) )
            {
               emit dirCursorText("right");
                //bearing = 315;
                dir = "right";
            }
            else
            {
                if((bearing > 105)&&(bearing <= 150) )
                {
                   emit dirCursorText("right down");
                    //bearing = 315;
                    dir = "right down";
                }
                else
                {
                    if((bearing > 150)&&(bearing <= 195) )
                    {
                       emit dirCursorText("down");
                        //bearing = 315;
                        dir = "down";
                    }
                    else
                    {
                        if((bearing > 195)&&(bearing <= 240) )
                        {
                            emit dirCursorText("left down");
                            //bearing = 315;
                            dir = "left down";
                        }
                        else
                        {
                            if((bearing > 240)&&(bearing <= 300) )
                            {
                               emit dirCursorText("left");
                                //bearing = 315;
                                dir = "left";
                            }
                            else
                            {
                                if((bearing > 300)&&(bearing <= 330) )
                                {
                                    emit dirCursorText("left up");
                                    //bearing = 315;
                                    dir = "left up";
                                }

                            }

                        }

                    }

                }

            }

        }

    }


    bearingPad = bearing;
    distancePad = dist;
    directionPad = dir;
    emit changeCursorPosition(bearing, dist, dir);

    update();



}

double SlugsPadCameraControl::getDistPixel(int x1, int y1, int x2, int y2)
{
    double cateto_opuesto,cateto_adyacente;
     //latitud y longitud del primer punto


    cateto_opuesto = abs((x1-x2)); //diferencia de latitudes entre PCR1 y PCR2
    cateto_adyacente = abs((y1-y2));//diferencia de longitudes entre PCR1 y PCR2

    return  sqrt(pow(cateto_opuesto,2) + pow(cateto_adyacente,2));

      // distancia = (float) hipotenusa;
}


QPointF SlugsPadCameraControl::ObtenerMarcacionDistanciaPixel(double lon1, double lat1,
                                                              double lon2, double lat2)
{
    double cateto_opuesto,cateto_adyacente, hipotenusa, distancia;
    double marcacion = 0.0;

    //latitude and longitude first point

    if(lat1<0) lat1= lat1*(-1);
    if(lat2<0) lat2= lat2*(-1);
    if(lon1<0) lon1= lon1*(-1);
    if(lon2<0) lon1= lon1*(-1);

    cateto_opuesto = abs((lat1-lat2));
    cateto_adyacente = abs((lon1-lon2));

    hipotenusa = sqrt(pow(cateto_opuesto,2) + pow(cateto_adyacente,2));
    distancia = hipotenusa*60.0;


    if ((lat1 < lat2) && (lon1 > lon2)) //primer cuadrante
        marcacion = 360 -((asin(cateto_adyacente/hipotenusa))/ 0.017453292);
    else if ((lat1 < lat2) && (lon1 < lon2)) //segundo cuadrante
        marcacion = (asin(cateto_adyacente/hipotenusa))/ 0.017453292;
    else if((lat1 > lat2) && (lon1 < lon2)) //tercer cuadrante
        marcacion = 180 -((asin(cateto_adyacente/hipotenusa))/ 0.017453292);
    else if((lat1 > lat2) && (lon1 > lon2)) //cuarto cuadrante
        marcacion = 180 +((asin(cateto_adyacente/hipotenusa))/ 0.017453292);
    else if((lat1 < lat2) && (lon1 == lon2)) //360
        marcacion = 360;
    else if((lat1 == lat2) && (lon1 > lon2)) //270
        marcacion = 270;
    else if((lat1 > lat2) && (lon1 == lon2)) //180
        marcacion = 180;
    else if((lat1 == lat2) && (lon1 < lon2)) //90
        marcacion =90;
    else if((lat1 == lat2) && (lon1 == lon2)) //0
        marcacion = 0.0;



    return QPointF(marcacion,distancia);

}



QPointF SlugsPadCameraControl::getPointBy_BearingDistance(double lat1, double lon1, double rumbo, double distancia)
{
    double lon2 = 0;
    double lat2 = 0;
    double rad= M_PI/180;

    rumbo = rumbo*rad;
    lon2=(lon1 + ((distancia/60) * (sin(rumbo))));
    lat2=(lat1 + ((distancia/60) * (cos(rumbo))));

    return QPointF(lon2,lat2);
}

