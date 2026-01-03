#pragma once

#include "MAVLinkLib.h"
#include <string>
#include <vector>

class UTMSPAircraft {
public:
    UTMSPAircraft();
    std::string aircraftSerialNo(const mavlink_message_t &message);
    std::string aircraftModel();
    std::string aircraftType(const mavlink_message_t& message);
    std::string aircraftClass();

private:
    int             _mavType;
    uint64_t        _mavSerialNumber;

    static std::vector<std::string> _mavTypes;
};
