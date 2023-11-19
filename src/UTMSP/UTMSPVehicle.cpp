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
    _dispatcher(dispatcher)
{
    UTMSP_LOG_INFO() << "UTMSPManagerLog: UTMSPVehicle Contructor";
    connect(&vehicle, &Vehicle::mavlinkMessageReceived, this, &UTMSPVehicle::triggerNetworkRemoteID);
    UTMSP_LOG_INFO() << "UTMSPManagerLog: UTMSPVehicle MAvlink msg slot connected";
    _aircraftModel = aircraftModel();
    _aircraftClass = aircraftClass();
    _operatorID = operatorID();
    _operatorClass = operatorClass();
}

bool        UTMSPVehicle::_remoteIDFlag = false;
bool        UTMSPVehicle::_stopFlag;
std::string UTMSPVehicle::_flightID;
QString     UTMSPVehicle::_vehicleSerialNumber;
bool        UTMSPVehicle::_vehicleActivation;

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
    _aircraftType = aircraftType(message);
    _aircraftSerialNumber = aircraftSerialNo(message);
    _i++;

    if(_i>20 && _i<22) // First 20 Mavlink message are NIL
    {
        _vehicleSerialNumber = QString::fromStdString(_aircraftSerialNumber);
        UTMSP_LOG_DEBUG() << "Serial Number" << _vehicleSerialNumber;
        UTMSP_LOG_DEBUG() << "Aircraft Type" << _aircraftType;
        emit vehicleSerialNumberChanged();
    }

    if (_remoteIDFlag) {
        if (_stopFlag) {
            updateFlightPlanState(UTMSPFlightPlanManager::FlightState::StopTelemetryStreaming);
            _remoteIDFlag = false;
        } else {
            _stopFlag = networkRemoteID(message, _aircraftSerialNumber, _operatorID, _flightID);
        }
    }
}

void UTMSPVehicle::triggerUploadButton(bool flag){
    _vehicleActivation = flag;
    emit vehicleActivationChanged();
}
