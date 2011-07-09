#include "QGCMapTool.h"
#include "ui_QGCMapTool.h"

QGCMapTool::QGCMapTool(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::QGCMapTool)
{
    ui->setupUi(this);

    // Connect map and toolbar
    ui->toolBar->setMap(ui->map);
}

QGCMapTool::~QGCMapTool()
{
    delete ui;
}
