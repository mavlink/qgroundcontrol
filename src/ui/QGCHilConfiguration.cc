/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#include <QSettings>

#include "QGCHilConfiguration.h"
#include "ui_QGCHilConfiguration.h"

#include "QGCHilFlightGearConfiguration.h"
#include "QGCHilJSBSimConfiguration.h"
#include "QGCHilXPlaneConfiguration.h"
#include "UAS.h"

QGCHilConfiguration::QGCHilConfiguration(Vehicle* vehicle, QWidget *parent)
    : QWidget(parent)
    , _vehicle(vehicle)
    , ui(new Ui::QGCHilConfiguration)
{
    ui->setupUi(this);

    // XXX its quite wrong that this is implicitely a factory
    // class, but this is something to clean up for later.

    QSettings settings;
    settings.beginGroup("QGC_HILCONFIG");
    int i = settings.value("SIMULATOR_INDEX", -1).toInt();

    if (i > 0) {
//        ui->simComboBox->blockSignals(true);
        ui->simComboBox->setCurrentIndex(i);
//        ui->simComboBox->blockSignals(false);
        on_simComboBox_currentIndexChanged(i);
    }

    settings.endGroup();
}

void QGCHilConfiguration::receiveStatusMessage(const QString& message)
{
    ui->statusLabel->setText(message);
}

QGCHilConfiguration::~QGCHilConfiguration()
{
    QSettings settings;
    settings.beginGroup("QGC_HILCONFIG");
    settings.setValue("SIMULATOR_INDEX", ui->simComboBox->currentIndex());
    settings.endGroup();
    delete ui;
}

void QGCHilConfiguration::setVersion(QString version)
{
    Q_UNUSED(version);
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
        _vehicle->uas()->enableHilFlightGear(false, "", true, this);
        QGCHilFlightGearConfiguration* hfgconf = new QGCHilFlightGearConfiguration(_vehicle, this);
        hfgconf->show();
        ui->simulatorConfigurationLayout->addWidget(hfgconf);
        QGCFlightGearLink* fg = dynamic_cast<QGCFlightGearLink*>(_vehicle->uas()->getHILSimulation());
        if (fg)
        {
            connect(fg, &QGCFlightGearLink::statusMessage, ui->statusLabel, &QLabel::setText);
        }

    }
    else if (2 == index || 3 == index)
    {
        // Ensure the sim exists and is disabled
        _vehicle->uas()->enableHilXPlane(false);
        QGCHilXPlaneConfiguration* hxpconf = new QGCHilXPlaneConfiguration(_vehicle->uas()->getHILSimulation(), this);
        hxpconf->show();
        ui->simulatorConfigurationLayout->addWidget(hxpconf);

        // Select correct version of XPlane
        QGCXPlaneLink* xplane = dynamic_cast<QGCXPlaneLink*>(_vehicle->uas()->getHILSimulation());
        if (xplane)
        {
            xplane->setVersion((index == 2) ? 10 : 9);
            connect(xplane, &QGCXPlaneLink::statusMessage, ui->statusLabel, &QLabel::setText);
        }
    }
// Disabling JSB Sim since its not well maintained,
// but as refactoring is pending we're not ditching the code yet
//    else if (4)
//    {
//        // Ensure the sim exists and is disabled
//        _vehicle->uas()->enableHilJSBSim(false, "");
//        QGCHilJSBSimConfiguration* hfgconf = new QGCHilJSBSimConfiguration(_vehicle, this);
//        hfgconf->show();
//        ui->simulatorConfigurationLayout->addWidget(hfgconf);
//        QGCJSBSimLink* jsb = dynamic_cast<QGCJSBSimLink*>(_vehicle->uas()->getHILSimulation());
//        if (jsb)
//        {
//            connect(jsb, SIGNAL(statusMessage(QString)), ui->statusLabel, SLOT(setText(QString)));
//        }
//    }
}
