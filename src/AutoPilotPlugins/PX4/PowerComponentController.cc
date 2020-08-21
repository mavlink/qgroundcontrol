/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "PowerComponentController.h"
#include "QGCMAVLink.h"
#include "UAS.h"

#include <QVariant>
#include <QQmlProperty>

PowerComponentController::PowerComponentController(void)
{

}

void PowerComponentController::calibrateEsc(void)
{
    _warningMessages.clear();
    connect(_vehicle, &Vehicle::textMessageReceived, this, &PowerComponentController::_handleVehicleTextMessage);
    _vehicle->startCalibration(Vehicle::CalibrationEsc);
}

void PowerComponentController::startBusConfigureActuators(void)
{
    _warningMessages.clear();
    connect(_vehicle, &Vehicle::textMessageReceived, this, &PowerComponentController::_handleVehicleTextMessage);
    _vehicle->startUAVCANBusConfig();
}

void PowerComponentController::stopBusConfigureActuators(void)
{
    disconnect(_vehicle, &Vehicle::textMessageReceived, this, &PowerComponentController::_handleVehicleTextMessage);
    _vehicle->stopUAVCANBusConfig();
}

void PowerComponentController::_stopCalibration(void)
{
    disconnect(_vehicle, &Vehicle::textMessageReceived, this, &PowerComponentController::_handleVehicleTextMessage);
}

void PowerComponentController::_stopBusConfig(void)
{
    _stopCalibration();
}

void PowerComponentController::_handleVehicleTextMessage(int vehicleId, int /* compId */, int /* severity */, QString text)
{
    if (vehicleId != _vehicle->id()) {
        return;
    }
    
    // All calibration messages start with [cal]
    QString calPrefix("[cal] ");
    if (!text.startsWith(calPrefix)) {
        return;
    }
    text = text.right(text.length() - calPrefix.length());

    // Make sure we can understand this firmware rev
    QString calStartPrefix("calibration started: ");
    if (text.startsWith(calStartPrefix)) {
        text = text.right(text.length() - calStartPrefix.length());
        
        // Split version number and cal type
        QStringList parts = text.split(" ");
        if (parts.count() != 2) {
            emit incorrectFirmwareRevReporting();
            return;
        }
        
#if 0
        // FIXME: Cal version check is not working. Needs to be able to cancel, calibration
        
        int firmwareRev = parts[0].toInt();
        if (firmwareRev < _neededFirmwareRev) {
            emit oldFirmware();
            return;
        }
        if (firmwareRev > _neededFirmwareRev) {
            emit newerFirmware();
            return;
        }
#endif
    }

    if (text == "Connect battery now") {
        emit connectBattery();
        return;
    }
    
    if (text == "Battery connected") {
        emit batteryConnected();
        return;
    }

    
    QString failedPrefix("calibration failed: ");
    if (text.startsWith(failedPrefix)) {
        QString failureText = text.right(text.length() - failedPrefix.length());
        if (failureText.startsWith("Disconnect battery")) {
            emit disconnectBattery();
            return;
        }
        
        _stopCalibration();
        emit calibrationFailed(text.right(text.length() - failedPrefix.length()));
        return;
    }
    
    QString calCompletePrefix("calibration done:");
    if (text.startsWith(calCompletePrefix)) {
        _stopCalibration();
        emit calibrationSuccess(_warningMessages);
        return;
    }
    
    QString warningPrefix("config warning: ");
    if (text.startsWith(warningPrefix)) {
        _warningMessages << text.right(text.length() - warningPrefix.length());
    }

    QString busFailedPrefix("bus conf fail:");
    if (text.startsWith(busFailedPrefix)) {

        _stopBusConfig();
        emit calibrationFailed(text.right(text.length() - failedPrefix.length()));
        return;
    }

    if (text.startsWith(calCompletePrefix)) {
        _stopBusConfig();
        emit calibrationSuccess(_warningMessages);
        return;
    }

    QString busWarningPrefix("bus conf warn: ");
    if (text.startsWith(busWarningPrefix)) {
        _warningMessages << text.right(text.length() - warningPrefix.length());
    }
}
