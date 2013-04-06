#include "QGCHilConfiguration.h"
#include "ui_QGCHilConfiguration.h"

#include "QGCHilFlightGearConfiguration.h"
#include "QGCHilJSBSimConfiguration.h"
#include "QGCHilXPlaneConfiguration.h"

QGCHilConfiguration::QGCHilConfiguration(UAS *mav, QWidget *parent) :
    QWidget(parent),
    mav(mav),
    ui(new Ui::QGCHilConfiguration)
{
    ui->setupUi(this);
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
    //clean up
    QLayoutItem *child;
    while ((child = ui->simulatorConfigurationLayout->takeAt(0)) != 0)
    {
      delete child->widget();
      delete child;
    }

    if(1 == index)
    {
        // Ensure the sim exists and is disabled
        mav->enableHilFlightGear(false, "");
        QGCHilFlightGearConfiguration* hfgconf = new QGCHilFlightGearConfiguration(mav, this);
        hfgconf->show();
        ui->simulatorConfigurationLayout->addWidget(hfgconf);
        QGCFlightGearLink* fg = dynamic_cast<QGCFlightGearLink*>(mav->getHILSimulation());
        if (fg)
        {
            connect(fg, SIGNAL(statusMessage(QString)), ui->statusLabel, SLOT(setText(QString)));
        }

    }
    else if (2 == index || 3 == index)
    {
        // Ensure the sim exists and is disabled
        mav->enableHilXPlane(false);
        QGCHilXPlaneConfiguration* hxpconf = new QGCHilXPlaneConfiguration(mav->getHILSimulation(), this);
        hxpconf->show();
        ui->simulatorConfigurationLayout->addWidget(hxpconf);

        // Select correct version of XPlane
        QGCXPlaneLink* xplane = dynamic_cast<QGCXPlaneLink*>(mav->getHILSimulation());
        if (xplane)
        {
            xplane->setVersion((index == 2) ? 10 : 9);
            connect(xplane, SIGNAL(statusMessage(QString)), ui->statusLabel, SLOT(setText(QString)));
        }
    }
    else if (4)
    {
        // Ensure the sim exists and is disabled
        mav->enableHilJSBSim(false, "");
        QGCHilJSBSimConfiguration* hfgconf = new QGCHilJSBSimConfiguration(mav, this);
        hfgconf->show();
        ui->simulatorConfigurationLayout->addWidget(hfgconf);
        QGCJSBSimLink* jsb = dynamic_cast<QGCJSBSimLink*>(mav->getHILSimulation());
        if (jsb)
        {
            connect(jsb, SIGNAL(statusMessage(QString)), ui->statusLabel, SLOT(setText(QString)));
        }
    }
}
