/****************************************************************************
 *
 * (c) 2023 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "Vehicle.h"

#include <QQmlApplicationEngine>

#include "UTMSPVehicle.h"
#include "UTMSPLogger.h"

UTMSPVehicle::UTMSPVehicle(std::shared_ptr<Dispatcher> dispatcher, const Vehicle& vehicle):
    _dispatcher(dispatcher),
    _remoteIDFlag(false),
    _stopFlag(false),
    _flightID(""),
    _vehicleSerialNumber(""),
    _vehicleActivation(false)
{
    UTMSP_LOG_INFO() << "UTMSPManagerLog: UTMSPVehicle Contructor";
    connect(&vehicle, &Vehicle::mavlinkMessageReceived, this, &UTMSPVehicle::triggerNetworkRemoteID);
    UTMSP_LOG_INFO() << "UTMSPManagerLog: UTMSPVehicle MAvlink msg slot connected";
    _aircraftModel = _utmspAircraft.aircraftModel();
    _aircraftClass = _utmspAircraft.aircraftClass();
    _operatorID    = _utmspOperator.operatorID();
    _operatorClass = _utmspOperator.operatorClass();
}

void UTMSPVehicle::loadTelemetryFlag(bool value){
    _remoteIDFlag = value;
}

void UTMSPVehicle::triggerFlightAuthorization()
{
    UTMSP_LOG_INFO() << "Registration process Initiated Successfully";
    _flightID = flightPlanAuthorization();
}

void UTMSPVehicle::triggerNetworkRemoteID(const mavlink_message_t &message)
{
    _aircraftType = _utmspAircraft.aircraftType(message);
    _aircraftSerialNumber = _utmspAircraft.aircraftSerialNo(message);

    if(_aircraftSerialNumber != "0")
    {
        _vehicleSerialNumber = QString::fromStdString(_aircraftSerialNumber);
        emit vehicleSerialNumberChanged();
    }

    if (_remoteIDFlag) {
        if (_stopFlag) {
            _utmspFlightPlanManager.updateFlightPlanState(UTMSPFlightPlanManager::FlightState::StopTelemetryStreaming);
            _remoteIDFlag = false;
        } else {
            _stopFlag = networkRemoteID(message, _aircraftSerialNumber, _operatorID, _flightID);
        }
    }
}

void UTMSPVehicle::triggerActivationStatusBar(bool flag){
    _vehicleActivation = flag;
    emit vehicleActivationChanged();
}
