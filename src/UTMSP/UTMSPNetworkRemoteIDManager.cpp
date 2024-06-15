/****************************************************************************
 *
 * (c) 2023 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "UTMSPNetworkRemoteIDManager.h"
#include "UTMSPLogger.h"

UTMSPNetworkRemoteIDManager::UTMSPNetworkRemoteIDManager(std::shared_ptr<Dispatcher> dispatcher):
    _dispatcher(dispatcher)
{

}

UTMSPNetworkRemoteIDManager::~UTMSPNetworkRemoteIDManager()
{

}

void UTMSPNetworkRemoteIDManager::getCapabilty(const std::string &token)
{
    setBearerToken(token.c_str());
}

void UTMSPNetworkRemoteIDManager::startTelemetry(const double &latitude,
                                                 const double &longitude,
                                                 const double &altitude,
                                                 const double &heading,
                                                 const double &velocityX,
                                                 const double &velocityY,
                                                 const double &velocityZ,
                                                 const double &relativeAltitude,
                                                 const std::string &aircraftSerialNumber,
                                                 const std::string &operatorID,
                                                 const std::string &flightID)
{
    // Generate RID JSON
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    std::ostringstream oss;

    if(_j==0){
        _initLatitude = latitude;
        _initLongitude = longitude;
        _j++;
    }
    _ridData = {};
    _ridData.timestamp_accuracy = 0.0;
    _ridData.position.accuracy_h = "HAUnknown";
    _ridData.position.accuracy_v = "VAUnknown";
    _ridData.position.extrpolated = true;
    _ridData.speed_accuracy = "SAUnknown";
    _ridData.height.reference = "TakeoffLocation";
    _ridData.operational_status = "Undeclared";
    oss << std::put_time(std::gmtime(&now_c), "%FT%T") << "Z";
    _ridData.timestamp.value     = oss.str();
    _ridData.timestamp.format    = "RFC3339";
    _ridData.position.lat        = latitude;
    _ridData.position.lng        = longitude;
    _ridData.position.alt        = altitude / 1000;
    _ridData.track               = heading;
    _ridData.speed               = std::sqrt(velocityX*velocityX + velocityY*velocityY);
    _ridData.vertical_speed      = velocityZ;
    _ridData.height.distance     = relativeAltitude;

    this->_flightDetails.rid_details.id        = flightID;
    this->_flightDetails.rid_details.operator_id = operatorID;
    this->_flightDetails.rid_details.operation_description = "Delivery operation, see more details at https://deliveryops.com/operation";
    this->_flightDetails.eu_classification.category = "EUCategoryUndefined";
    this->_flightDetails.eu_classification.clasS = "EUClassUndefined";
    this->_flightDetails.uas_id.serial_number = aircraftSerialNumber;
    this->_flightDetails.uas_id.registration_number =operatorID;
    this->_flightDetails.uas_id.utm_id = aircraftSerialNumber; //TODO->Will be taken care as part of registration service integration
    this->_flightDetails.uas_id.specific_session_id = "Unknown";
    this->_flightDetails.operator_location_lng = _initLongitude;
    this->_flightDetails.operator_location_lat = _initLatitude;
    this->_flightDetails.operator_location_accuracyH ="HAUnknown";
    this->_flightDetails.operator_location_accuracyV ="VAUnknown";
    this->_flightDetails.auth_data_format      = "string";
    this->_flightDetails.auth_data_data        = 0; //TODO->Needs to be updated
    this->_flightDetails.serial_number         = aircraftSerialNumber;
    this->_flightDetails.registration_number   = operatorID;

    _flightDetailsJson["rid_details"]["id"]                      = this->_flightDetails.rid_details.id;
    _flightDetailsJson["rid_details"]["operator_id"]             = this->_flightDetails.rid_details.operator_id;
    _flightDetailsJson["rid_details"]["operation_description"]   = this->_flightDetails.rid_details.operation_description;
    _flightDetailsJson["eu_classification"]["category"]          = this->_flightDetails.eu_classification.category;
    _flightDetailsJson["eu_classification"]["class"]             = this->_flightDetails.eu_classification.clasS;
    _flightDetailsJson["uas_id"]["serial_number"]                = this->_flightDetails.uas_id.serial_number;
    _flightDetailsJson["uas_id"]["registration_number"]          = this->_flightDetails.uas_id.registration_number;
    _flightDetailsJson["uas_id"]["utm_id"]                       = this->_flightDetails.uas_id.utm_id;
    _flightDetailsJson["uas_id"]["specific_session_id"]          = this->_flightDetails.uas_id.specific_session_id ;
    _flightDetailsJson["operator_location"]["position"]          = {
                                                           {"lng",this->_flightDetails.operator_location_lng},
                                                           {"lat",this->_flightDetails.operator_location_lat},
                                                           {"accuracy_h",this->_flightDetails.operator_location_accuracyH},
                                                           {"accuracy_v",this->_flightDetails.operator_location_accuracyV}};
    _flightDetailsJson["operator_location"]["altitude"]          = 0; //TODO-> Will get the operator location
    _flightDetailsJson["operator_location"]["altitude_type"]     = "Takeoff";
    _flightDetailsJson["auth_data"]                              = {
        {"format",  this->_flightDetails.auth_data_format},
        {"data", this->_flightDetails.auth_data_data}
    };
    _flightDetailsJson["serial_number"]                          = this->_flightDetails.uas_id.serial_number;;
    _flightDetailsJson["registration_number"]                    = this->_flightDetails.uas_id.registration_number;

    _ridDataJson.clear();
    _ridDataJson["timestamp"]["value"]            = _ridData.timestamp.value;
    _ridDataJson["timestamp"]["format"]           = _ridData.timestamp.format;
    _ridDataJson["timestamp_accuracy"]            = _ridData.timestamp_accuracy;
    _ridDataJson["operational_status"]            = _ridData.operational_status;
    _ridDataJson["position"]["lat"]               = _ridData.position.lat;
    _ridDataJson["position"]["lng"]               = _ridData.position.lng;
    _ridDataJson["position"]["alt"]               = _ridData.position.alt;
    _ridDataJson["position"]["accuracy_h"]        = _ridData.position.accuracy_h;
    _ridDataJson["position"]["accuracy_v"]        = _ridData.position.accuracy_v;
    _ridDataJson["position"]["extrapolated"]      = _ridData.position.extrpolated;
    _ridDataJson["position"]["pressure_altitude"] = _ridData.position.pressure_altitude;
    _ridDataJson["track"]                         = _ridData.track;
    _ridDataJson["speed"]                         = _ridData.speed;
    _ridDataJson["speed_accuracy"]                = _ridData.speed_accuracy;
    _ridDataJson["vertical_speed"]                = _ridData.vertical_speed;
    _ridDataJson["height"]["distance"]            = _ridData.height.distance;
    _ridDataJson["height"]["reference"]           = _ridData.height.reference ;
    _ridDataJson["group_radius"]                  = _ridData.group_radius;
    _ridDataJson["group_ceiling"]                 = _ridData.group_ceiling;
    _ridDataJson["group_floor"]                   = _ridData.group_floor;
    _ridDataJson["group_count"]                   = 1;
    _ridDataJson["group_time_start"]              = _ridData.timestamp.value;
    _ridDataJson["group_time_end"]                = _ridData.timestamp.value;

    json current_states = json::array();
    current_states.push_back(_ridDataJson);
    json mix;
    mix["current_states"] = current_states;
    mix["flight_details"] = _flightDetailsJson;
    json final_array = json::array();
    final_array.push_back(mix);
    json final_format;
    final_format["observations"] = final_array;
    json data = final_format;

    auto [statusCode, response] = requestTelemetry( QString::fromStdString(data.dump(4)));
    _statusCode = statusCode;
    _response = response.toStdString();
    UTMSP_LOG_DEBUG()<< "Status Code: " << _statusCode;

    if(!_response.empty()){

        if(_statusCode == 201)
        {
            try {
                auto now = std::chrono::system_clock::now();
                std::time_t now_c = std::chrono::system_clock::to_time_t(now);
                char buffer[20];
                std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", std::localtime(&now_c));
                UTMSP_LOG_DEBUG() <<"The Telemetry RID data submitted at " << buffer;
                UTMSP_LOG_DEBUG() << "--------------Telemetry Submitted Successfully---------------";
            }
            catch (const json::parse_error& e) {
                UTMSP_LOG_ERROR() << "UTMSPNetworkRemoteManager: Error parsing the Telemetry response: " << e.what();
            }
        }
        else
        {
            UTMSP_LOG_ERROR() << "UTMSPNetworkRemoteManager: Invalid Status Code";
        }
    }
}

bool UTMSPNetworkRemoteIDManager::stopTelemetry()
{
    UTMSP_LOG_INFO() << "--------------Telemetry Stopped Successfully---------------";

    return true;
}
