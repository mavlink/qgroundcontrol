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
 *   @brief Implementation of class UASInfoWidget
 *
 *   @author Lorenz Meier <mavteam@student.ethz.ch>
 *
 */

#include <QtGlobal>
#include <QTimer>
#include <QDir>
#include <QDebug>

#include <float.h>
#include <cstdlib>
#include <cmath>

#include "UASInfoWidget.h"
#include "MultiVehicleManager.h"
#include "QGC.h"
#include "UAS.h"
#include "QGCApplication.h"

UASInfoWidget::UASInfoWidget(const QString& title, QAction* action, QWidget *parent, QString name)
    : QGCDockWidget(title, action, parent)
{
    ui.setupUi(this);
    this->name = name;
    activeUAS = NULL;

    connect(qgcApp()->toolbox()->multiVehicleManager(), &MultiVehicleManager::activeVehicleChanged, this, &UASInfoWidget::_activeVehicleChanged);
    _activeVehicleChanged(qgcApp()->toolbox()->multiVehicleManager()->activeVehicle());

    startTime = QGC::groundTimeMilliseconds();

    // Set default values
    /** Set two voltage decimals and zero charge level decimals **/
    this->voltageDecimals = 2;
    this->loadDecimals = 2;

    this->voltage = 0;
    this->chargeLevel = 0;
    this->load = 0;
    receiveLoss = 0;
    sendLoss = 0;
    changed = true;
    errors = QMap<QString, int>();

    updateTimer = new QTimer(this);
    connect(updateTimer, &QTimer::timeout, this, &UASInfoWidget::refresh);
    updateTimer->start(updateInterval);

    this->setVisible(false);
    
    loadSettings();
}

UASInfoWidget::~UASInfoWidget()
{

}

void UASInfoWidget::showEvent(QShowEvent* event)
{
    // React only to internal (pre-display)
    // events
    Q_UNUSED(event);
    updateTimer->start(updateInterval);
}

void UASInfoWidget::hideEvent(QHideEvent* event)
{
    // React only to internal (pre-display)
    // events
    Q_UNUSED(event);
    updateTimer->stop();
}

void UASInfoWidget::_activeVehicleChanged(Vehicle* vehicle)
{
    if (activeUAS) {
        disconnect(activeUAS, &UASInterface::batteryChanged, this, &UASInfoWidget::updateBattery);
        disconnect(activeUAS, &UASInterface::dropRateChanged, this, &UASInfoWidget::updateReceiveLoss);
        disconnect(static_cast<UAS*>(activeUAS), &UAS::loadChanged, this, &UASInfoWidget::updateCPULoad);
        disconnect(activeUAS, &UASInterface::errCountChanged, this, &UASInfoWidget::updateErrorCount);
        activeUAS = NULL;
    }
    
    if (vehicle) {
        activeUAS = vehicle->uas();
        connect(activeUAS, &UASInterface::batteryChanged, this, &UASInfoWidget::updateBattery);
        connect(activeUAS, &UASInterface::dropRateChanged, this, &UASInfoWidget::updateReceiveLoss);
        connect(static_cast<UAS*>(activeUAS), &UAS::loadChanged, this, &UASInfoWidget::updateCPULoad);
        connect(activeUAS, &UASInterface::errCountChanged, this, &UASInfoWidget::updateErrorCount);
    }
}

void UASInfoWidget::updateBattery(UASInterface* uas, double voltage, double current, double percent, int seconds)
{
    Q_UNUSED(current)
    setVoltage(uas, voltage);
    setChargeLevel(uas, percent);
    setTimeRemaining(uas, seconds);
}

void UASInfoWidget::updateErrorCount(int uasid, QString component, QString device, int count)
{
    //qDebug() << __FILE__ << __LINE__ << activeUAS->getUASID() << "=" << uasid;
    if (activeUAS->getUASID() == uasid) {
        errors.remove(component + ":" + device);
        errors.insert(component + ":" + device, count);
    }
}

/**
 *
 */
void UASInfoWidget::updateCPULoad(UASInterface* uas, double load)
{
    if (activeUAS == uas) {
        this->load = load;
    }
}

void UASInfoWidget::updateReceiveLoss(int uasId, float receiveLoss)
{
    Q_UNUSED(uasId);
    this->receiveLoss = this->receiveLoss * 0.8f + receiveLoss * 0.2f;
}

/**
  The send loss is typically calculated on the GCS based on packets
  that were received scrambled from the MAV
 */
void UASInfoWidget::updateSendLoss(int uasId, float sendLoss)
{
    Q_UNUSED(uasId);
    this->sendLoss = this->sendLoss * 0.8f + sendLoss * 0.2f;
}

void UASInfoWidget::setVoltage(UASInterface* uas, double voltage)
{
    Q_UNUSED(uas);
    this->voltage = voltage;
}

void UASInfoWidget::setChargeLevel(UASInterface* uas, double chargeLevel)
{
    if (activeUAS == uas) {
        this->chargeLevel = chargeLevel;
    }
}

void UASInfoWidget::setTimeRemaining(UASInterface* uas, double seconds)
{
    if (activeUAS == uas) {
        this->timeRemaining = seconds;
    }
}

void UASInfoWidget::refresh()
{
    ui.voltageLabel->setText(QString::number(this->voltage, 'f', voltageDecimals));
    ui.batteryBar->setValue(qMax(0,qMin(static_cast<int>(this->chargeLevel), 100)));

    ui.loadLabel->setText(QString::number(this->load, 'f', loadDecimals));
    ui.loadBar->setValue(qMax(0, qMin(static_cast<int>(this->load), 100)));

    ui.receiveLossBar->setValue(qMax(0, qMin(static_cast<int>(receiveLoss), 100)));
    ui.receiveLossLabel->setText(QString::number(receiveLoss, 'f', 2));

    ui.sendLossBar->setValue(sendLoss);
    ui.sendLossLabel->setText(QString::number(sendLoss, 'f', 2));

    ui.label_5->setText(QString::number(this->load, 'f', loadDecimals));
    ui.progressBar->setValue(qMax(0, qMin(static_cast<int>(this->load), 100)));

    QString errorString;
    QMapIterator<QString, int> i(errors);
    while (i.hasNext()) {
        i.next();
        errorString += QString(i.key() + ": %1 ").arg(i.value());

        // FIXME
        errorString.replace("IMU:", "");


    }
    ui.errorLabel->setText(errorString);
}
