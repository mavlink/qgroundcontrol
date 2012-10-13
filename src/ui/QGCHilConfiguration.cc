#include "QGCHilConfiguration.h"
#include "ui_QGCHilConfiguration.h"

#include "QGCHilFlightGearConfiguration.h"
#include "QGCHilXPlaneConfiguration.h"

QGCHilConfiguration::QGCHilConfiguration(UAS *mav, QWidget *parent) :
    QWidget(parent),
    mav(mav),
    ui(new Ui::QGCHilConfiguration)
{
    ui->setupUi(this);

    connect(mav->getHILSimulation(), SIGNAL(statusMessage(QString)), this, SLOT(receiveStatusMessage(QString)));
    connect(ui->simComboBox, SIGNAL(activated(QString)), mav->getHILSimulation(), SLOT(setVersion(QString)));
    //ui->simComboBox->setEditText(mav->getHILSimulation()->getVersion());

//    connect(ui->)
}

void QGCHilConfiguration::receiveStatusMessage(const QString& message)
{
    ui->statusLabel->setText(message);
}

QGCHilConfiguration::~QGCHilConfiguration()
{
    delete ui;
}

void QGCHilConfiguration::on_simComboBox_currentIndexChanged(int index)
{
    //XXX make sure here that no other simulator is running
    if(1 == index)
    {
        QGCHilFlightGearConfiguration* hfgconf = new QGCHilFlightGearConfiguration(mav, this);
        hfgconf->show();
        ui->simulatorConfigurationDockWidget->setWidget(hfgconf);

    }
    else if(2 == index || 3 == index)
    {
        QGCHilXPlaneConfiguration* hxpconf = new QGCHilXPlaneConfiguration(mav->getHILSimulation(), this);
        hxpconf->show();
        ui->simulatorConfigurationDockWidget->setWidget(hxpconf);

    }
}
