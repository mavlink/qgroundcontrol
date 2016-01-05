/*=====================================================================

QGroundControl Open Source Ground Control Station

(c) 2009, 2015 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

This file is part of the QGROUNDCONTROL project

    QGROUNDCONTROL is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    QGROUNDCONTROL is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.

======================================================================*/

/**
 * @file
 *   @brief QGC Main Tool Bar
 *   @author Gus Grubba <mavlink@grubba.com>
 */

#include <QQmlContext>
#include <QQmlEngine>

#include "MainToolBarController.h"
#include "ScreenToolsController.h"
#include "UASMessageView.h"
#include "UASMessageHandler.h"
#include "QGCApplication.h"
#include "MultiVehicleManager.h"
#include "UAS.h"

MainToolBarController::MainToolBarController(QObject* parent)
    : QObject(parent)
    , _vehicle(NULL)
    , _mav(NULL)
    , _progressBarValue(0.0f)
    , _telemetryRRSSI(0)
    , _telemetryLRSSI(0)
{
    _activeVehicleChanged(qgcApp()->toolbox()->multiVehicleManager()->activeVehicle());
    connect(qgcApp()->toolbox()->mavlinkProtocol(),     &MAVLinkProtocol::radioStatusChanged, this, &MainToolBarController::_telemetryChanged);
    connect(qgcApp()->toolbox()->multiVehicleManager(), &MultiVehicleManager::activeVehicleChanged, this, &MainToolBarController::_activeVehicleChanged);
}

MainToolBarController::~MainToolBarController()
{

}

void MainToolBarController::_activeVehicleChanged(Vehicle* vehicle)
{
    // Disconnect the previous one (if any)
    if (_vehicle) {
        disconnect(_vehicle->autopilotPlugin(), &AutoPilotPlugin::parameterListProgress, this, &MainToolBarController::_setProgressBarValue);
        _mav = NULL;
        _vehicle = NULL;
    }

    // Connect new system
    if (vehicle)
    {
        _vehicle = vehicle;
        _mav = vehicle->uas();
        connect(_vehicle->autopilotPlugin(), &AutoPilotPlugin::parameterListProgress, this, &MainToolBarController::_setProgressBarValue);
    }
}

void MainToolBarController::_telemetryChanged(LinkInterface*, unsigned rxerrors, unsigned fixed, int rssi, int remrssi, unsigned txbuf, unsigned noise, unsigned remnoise)
{
    if(_telemetryLRSSI != rssi) {
        _telemetryLRSSI = rssi;
        emit telemetryLRSSIChanged(_telemetryLRSSI);
    }
    if(_telemetryRRSSI != remrssi) {
        _telemetryRRSSI = remrssi;
        emit telemetryRRSSIChanged(_telemetryRRSSI);
    }
    if(_telemetryRXErrors != rxerrors) {
        _telemetryRXErrors = rxerrors;
        emit telemetryRXErrorsChanged(_telemetryRXErrors);
    }
    if(_telemetryFixed != fixed) {
        _telemetryFixed = fixed;
        emit telemetryFixedChanged(_telemetryFixed);
    }
    if(_telemetryTXBuffer != txbuf) {
        _telemetryTXBuffer = txbuf;
        emit telemetryTXBufferChanged(_telemetryTXBuffer);
    }
    if(_telemetryLNoise != noise) {
        _telemetryLNoise = noise;
        emit telemetryLNoiseChanged(_telemetryLNoise);
    }
    if(_telemetryRNoise != remnoise) {
        _telemetryRNoise = remnoise;
        emit telemetryRNoiseChanged(_telemetryRNoise);
    }
}

void MainToolBarController::_setProgressBarValue(float value)
{
    _progressBarValue = value;
    emit progressBarValueChanged(value);
}
