#include "QGCMapTool.h"
#include "ui_QGCMapTool.h"

QGCMapTool::QGCMapTool(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::QGCMapTool)
{
    ui->setupUi(this);

    // Connect map and toolbar
    ui->toolBar->setMap(ui->map);
    // Connect zoom slider and map
    ui->zoomSlider->setMinimum(ui->map->MinZoom());
    ui->zoomSlider->setMaximum(ui->map->MaxZoom());
    ui->zoomSlider->setValue(ui->map->ZoomReal());
    connect(ui->zoomSlider, SIGNAL(valueChanged(int)), ui->map, SLOT(SetZoom(int)));
    connect(ui->map, SIGNAL(zoomChanged(int)), ui->zoomSlider, SLOT(setValue(int)));
}

QGCMapTool::~QGCMapTool()
{
    delete ui;
}
