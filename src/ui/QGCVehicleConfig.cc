#include "QGCVehicleConfig.h"
#include "ui_QGCVehicleConfig.h"

QGCVehicleConfig::QGCVehicleConfig(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::QGCVehicleConfig)
{
    ui->setupUi(this);
}

QGCVehicleConfig::~QGCVehicleConfig()
{
    delete ui;
}
