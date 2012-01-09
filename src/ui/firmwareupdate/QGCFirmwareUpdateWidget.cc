#include "QGCFirmwareUpdateWidget.h"
#include "ui_QGCFirmwareUpdateWidget.h"

QGCFirmwareUpdateWidget::QGCFirmwareUpdateWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::QGCFirmwareUpdateWidget)
{
    ui->setupUi(this);
}

QGCFirmwareUpdateWidget::~QGCFirmwareUpdateWidget()
{
    delete ui;
}
