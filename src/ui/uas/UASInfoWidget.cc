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
    //    connect(uas, SIGNAL(voltageChanged(int, double)), this, SLOT(setVoltage(int, double)));
    connect(uas, SIGNAL(batteryChanged(UASInterface*,double,double,int)), this, SLOT(updateBattery(UASInterface*,double,double,int)));
    connect(uas, SIGNAL(valueChanged(int,QString,double,quint64)), this, SLOT(valueChanged(int,QString,double,quint64)));
    connect(uas, SIGNAL(actuatorChanged(UASInterface*,int,double)), this, SLOT(actuatorChanged(UASInterface*,int,double)));
    connect(uas, SIGNAL(loadChanged(UASInterface*, double)), this, SLOT(updateCPULoad(UASInterface*,double)));

    // Set this UAS as active if it is the first one
    if (activeUAS == 0) activeUAS = uas;
    }
}

void UASInfoWidget::setActiveUAS(UASInterface* uas)
{
    activeUAS = uas;
}

void UASInfoWidget::addInstrument(QString key, double min, double max, double initial, QString unit)
{

}

void UASInfoWidget::valueChanged(int uasid, QString key, double value,quint64 time)
{

}

void UASInfoWidget::actuatorChanged(UASInterface* uas, int actId, double value)
{
    if (activeUAS == uas)
    {
        switch (actId)
        {
        case 0:
            ui.topRotorLabel->setText(QString::number(value*3300, 'f', 2));
            ui.topRotorBar->setValue(value * 100);
            break;
        case 1:
            ui.botRotorLabel->setText(QString::number(value*3300, 'f', 2));
            ui.botRotorBar->setValue(value * 100);
            break;
        case 2:
            ui.leftServoLabel->setText(QString::number(value*57.2957795f, 'f', 2));
            ui.leftServoBar->setValue((value * 50.0f) + 50);
            break;
        case 3:
            ui.rightServoLabel->setText(QString::number(value*57.2957795f, 'f', 2));
            ui.rightServoBar->setValue((value * 50.0f) + 50);
            break;
        }
    }
}

void UASInfoWidget::updateBattery(UASInterface* uas, double voltage, double percent, int seconds)
{
    setVoltage(uas, voltage);
    setChargeLevel(uas, percent);
    setTimeRemaining(uas, seconds);
}

void UASInfoWidget::updateCPULoad(UASInterface* uas, double load)
{
    if (activeUAS == uas)
    {
        this->load = load;
    }
}

//void UASInfoWidget::setBattery(int uasid, BatteryType type, int cells)
//{
//    this->batteryType = type;
//    this->cells = cells;
//    switch (batteryType)
//    {
//            case NICD:
//        break;
//            case NIMH:
//        break;
//            case LIION:
//        break;
//            case LIPOLY:
//        fullVoltage = this->cells * 4.18;
//        emptyVoltage = this->cells * 3.4;
//        break;
//            case LIFE:
//        break;
//            case AGZN:
//        break;
//    }
//}

//double UASInfoWidget::calculateTimeRemaining() {
//    quint64 dt = MG::TIME::getGroundTimeNow() - startTime;
//    double seconds = dt / 1000.0f;
//    double voltDifference = startVoltage - currentVoltage;
//    if (voltDifference <= 0) voltDifference = 0.00000000001f;
//    double dischargePerSecond = voltDifference / seconds;
//    double remaining = (currentVoltage - emptyVoltage) / dischargePerSecond;
//    // Can never be below 0
//    if (remaining <= 0) remaining = 0.0000000000001f;
//    return remaining;
//}

void UASInfoWidget::setVoltage(UASInterface* uas, double voltage)
{
    this->voltage = voltage;
}

//void UASInfoWidget::setVoltage(int uasid, double voltage)
//{
//    // Read and update data
//    currentVoltage = voltage;
//    if (startVoltage == 0) startVoltage = currentVoltage;
//    // This is a low pass filter to get smoother results: (0.8 * lastChargeLevel) + (0.2 * chargeLevel)
//    double chargeLevel = (currentVoltage - emptyVoltage)/(fullVoltage - emptyVoltage);
//    lastChargeLevel = (0.6 * lastChargeLevel) + (0.4 * chargeLevel);
//
//    lastRemainingTime = calculateTimeRemaining();
//
//    ui.voltageLabel->setText(QString::number(currentVoltage, 'f', voltageDecimals));
//    setChargeLevel(0, lastChargeLevel * 100);
//    setTimeRemaining(0, lastRemainingTime);
//}

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

//    if(this->timeRemaining > 1 && this->timeRemaining < MG::MAX_FLIGHT_TIME)
//    {
//        // Filter output to get a higher stability
//        static int filterTime = static_cast<int>(this->timeRemaining);
//        //filterTime = 0.8 * filterTime + 0.2 * static_cast<int>(this->timeRemaining);
//
//        int hours = filterTime % (60 * 60);
//        int min = (filterTime - hours * 60) % 60;
//        int sec = (filterTime - hours * 60 - min * 60);
//        QString timeText;
//        timeText = timeText.sprintf("%02d:%02d:%02d", hours, min, sec);
//        ui.voltageTimeEstimateLabel->setText(timeText);
//    } else {
//        ui.voltageTimeEstimateLabel->setText(tr("Calculating"));
//    }
}
