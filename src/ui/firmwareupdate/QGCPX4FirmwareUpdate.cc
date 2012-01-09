#include "QGCPX4FirmwareUpdate.h"
#include "ui_QGCPX4FirmwareUpdate.h"

QGCPX4FirmwareUpdate::QGCPX4FirmwareUpdate(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::QGCPX4FirmwareUpdate)
{
    ui->setupUi(this);
}

QGCPX4FirmwareUpdate::~QGCPX4FirmwareUpdate()
{
    delete ui;
}
