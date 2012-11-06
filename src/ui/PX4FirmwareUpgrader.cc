#include "PX4FirmwareUpgrader.h"
#include "ui_PX4FirmwareUpgrader.h"

#include <QGC.h>
#include <QDebug>

PX4FirmwareUpgrader::PX4FirmwareUpgrader(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PX4FirmwareUpgrader)
{
    ui->setupUi(this);

    connect(ui->selectFileButton, SIGNAL(clicked()), this, SLOT(selectFirmwareFile()));
    connect(ui->flashButton, SIGNAL(clicked()), this, SIGNAL(upgrade()));
}

PX4FirmwareUpgrader::~PX4FirmwareUpgrader()
{
    delete ui;
}

void PX4FirmwareUpgrader::selectFirmwareFile()
{

}

void PX4FirmwareUpgrader::setDetectionStatusText(const QString &text)
{
    ui->detectionStatusLabel->setText(text);
}

void PX4FirmwareUpgrader::setFlashStatusText(const QString &text)
{
    ui->flashProgressLabel->setText(text);
}

void PX4FirmwareUpgrader::setFlashProgress(int percent)
{
    ui->flashProgressBar->setValue(percent);
}

void PX4FirmwareUpgrader::setPortName(const QString &portname)
{
    // Prepend newly found port to the list
    if (ui->serialPortComboBox->findText(portname) == -1)
    {
        ui->serialPortComboBox->insertItem(0, portname);
        ui->serialPortComboBox->setEditText(portname);
    }
}
