#include "QGCPX4AirframeConfig.h"
#include "ui_QGCPX4AirframeConfig.h"

QGCPX4AirframeConfig::QGCPX4AirframeConfig(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::QGCPX4AirframeConfig)
{
    ui->setupUi(this);
}

QGCPX4AirframeConfig::~QGCPX4AirframeConfig()
{
    delete ui;
}
