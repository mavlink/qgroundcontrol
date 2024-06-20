/****************************************************************************
 *
 * (c) 2023 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <string>
#include <nlohmann/json.hpp>

#include "UTMSPBlenderRestInterface.h"

using json = nlohmann::ordered_json;

class UTMSPFlightPlanManager: public UTMSPBlenderRestInterface
{
public:
    UTMSPFlightPlanManager();
    ~UTMSPFlightPlanManager();

    enum class FlightState {
        Idle,
        Register,
        Activated,
        StartTelemetryStreaming,
        StopTelemetryStreaming
    };


    void getCapability();
    void preRegisterFlightPlan();
    void preRegisterFlightPlanNotification();
    void registerFlightPlan(const std::string& token,
                            const json& boundaryPolygons,
                            const int& minAltitude,
                            const int& maxAltitude,
                            const std::string& startDateTime,
                            const std::string& endDateTime);
    std::tuple<std::string, std::string, bool> registerFlightPlanNotification();
    bool activateFlightPlan(const std::string &token);
    void activateFlightPlanNotification();
    void  updateFlightPlanState(FlightState state);
    FlightState getFlightPlanState();

    // Structs for RegisterFlightPlan JSON
    struct Min_Altitude{
        int meters;
        std::string datum;
    };

    struct Max_Altitude{
        int meters;
        std::string datum;
    };

    struct Properties{
        Min_Altitude min_Altitude;
        Max_Altitude max_Altitude;
    };

    struct Geometry{
        std::string type;
        json coordinates;
    };


    struct Features{
        std::string type;
        Properties properties;
        Geometry geometry;

    };

    struct FlightDeclaration{
        std::string type;
        Features features;
    };

    struct FlightData{
        std::string user;
        int operation;
        std::string aircraftID;
        std::string gcsID;
        std::string party;
        std::string startDateTime;
        std::string endDateTime;
        FlightDeclaration flightDeclaration;
    };

private:
    FlightData                 _flightData;
    json                       _flightDataJson;
    FlightState                _currentState;
    std::string                _responseJSON;
    bool                       _responseStatus;
    std::string                _flightResponseID;
    json                       _updateState;
};
