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
#include <QPushButton>
#include "SlugsPadCameraControl.h"


SlugsVideoCamControl::SlugsVideoCamControl(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SlugsVideoCamControl)
{
    ui->setupUi(this);
//   x1= 0;
//   y1 = 0;

    connect(ui->viewCamBordeatMap_checkBox,SIGNAL(clicked(bool)),this,SLOT(changeViewCamBorderAtMapStatus(bool)));
    padCamera = new SlugsPadCameraControl(this);

    ui->gridLayout->addWidget(padCamera);

    //connect(padCamera,SIGNAL(mouseMoveCoord(int,int)),this,SLOT(mousePadMoveEvent(int,int)));
    //connect(padCamera,SIGNAL(mousePressCoord(int,int)),this,SLOT(mousePadPressEvent(int,int)));
    //connect(padCamera,SIGNAL(mouseReleaseCoord(int,int)),this,SLOT(mousePadReleaseEvent(int,int)));
    //connect(padCamera,SIGNAL(changeCursorPosition(double,double,QString)),this,SLOT(getDeltaPositionPad(double,double,QString)));


}

SlugsVideoCamControl::~SlugsVideoCamControl()
{
    delete ui;
}

//void SlugsVideoCamControl::mouseMoveEvent(QMouseEvent *event)
//{
//     Q_UNUSED(event);

//}


//void SlugsVideoCamControl::mousePressEvent(QMouseEvent *evnt)
//{
//  Q_UNUSED(evnt);

//}

//void SlugsVideoCamControl::mouseReleaseEvent(QMouseEvent *evnt)
//{
//    Q_UNUSED(evnt);

//}


//void SlugsVideoCamControl::mousePadMoveEvent(int x, int y)
//{

//}

//void SlugsVideoCamControl::mousePadPressEvent(int x, int y)
//{

//}

//void SlugsVideoCamControl::mousePadReleaseEvent(int x, int y)
//{


//}

void SlugsVideoCamControl::changeViewCamBorderAtMapStatus(bool status)
{
    emit viewCamBorderAtMap(status);
}

void SlugsVideoCamControl::getDeltaPositionPad(double bearing, double distance, QString dirText)
{
    ui->label_dir->setText(dirText);
    ui->label_x->setText("Distancia= " + QString::number(distance));
    ui->label_y->setText("Bearing= " + QString::number(bearing));

    //emit changeCamPosition(20, bearing, dirText);
}

