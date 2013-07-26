#include "ApmFirmwareStatus.h"


ApmFirmwareStatus::ApmFirmwareStatus(QWidget *parent) : QWidget(parent)
{
    ui.setupUi(this);
    ui.progressBar->setMaximum(50);
}

ApmFirmwareStatus::~ApmFirmwareStatus()
{
}
void ApmFirmwareStatus::passMessage(QString msg)
{
    ui.textBrowser->append(msg);
}
void ApmFirmwareStatus::setStatus(QString message)
{
    ui.label->setText("<h2>" + message + "</h2>");
}

void ApmFirmwareStatus::resetProgress()
{
    ui.progressBar->setValue(0);
}

void ApmFirmwareStatus::progressTick()
{
    ui.progressBar->setValue(ui.progressBar->value()+1);
}
