#include "QGCFirmwareUpdate.h"
#include "ui_QGCFirmwareUpdate.h"

QGCFirmwareUpdate::QGCFirmwareUpdate(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::QGCFirmwareUpdate)
{
    ui->setupUi(this);
}

QGCFirmwareUpdate::~QGCFirmwareUpdate()
{
    delete ui;
}

void QGCFirmwareUpdate::changeEvent(QEvent *e)
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
