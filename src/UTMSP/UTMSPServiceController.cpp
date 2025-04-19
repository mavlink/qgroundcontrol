/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "UTMSPServiceController.h"
#include "UTMSPLogger.h"
#include <QGCMAVLink.h>

UTMSPServiceController::UTMSPServiceController(QObject *parent):
    QObject(parent),
    _utmspNetworkRemoteIDManager(std::make_shared<Dispatcher>()),
    _activationFlag(false),
    _clientID(""),
    _clientPassword(""),
    _blenderToken(""),
    _lastLatitude(0),
    _lastLongitude(0),
    _lastAltitude(0)
{

}

UTMSPServiceController::~UTMSPServiceController()
{

}

std::string UTMSPServiceController::flightPlanAuthorization()
{
    _currentState = UTMSPFlightPlanManager::FlightState::Idle;

    //Get the Token
    _blenderToken = _utmspAuthorizaton.getOAuth2Token();
    UTMSP_LOG_INFO() << "BlenderToken" + _blenderToken;

    // State 1 --> Register
    _utmspFlightPlanManager.getFlightPlanState();
    _currentState = UTMSPFlightPlanManager::FlightState::Register;

    _coordinateList = json::array();
    for (const auto& coordinates: _boundaryPolygon)
    {
        json coordinateJson ={coordinates.latitude(),coordinates.longitude()};
        _coordinateList.push_back(coordinateJson);
    }
    _utmspFlightPlanManager.registerFlightPlan(_blenderToken, _coordinateList,_minAltitude,_maxAltitude,_startDateTime, _endDateTime);
    //TODO-> Find a correct way to assign min and max altitude for UTM mission
    auto [responseJson, flightID, flag] = _utmspFlightPlanManager.registerFlightPlanNotification();
    QString _Json = QString::fromStdString(responseJson);
    QString _flightID = QString::fromStdString(flightID);
    _responseFlightID = _flightID;
    emit responseFlightIDChanged();
    _utmspFlightPlanManager.updateFlightPlanState(_currentState);

    // State 2 --> Activate Flight Plan
    _utmspFlightPlanManager.getFlightPlanState();
    bool activated_flag = _utmspFlightPlanManager.activateFlightPlan(_blenderToken);
    if(activated_flag == true){
        _currentState = UTMSPFlightPlanManager::FlightState::Activated;
        _activationFlag = true;
        emit activationFlagChanged();
        _utmspNetworkRemoteIDManager.getCapabilty(_blenderToken);
        _utmspFlightPlanManager.updateFlightPlanState(_currentState);
        _utmspFlightPlanManager.getFlightPlanState();
    }


    return _responseFlightID.toStdString();
}
bool UTMSPServiceController::networkRemoteID(const mavlink_message_t &message,
                                             const std::string &serialNumber,
                                             const std::string &operatorID,
                                             const std::string &flightID)
{
    static uint8_t lastVehicleState = MAV_STATE_UNINIT;
    switch (message.msgid) {
    case MAVLINK_MSG_ID_HEARTBEAT: {
        mavlink_heartbeat_t heartbeat;
        mavlink_msg_heartbeat_decode(&message, &heartbeat);
        if (lastVehicleState == MAV_STATE_ACTIVE && heartbeat.system_status != MAV_STATE_ACTIVE) {
            // State 4 --> Stop telemetry when transitioning out of ACTIVE state
            bool stopFlag = _utmspNetworkRemoteIDManager.stopTelemetry();
            emit stopTelemetryFlagChanged(stopFlag);
            _streamingFlag = true;
        }
        lastVehicleState = heartbeat.system_status;
        break;
    }
    case MAVLINK_MSG_ID_GLOBAL_POSITION_INT: {
        // Only process if vehicle is in ACTIVE state
        if (lastVehicleState != MAV_STATE_ACTIVE) {
            return false;
        }
        // Rate-limit updates to 3 Hz
        if (!_timerLastSent.hasExpired(3000)) {
            return false;
        }
        _timerLastSent.restart();
        mavlink_global_position_int_t globalPosition;
        mavlink_msg_global_position_int_decode(&message, &globalPosition);
        // Update vehicle state
        _vehicleLatitude = static_cast<double>(globalPosition.lat) / 1e7;
        _vehicleLongitude = static_cast<double>(globalPosition.lon) / 1e7;
        _vehicleAltitude = static_cast<double>(globalPosition.alt) / 1000;
        _vehicleHeading = static_cast<double>(globalPosition.hdg);
        _vehicleVelocityX = globalPosition.vx / 100.f;
        _vehicleVelocityY = globalPosition.vy / 100.f;
        _vehicleVelocityZ = globalPosition.vz / 100.f;
        _vehicleRelativeAltitude = static_cast<double>(globalPosition.relative_alt) / 1000.0;
        // State 3 --> Start telemetry
        _currentState = UTMSPFlightPlanManager::FlightState::StartTelemetryStreaming;
        _utmspNetworkRemoteIDManager.startTelemetry(_vehicleLatitude,
                                                    _vehicleLongitude,
                                                    _vehicleAltitude,
                                                    _vehicleHeading,
                                                    _vehicleVelocityX,
                                                    _vehicleVelocityY,
                                                    _vehicleVelocityZ,
                                                    _vehicleRelativeAltitude,
                                                    serialNumber,
                                                    operatorID,
                                                    flightID);
        _utmspFlightPlanManager.updateFlightPlanState(_currentState);
        _streamingFlag = false;
        break;
    }

    case MAVLINK_MSG_ID_GPS_RAW_INT: {
        mavlink_gps_raw_int_t gps_raw;
        mavlink_msg_gps_raw_int_decode(&message, &gps_raw);
        if (gps_raw.eph == UINT16_MAX) {
            _lastHdop = 1.f;
        } else {
            _lastHdop = gps_raw.eph / 100.f;
        }
        break;
    }
    }

    return _streamingFlag;
}
