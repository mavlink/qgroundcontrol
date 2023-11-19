/****************************************************************************
 *
 * (c) 2023 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "UTMSPServiceController.h"
#include "UTMSPLogger.h"

UTMSPServiceController::UTMSPServiceController(QObject *parent):
    QObject(parent),
    UTMSPNetworkRemoteIDManager(std::make_shared<Dispatcher>())
{

}

UTMSPServiceController::~UTMSPServiceController()
{

}

QString                 UTMSPServiceController::_responseJson;
QString                 UTMSPServiceController::_responseFlightID;
bool                    UTMSPServiceController::_responseFlag;
bool                    UTMSPServiceController::_activationFlag;
std::string             UTMSPServiceController::_clientID;
std::string             UTMSPServiceController::_clientPassword;
std::string             UTMSPServiceController::_blenderToken;
QList<QGeoCoordinate>   UTMSPServiceController::_boundaryPolygon;
std::string             UTMSPServiceController::_startDateTime;
std::string             UTMSPServiceController::_endDateTime;
int                     UTMSPServiceController::_minAltitude;
int                     UTMSPServiceController::_maxAltitude;
double                  UTMSPServiceController::_lastLatitude;
double                  UTMSPServiceController::_lastLongitude;
double                  UTMSPServiceController::_lastAltitude = 0;

std::string UTMSPServiceController::flightPlanAuthorization()
{
    _currentState = getFlightPlanState();

    //Get the Token
    _blenderToken = _utmspAuth.getOAuth2Token();

    // State 1 --> Register
    getFlightPlanState();
    _currentState = UTMSPFlightPlanManager::FlightState::Register;

    _coordinateList = json::array();
    for (const auto& coordinates: _boundaryPolygon)
    {
        json coordinateJson ={coordinates.latitude(),coordinates.longitude()};
        _coordinateList.push_back(coordinateJson);
    }
    registerFlightPlan(_blenderToken, _coordinateList,_minAltitude,_maxAltitude,_startDateTime, _endDateTime);
    //TODO-> Find a correct way to assign min and max altitude for UTM mission
    auto [responseJson, flightID, flag] = registerFlightPlanNotification();
    QString _Json = QString::fromStdString(responseJson);
    QString _flightID = QString::fromStdString(flightID);
    _responseFlightID = _flightID;
    emit responseFlightIDChanged();
    _responseJson = _Json;
    emit responseJsonChanged();
    _responseFlag = flag;
    emit responseFlagChanged();
    updateFlightPlanState(_currentState);

    // State 2 --> Activate Flight Plan
    getFlightPlanState();
    if(flag == true){
        _currentState = UTMSPFlightPlanManager::FlightState::Activated;
        _activationFlag = true;
        emit activationFlagChanged();
        getCapabilty(_blenderToken);
        updateFlightPlanState(_currentState);
        getFlightPlanState();
    }


    return _responseFlightID.toStdString();
}
bool UTMSPServiceController::networkRemoteID(const mavlink_message_t& message,
                                             const std::string &serialNumber,
                                             const std::string &operatorID,
                                             const std::string &flightID)
{
    double vehicleLatitude;
    double vehicleLongitude;
    double lastLatitude;
    double lastLongitude;

    switch (message.msgid)
    {
    case MAVLINK_MSG_ID_GLOBAL_POSITION_INT:

        // rate-limit updates to 3 Hz
        if (!_timerLastSent.hasExpired(3000)) {
            return false;
        }
        _timerLastSent.restart();

        mavlink_global_position_int_t globalPosition;
        mavlink_msg_global_position_int_decode(&message, &globalPosition);
        _vehicleLatitude         = (static_cast<double>(globalPosition.lat /1e7));
        _vehicleLongitude        = (static_cast<double>(globalPosition.lon / 1e7));
        _vehicleAltitude         = static_cast<double>(globalPosition.alt) / 1000;
        _vehicleHeading          = static_cast<double>(globalPosition.hdg);
        _vehicleVelocityX        = (globalPosition.vx / 100.f);
        _vehicleVelocityY        = (globalPosition.vy / 100.f);
        _vehicleVelocityZ        = (globalPosition.vz / 100.f);
        _vehicleRelativeAltitude = static_cast<double>(globalPosition.relative_alt) / 1000.0;

        vehicleLatitude = std::round(_vehicleLatitude* 1e4)/ 1e4;
        vehicleLongitude = std::round(_vehicleLongitude* 1e4)/ 1e4;
        lastLatitude = std::round(_lastLatitude* 1e4)/ 1e4;
        lastLongitude = std::round(_lastLongitude* 1e4)/ 1e4;

        if((std::abs(vehicleLatitude-lastLatitude) > 0.0001)  ||  (std::abs(vehicleLongitude-lastLongitude) > 0.0001))
        {
            // State 3 --> Start telemetry
            _currentState = UTMSPFlightPlanManager::FlightState::StartTelemetryStreaming;
            startTelemetry(_vehicleLatitude, _vehicleLongitude, _vehicleAltitude, _vehicleHeading, _vehicleVelocityX, _vehicleVelocityY, _vehicleVelocityZ, _vehicleRelativeAltitude, serialNumber, operatorID, flightID);
            updateFlightPlanState(_currentState);
            _streamingFlag = false;
        }
        else
        {
            // State 4 --> Stop telemetry
            bool stopflag = stopTelemetry();
            emit stopTelemetryFlagChanged(stopflag);
            _streamingFlag = true;
        }
        break;

    case MAVLINK_MSG_ID_GPS_RAW_INT:
        mavlink_gps_raw_int_t gps_raw;
        mavlink_msg_gps_raw_int_decode(&message, &gps_raw);
        if (gps_raw.eph == UINT16_MAX) {
            _lastHdop = 1.f;
        } else {
            _lastHdop = gps_raw.eph / 100.f;
        }
        break;
    }

    return _streamingFlag;
}
