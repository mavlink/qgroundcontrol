#include "QGCPxImuFirmwareUpdate.h"
#include "ui_QGCPxImuFirmwareUpdate.h"

QGCPxImuFirmwareUpdate::QGCPxImuFirmwareUpdate(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::QGCPxImuFirmwareUpdate)
{
    ui->setupUi(this);
}

QGCPxImuFirmwareUpdate::~QGCPxImuFirmwareUpdate()
{
    delete ui;
}

void QGCPxImuFirmwareUpdate::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}
