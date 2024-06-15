/****************************************************************************
 *
 * (c) 2023 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "UTMSPFlightPlanManager.h"
#include "UTMSPLogger.h"

UTMSPFlightPlanManager::UTMSPFlightPlanManager():
    _currentState(FlightState::Idle),
    _responseJSON(""),
    _responseStatus(false),
    _flightResponseID("")
{

}

UTMSPFlightPlanManager::~UTMSPFlightPlanManager()
{

}

void UTMSPFlightPlanManager::getCapability()
{
    //TODO
}

void UTMSPFlightPlanManager::preRegisterFlightPlan()
{
    //TODO preRegisterFlightPlan is bit unclear now
}

void UTMSPFlightPlanManager::preRegisterFlightPlanNotification()
{
    //TODO
}

void UTMSPFlightPlanManager::registerFlightPlan(const std::string &token,
                                                const json &boundaryPolygons,
                                                const int &minAltitude,
                                                const int &maxAltitude,
                                                const std::string &startDateTime,
                                                const std::string &endDateTime)
{
    // Generate Flight declaretion JSON

    this->_flightData.user      =  "user@example.com"; //TODO
    this->_flightData.operation = 1;
    this->_flightData.party     = "Flight 1023"; //TODO
    this->_flightData.startDateTime = startDateTime;
    this->_flightData.endDateTime   = endDateTime;
    this->_flightData.flightDeclaration.type  = "FeatureCollection";
    this->_flightData.flightDeclaration.features.type = "Feature";
    this->_flightData.flightDeclaration.features.properties.min_Altitude.meters = minAltitude;
    this->_flightData.flightDeclaration.features.properties.min_Altitude.datum = "agl";
    this->_flightData.flightDeclaration.features.properties.max_Altitude.meters = maxAltitude;
    this->_flightData.flightDeclaration.features.properties.max_Altitude.datum = "agl";
    this->_flightData.flightDeclaration.features.geometry.type = "Polygon";
    this->_flightData.flightDeclaration.features.geometry.coordinates = boundaryPolygons;

    _flightDataJson.clear();

    // Generate GeoJson for flight plan

    _flightDataJson["submitted_by"] = this->_flightData.user;
    _flightDataJson["type_of_operation"]= this->_flightData.operation;
    _flightDataJson["originating_party"]= this->_flightData.party;
    _flightDataJson["start_datetime"]= this->_flightData.startDateTime;
    _flightDataJson["end_datetime"] = this->_flightData.endDateTime;
    _flightDataJson["flight_declaration_geo_json"]["type"] = this->_flightData.flightDeclaration.type;
    _flightDataJson["flight_declaration_geo_json"]["features"] ={{
                                                                   {"type", this->_flightData.flightDeclaration.features.type},
                                                                   {"properties",{

                                                                                   {"min_altitude",{
                                                                                                        {"meters",this->_flightData.flightDeclaration.features.properties.min_Altitude.meters},
                                                                                                        {"datum",this->_flightData.flightDeclaration.features.properties.min_Altitude.datum}
                                                                                                    }},
                                                                                   {"max_altitude",{
                                                                                                        {"meters",this->_flightData.flightDeclaration.features.properties.max_Altitude.meters},
                                                                                                        {"datum",this->_flightData.flightDeclaration.features.properties.max_Altitude.datum}
                                                                                                    }}}},

                                                                   {"geometry",  {

                                                                                    {"type", this->_flightData.flightDeclaration.features.geometry.type},

                                                                                    {"coordinates",{this->_flightData.flightDeclaration.features.geometry.coordinates}}

                                                                                }}}};


    json data = _flightDataJson;
    auto dataString = QString::fromStdString(data.dump(4));
    setHost("BlenderClient");
    setBearerToken(token.c_str());
    auto [statusCode, response] =  setFlightPlan(dataString);
    UTMSP_LOG_INFO() << "UTMSPFlightPlanManager: Register Response -->" << response;

    if(statusCode == 201)
    {
        try {
            json jsonData = json::parse(response.toStdString());
            std::string flightID = jsonData["id"];

            _flightResponseID = flightID;
            _responseJSON = response.toStdString();
            _responseStatus = true;
        }
        catch (const json::parse_error& e) {
            UTMSP_LOG_ERROR() << "UTMSPFlightPlanManager: Error parsing the response: " << e.what();
            _responseJSON = "Response is Invalid";
            _responseStatus = false;
        }
    }
    else
    {
        UTMSP_LOG_ERROR() << "UTMSPFlightPlanManager: Invalid Status Code";
        _responseJSON = "Response is Invalid";
        _responseStatus = false;
    }
}

std::tuple<std::string, std::string, bool> UTMSPFlightPlanManager::registerFlightPlanNotification()
{
    return std::make_tuple(_responseJSON, _flightResponseID, _responseStatus);
}

void UTMSPFlightPlanManager::activateFlightPlan()
{
    //TODO : Plan for conformance Monitering phase 2
}

void UTMSPFlightPlanManager::activateFlightPlanNotification()
{
    //TODO : Plan for conformance Monitering phase 2
}

UTMSPFlightPlanManager::FlightState UTMSPFlightPlanManager::getFlightPlanState()
{
    UTMSP_LOG_INFO() << "UTMSPFlightPlanManager: Fetch state -->" << static_cast<int>(_currentState);

    return _currentState;
}

void UTMSPFlightPlanManager::updateFlightPlanState(FlightState state)
{
    UTMSP_LOG_INFO() << "UTMSPFlightPlanManager: State updated to -->" << static_cast<int>(state);
    _currentState = state;
}
