/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

// #include <nlohmann/json.hpp>
#include "services/dispatcher.h"

#include "UTMSPBlenderRestInterface.h"
#include "parse/nlohmann/json.hpp"

class Dispatcher;

using json = nlohmann::ordered_json;

class UTMSPNetworkRemoteIDManager:public UTMSPBlenderRestInterface
{
public:
    UTMSPNetworkRemoteIDManager(std::shared_ptr<Dispatcher> dispatcher);
    ~UTMSPNetworkRemoteIDManager();

    void getCapabilty(const std::string& token);
    void startTelemetry(const double& latitude,
                        const double& longitude,
                        const double& altitude,
                        const double& heading,
                        const double& velocityX,
                        const double& velocityY,
                        const double& velocityZ,
                        const double& relativeAltitude,
                        const std::string& aircraftSerialNumber,
                        const std::string& operatorID,
                        const std::string& flightID);
    bool stopTelemetry();

    struct Rid_details{
        std::string   id;
        std::string   operator_id;
        std::string   operation_description;
    };

    struct Eu_classification{
        std::string category;
        std::string clasS;
    };

    struct Uas_id{
        std::string   serial_number;
        std::string   registration_number;
        std::string   utm_id;
        std::string   specific_session_id;
    };

    struct FlightDetails
    {
        Rid_details       rid_details;
        Eu_classification eu_classification;
        Uas_id            uas_id;
        double            operator_location_lng;
        double            operator_location_lat;
        std::string       operator_location_accuracyH;
        std::string       operator_location_accuracyV;
        std::string       auth_data_format;
        int               auth_data_data;
        std::string       serial_number;
        std::string       registration_number;
    };

    struct Position {
        double lat;                   ///< The latitude of the position [°].
        double lng;                   ///< The longitude of the position in [°].
        double alt;                   ///< The altitude above mean sea level of the position in [m].
        std::string accuracy_h;            ///< The horizontal accuracy of the position in [m].
        std::string accuracy_v;
        bool extrpolated;
        double pressure_altitude;
    };

    struct Height{
        double distance;
        std::string reference;
    };

    struct Timestamp{
        std::string value;
        std::string format;
    };

    struct RidData {
        Timestamp timestamp;
        double timestamp_accuracy;
        std::string operational_status;
        Position position;
        double track;
        double speed;
        std::string speed_accuracy;
        double vertical_speed;
        Height height;
        double group_radius;
        double group_ceiling;
        double group_floor;
        int group_count;
        std::string group_time_start;
        std::string group_time_end;
    };

private:
    FlightDetails                _flightDetails;
    json                         _flightDetailsJson;
    RidData                      _ridData;
    json                         _ridDataJson;
    int                          _statusCode;
    std::string                  _response;
    std::shared_ptr<Dispatcher>  _dispatcher;
    double                       _initLatitude;
    double                       _initLongitude;
    int                          _j=0;
};
