#include "SlugsVideoCamControl.h"
#include "ui_SlugsVideoCamControl.h"

#include <QGraphicsScene>
#include <QGraphicsTextItem>
#include <QTextStream>
#include <QScrollBar>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QDebug>
#include <qmath.h>
#include <QPainter>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>


SlugsVideoCamControl::SlugsVideoCamControl(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SlugsVideoCamControl),
    dragging(0)
{
    ui->setupUi(this);
   x1= 0;
   y1 = 0;
    connect(ui->viewCamBordeatMap_checkBox,SIGNAL(clicked(bool)),this,SLOT(changeViewCamBorderAtMapStatus(bool)));
    tL = ui->padCamContro_frame->frameGeometry().topLeft();
    bR = ui->padCamContro_frame->frameGeometry().bottomRight();
    //ui->padCamContro_frame->setVisible(true);

    // create a layout for camera pad
    QGridLayout* padCameraLayout = new QGridLayout(ui->padCamContro_frame);
    padCameraLayout->setSpacing(2);
    padCameraLayout->setMargin(0);
    padCameraLayout->setAlignment(Qt::AlignTop);
    ui->padCamContro_frame->setLayout(padCameraLayout);
    // create a camera pad widget
    padCamera = new SlugsPadCameraControl();



   padCameraLayout->addWidget(padCamera);



//    QGraphicsScene *scene = new QGraphicsScene(ui->CamControlPanel_graphicsView);
//         scene->setItemIndexMethod(QGraphicsScene::NoIndex);
//         scene->setSceneRect(-200, -200, 400, 400);
//         setScene(scene);
//         setCacheMode(CacheBackground);
//         setViewportUpdateMode(BoundingRectViewportUpdate);
//         setRenderHint(QPainter::Antialiasing);
//         setTransformationAnchor(AnchorUnderMouse);
//         setResizeAnchor(AnchorViewCenter);

//      ui->CamControlPanel_graphicsView->installEventFilter(this);
//      ui->label_x->installEventFilter(this);

}

SlugsVideoCamControl::~SlugsVideoCamControl()
{
    delete ui;
}

void SlugsVideoCamControl::mouseMoveEvent(QMouseEvent *event)
{
    // ui->label_dir->setText("Camera Pos = " + QString::number(event->x()) + " - " + QString::number(event->y()));
//  QPoint tL = ui->padCamContro_frame->frameGeometry().topLeft();
//  QPoint bR = ui->padCamContro_frame->frameGeometry().bottomRight();

//  if (!(event->x() > bR.x() || event->x() < tL.x() ||
//      event->y() > bR.y() || event->y() < tL.y() ) && dragging){


//  }


}


void SlugsVideoCamControl::mousePressEvent(QMouseEvent *evnt)
{
  Q_UNUSED(evnt);

   x1 = evnt->x();
   y1 = evnt->y();

  dragging = true;
}

void SlugsVideoCamControl::mouseReleaseEvent(QMouseEvent *evnt)
{
    Q_UNUSED(evnt);
    dragging = false;
    getDeltaPositionPad(evnt->x(), evnt->y());
}

void SlugsVideoCamControl::changeViewCamBorderAtMapStatus(bool status)
{
    emit viewCamBorderAtMap(status);
}

void SlugsVideoCamControl::getDeltaPositionPad(int x2, int y2)
{
    //double dist = getDistPixel(x1,y1,x2,y2);

    QString dir = "nd";
    QPointF localMeasures = ObtenerMarcacionDistanciaPixel(y1,x1,y2,x2);

    double bearing = localMeasures.x();
     double dist = localMeasures.y();

    bearing = bearing +90;
    if(bearing>= 360) bearing = bearing - 360;

    if(((bearing > 330)&&(bearing < 360)) || ((bearing >= 0)&&(bearing <= 30)))
    {
        ui->label_dir->setText("up");
        //bearing = 315;
        dir = "up";
    }
    else
    {
        if((bearing > 30)&&(bearing <= 60) )
        {
            ui->label_dir->setText("right up");
            //bearing = 315;
            dir = "riht up";
        }
        else
        {
            if((bearing > 60)&&(bearing <= 105) )
            {
                ui->label_dir->setText("right");
                //bearing = 315;
                dir = "riht";
            }
            else
            {
                if((bearing > 105)&&(bearing <= 150) )
                {
                    ui->label_dir->setText("right down");
                    //bearing = 315;
                    dir = "riht down";
                }
                else
                {
                    if((bearing > 150)&&(bearing <= 195) )
                    {
                        ui->label_dir->setText("down");
                        //bearing = 315;
                        dir = "down";
                    }
                    else
                    {
                        if((bearing > 195)&&(bearing <= 240) )
                        {
                            ui->label_dir->setText("left down");
                            //bearing = 315;
                            dir = "left down";
                        }
                        else
                        {
                            if((bearing > 240)&&(bearing <= 300) )
                            {
                                ui->label_dir->setText("left");
                                //bearing = 315;
                                dir = "left";
                            }
                            else
                            {
                                if((bearing > 300)&&(bearing <= 330) )
                                {
                                    ui->label_dir->setText("left up");
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

    ui->label_x->setText("Distancia= " + QString::number(dist) + " - x =" + QString::number(x1) );
    ui->label_y->setText("Bearing= " + QString::number(bearing) + " - y =  " + QString::number(y1));



    emit changeCamPosition(20, bearing, dir);

//    if((x2<x1)&&(y2<y1)) // left up
//    {
//        ui->label_dir->setText("left Up");
////        bearing = 315;
//        dir = "left up";


//    }
//    else
//    {
//        if((x2<x1)&&(y2>y1)) // left down
//        {
//            ui->label_dir->setText("left down");
////            bearing = 225;
//            dir = "left down";


//        }
//        else
//        {
//            if((x2<x1)&&(y2==y1))// left
//            {
//                ui->label_dir->setText("left");
////                bearing = 270;
//                dir = "left";

//            }
//            else
//            {
//                if((x2==x1)&&(y2<y1)) // up
//                {
//                    ui->label_dir->setText("Up");
////                    bearing = 0;
//                    dir = "up";

//                }
//                else
//                {
//                    if((x2==x1)&&(y2>y1)) // down
//                    {
//                        ui->label_dir->setText("down");
////                        bearing = 180;
//                        dir = "down";

//                    }
//                    else
//                    {
//                        if((x2>x1)&&(y2==y1)) // right
//                        {
//                            ui->label_dir->setText("right");
////                            bearing = 90;
//                            dir = "right";

//                        }
//                        else
//                        {
//                            if((x2>x1)&&(y2<y1))// right up
//                            {
//                                ui->label_dir->setText("right Up");
////                                bearing = 45;
//                                dir = "right up";

//                            }
//                            else
//                            {
//                                if((x2>x1)&&(y2>y1))//right down
//                                {
//                                    ui->label_dir->setText("right down");
////                                    bearing = 135;
//                                    dir = "right down";

//                                }
//                            }
//                        }
//                    }
//                }
//            }
//        }
//    }



}

double SlugsVideoCamControl::getDistPixel(int x1, int y1, int x2, int y2)
{
    double cateto_opuesto,cateto_adyacente;
     //latitud y longitud del primer punto


    cateto_opuesto = abs((x1-x2)); //diferencia de latitudes entre PCR1 y PCR2
    cateto_adyacente = abs((y1-y2));//diferencia de longitudes entre PCR1 y PCR2

    return  sqrt(pow(cateto_opuesto,2) + pow(cateto_adyacente,2));

      // distancia = (float) hipotenusa;
}

/**
                * Esta función xxxxxxxxx
                * @param double lat1 -->
                * @param double lon1 -->
                * @param double lat2 -->
                * @param double lon2 -->
                * @param ref double rumbo -->
                * @param ref double distancia -->
                */
QPointF SlugsVideoCamControl::ObtenerMarcacionDistanciaPixel(double lon1, double lat1, double lon2, double lat2)
{
    double cateto_opuesto,cateto_adyacente, hipotenusa, distancia, marcacion;

    //latitude and longitude first point

    if(lat1<0) lat1= lat1*(-1);
    if(lat2<0) lat2= lat2*(-1);
    if(lon1<0) lon1= lon1*(-1);
    if(lon2<0) lon1= lon1*(-1);

    cateto_opuesto = abs((lat1-lat2));
    cateto_adyacente = abs((lon1-lon2));

    hipotenusa = sqrt(pow(cateto_opuesto,2) + pow(cateto_adyacente,2));
    distancia = hipotenusa*60;


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

void SlugsVideoCamControl::paintEvent(QPaintEvent *pe)
{
    Q_UNUSED(pe);
    QPainter painter(this);
    painter.setPen(Qt::blue);
    painter.setFont(QFont("Arial", 30));

//    QRectF rectangle(tL.x(), tL.y(), ui->padCamContro_frame->width(), ui->padCamContro_frame->height());
//     int startAngle = 30 * 16;
//     int spanAngle = 120 * 16;

    painter.drawLine(QPoint(tL.x(),tL.y()),QPoint(bR.x(),bR.y()));
   // painter.drawLine(QPoint());
    //painter.drawLines(padLines);


    // painter.drawPie(rectangle, startAngle, spanAngle);

    //painter.drawText(rect(), Qt::AlignCenter, "Qt");
}





