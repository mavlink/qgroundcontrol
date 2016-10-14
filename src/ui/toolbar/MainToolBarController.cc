/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


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
#include "ParameterManager.h"

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
        disconnect(_vehicle->parameterManager(), &ParameterManager::parameterListProgress, this, &MainToolBarController::_setProgressBarValue);
        _mav = NULL;
        _vehicle = NULL;
    }

    // Connect new system
    if (vehicle)
    {
        _vehicle = vehicle;
        _mav = vehicle->uas();
        connect(_vehicle->parameterManager(), &ParameterManager::parameterListProgress, this, &MainToolBarController::_setProgressBarValue);
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
