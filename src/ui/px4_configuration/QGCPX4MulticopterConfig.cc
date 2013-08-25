#include "QGCPX4MulticopterConfig.h"
#include "ui_QGCPX4MulticopterConfig.h"

QGCPX4MulticopterConfig::QGCPX4MulticopterConfig(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::QGCPX4MulticopterConfig)
{
    ui->setupUi(this);
}

QGCPX4MulticopterConfig::~QGCPX4MulticopterConfig()
{
    delete ui;
}
