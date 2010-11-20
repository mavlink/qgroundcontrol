#include "SlugsVideoCamControl.h"
#include "ui_SlugsVideoCamControl.h"

#include <QGraphicsScene>
#include <QGraphicsTextItem>
#include <QTextStream>
#include <QScrollBar>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QDebug>


SlugsVideoCamControl::SlugsVideoCamControl(QWidget *parent) :
    ui(new Ui::SlugsVideoCamControl)
{
    ui->setupUi(this);

//    QGraphicsScene *scene = new QGraphicsScene(ui->CamControlPanel_graphicsView);
//         scene->setItemIndexMethod(QGraphicsScene::NoIndex);
//         scene->setSceneRect(-200, -200, 400, 400);
//         setScene(scene);
//         setCacheMode(CacheBackground);
//         setViewportUpdateMode(BoundingRectViewportUpdate);
//         setRenderHint(QPainter::Antialiasing);
//         setTransformationAnchor(AnchorUnderMouse);
//         setResizeAnchor(AnchorViewCenter);

      ui->CamControlPanel_graphicsView->installEventFilter(this);
      ui->label_x->installEventFilter(this);

}

SlugsVideoCamControl::~SlugsVideoCamControl()
{
    delete ui;
}

void SlugsVideoCamControl::mouseMoveEvent(QMouseEvent *event)
{
    ui->label_x->setText(QString::number(event->x()));
    ui->label_y->setText(QString::number(event->y()));
}


//void SlugsVideoCamControl::mousePressEvent(QMouseEvent *evnt)
//{

//}

//void SlugsVideoCamControl::mouseReleaseEvent(QMouseEvent *evnt)
//{

//}

//void SlugsVideoCamControl::mouseDoubleClickEvent(QMouseEvent *evnt)
//{

//}
