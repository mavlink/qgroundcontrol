/*=====================================================================

PIXHAWK Micro Air Vehicle Flying Robotics Toolkit

(c) 2009 PIXHAWK PROJECT  <http://pixhawk.ethz.ch>

This file is part of the PIXHAWK project

    PIXHAWK is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    PIXHAWK is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with PIXHAWK. If not, see <http://www.gnu.org/licenses/>.

======================================================================*/

/**
 * @file
 *   @brief Brief Description
 *
 *   @author Lorenz Meier <mavteam@student.ethz.ch>
 *
 */

#include <float.h>
#include <UASInfoWidget.h>
#include <UASManager.h>
#include <MG.h>
#include <QTimer>
#include <QDir>
#include <cstdlib>
#include <cmath>

#include <QDebug>

UASInfoWidget::UASInfoWidget(QWidget *parent, QString name) : QWidget(parent)
{
    ui.setupUi(this);
    this->name = name;

    connect(UASManager::instance(), SIGNAL(activeUASSet(UASInterface*)), this, SLOT(setActiveUAS(UASInterface*)));

    activeUAS = NULL;

    //instruments = new QMap<QString, QProgressBar*>();

    // Set default battery type
    //    setBattery(0, LIPOLY, 3);
    startTime = MG::TIME::getGroundTimeNow();
    //    startVoltage = 0.0f;

    //    lastChargeLevel = 0.5f;
    //    lastRemainingTime = 1;

    // Set default values
    /** Set two voltage decimals and zero charge level decimals **/
    this->voltageDecimals     = 2;
    this->loadDecimals = 2;

    this->voltage = 0;
    this->chargeLevel = 0;
    this->load = 0;
    receiveLoss = 0;
    sendLoss = 0;

    updateTimer = new QTimer(this);
    connect(updateTimer, SIGNAL(timeout()), this, SLOT(refresh()));
    updateTimer->start(50);

}

UASInfoWidget::~UASInfoWidget()
{

}

void UASInfoWidget::addUAS(UASInterface* uas)
{
    if (uas != NULL)
    {
        connect(uas, SIGNAL(batteryChanged(UASInterface*,double,double,int)), this, SLOT(updateBattery(UASInterface*,double,double,int)));
        connect(uas, SIGNAL(dropRateChanged(int,float,float)), this, SLOT(updateDropRate(int,float,float)));
        connect(uas, SIGNAL(loadChanged(UASInterface*, double)), this, SLOT(updateCPULoad(UASInterface*,double)));

        // Set this UAS as active if it is the first one
        if (activeUAS == 0) activeUAS = uas;
    }
}

void UASInfoWidget::setActiveUAS(UASInterface* uas)
{
    activeUAS = uas;
}

void UASInfoWidget::updateBattery(UASInterface* uas, double voltage, double percent, int seconds)
{
    setVoltage(uas, voltage);
    setChargeLevel(uas, percent);
    setTimeRemaining(uas, seconds);
}

/**
 *
 */
void UASInfoWidget::updateCPULoad(UASInterface* uas, double load)
{
    if (activeUAS == uas)
    {
        this->load = load*100.0f;
    }
}

void UASInfoWidget::updateReceiveLoss(float receiveLoss)
{
    this->receiveLoss = this->receiveLoss * 0.8f + receiveLoss * 0.2f;
}

void UASInfoWidget::updateSendLoss(float sendLoss)
{
    this->sendLoss = this->sendLoss * 0.8f + sendLoss * 0.2f;
}

void UASInfoWidget::setVoltage(UASInterface* uas, double voltage)
{
    Q_UNUSED(uas);
    this->voltage = voltage;
}

void UASInfoWidget::setChargeLevel(UASInterface* uas, double chargeLevel)
{
    if (activeUAS == uas)
    {
        this->chargeLevel = chargeLevel;
    }
}

void UASInfoWidget::setTimeRemaining(UASInterface* uas, double seconds)
{
    if (activeUAS == uas)
    {
        this->timeRemaining = seconds;
    }
}

void UASInfoWidget::refresh()
{
    ui.voltageLabel->setText(QString::number(this->voltage, 'f', voltageDecimals));
    ui.batteryBar->setValue(static_cast<int>(this->chargeLevel));

    ui.loadLabel->setText(QString::number(this->load, 'f', loadDecimals));
    ui.loadBar->setValue(static_cast<int>(this->load));

    ui.receiveLossBar->setValue(receiveLoss);
    ui.receiveLossLabel->setText(QString::number(receiveLoss,'f', 2));

    ui.sendLossBar->setValue(sendLoss);
    ui.sendLossLabel->setText(QString::number(sendLoss, 'f', 2));
}
