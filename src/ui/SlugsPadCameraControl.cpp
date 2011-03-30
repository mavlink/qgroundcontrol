#include "SlugsPadCameraControl.h"
#include "ui_SlugsPadCameraControl.h"

SlugsPadCameraControl::SlugsPadCameraControl(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SlugsPadCameraControl),
    dragging(0)
{
    ui->setupUi(this);
    x1= 0;
    y1 = 0;
    motion = NONE;
}

SlugsPadCameraControl::~SlugsPadCameraControl()
{
    delete ui;
}

void SlugsPadCameraControl::activeUasSet(UASInterface *uas)
{
    if(uas) {
        this->activeUAS= uas;
    }
}

void SlugsPadCameraControl::mouseMoveEvent(QMouseEvent *event)
{
    Q_UNUSED(event);

    if(dragging) {
        getDeltaPositionPad(event->x(), event->y());
    }
}

void SlugsPadCameraControl::mousePressEvent(QMouseEvent *event)
{
    if(!dragging) {
        dragging = true;
        x1 = event->x();
        y1 = event->y();
    }
}

void SlugsPadCameraControl::mouseReleaseEvent(QMouseEvent *event)
{
    if(dragging) {
        dragging = false;
        getDeltaPositionPad(event->x(), event->y());

        xFin = event->x();
        yFin = event->y();
    }
}

void SlugsPadCameraControl::getDeltaPositionPad(int x2, int y2)
{
    QPointF localMeasures = ObtenerMarcacionDistanciaPixel(y1,x1,y2,x2);

    if(localMeasures.y()>10) {
        QString dir = "nd";

        double bearing = localMeasures.x();

        bearing = bearing +90;

        if(bearing>= 360) {
            bearing = bearing - 360;
        }

        if(bearing >337.5 || bearing <=22.5) {
            motion= UP;
            movePad = QPoint(0, 1);
            dir = "UP";
        } else if(bearing >22.5 && bearing <=67.5) {
            motion= RIGHT_UP;
            movePad = QPoint(1, 1);
            dir = "RIGHT UP";
        } else if(bearing >67.5 && bearing <=112.5) {
            motion= RIGHT;
            movePad = QPoint(1, 0);
            dir = "RIGHT";
        } else if(bearing >112.5 && bearing <= 157.5) {
            motion= RIGHT_DOWN;
            movePad = QPoint(1, -1);
            dir = "RIGHT DOWN";
        } else if(bearing >157.5 && bearing <=202.5) {
            motion= DOWN;
            movePad = QPoint(0, -1);
            dir = "DOWN";
        } else if(bearing >202.5 && bearing <=247.5) {
            motion= LEFT_DOWN;
            movePad = QPoint(-1, -1);
            dir = "LEFT DOWN";
        } else if(bearing >247.5 && bearing <=292.5) {
            motion= LEFT;
            movePad = QPoint(-1, 0);
            dir = "LEFT";
        } else if(bearing >292.5 && bearing <=337.5) {
            motion= LEFT_UP;
            movePad = QPoint(-1, 1);
            dir = "LEFT UP";
        }

        emit changeMotionCamera(motion);

        ui->lbPixel->setText(QString::number(localMeasures.y()));
        ui->lbDirection->setText(dir);

        //qDebug()<<dir;
        update();
    }
}

QPointF SlugsPadCameraControl::ObtenerMarcacionDistanciaPixel(double lon1, double lat1,
        double lon2, double lat2)
{
    double cateto_opuesto,cateto_adyacente, hipotenusa;//, distancia;
    double marcacion = 0.0;

    //latitude and longitude first point

    if(lat1<0) lat1= lat1*(-1);
    if(lat2<0) lat2= lat2*(-1);
    if(lon1<0) lon1= lon1*(-1);
    if(lon2<0) lon1= lon1*(-1);

    cateto_opuesto = abs((lat1-lat2));
    cateto_adyacente = abs((lon1-lon2));

    hipotenusa = sqrt(pow(cateto_opuesto,2) + pow(cateto_adyacente,2));
    //distancia = hipotenusa*60.0;


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

    return QPointF(marcacion,hipotenusa);// distancia);
}

void SlugsPadCameraControl::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_Left:
        emit changeMotionCamera(LEFT);
        break;

    case Qt::Key_Right:
        emit changeMotionCamera(RIGHT);
        break;

    case Qt::Key_Down:
        emit changeMotionCamera(DOWN);
        break;

    case Qt::Key_Up:
        emit changeMotionCamera(UP);
        break;

    default:
        QWidget::keyPressEvent(event);
    }
}
