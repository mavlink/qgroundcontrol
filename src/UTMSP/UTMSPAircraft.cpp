/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "UTMSPAircraft.h"
#include "QGCMAVLink.h"

UTMSPAircraft::UTMSPAircraft()
{

}

std::vector<std::string> UTMSPAircraft::_mavTypes= {
    "Generic micro air vehicle",
    "Fixed wing aircraft",
    "Quadrotor",
    "Coaxial helicopter",
    "Normal helicopter with tail rotor",
    "Ground installation",
    "Operator control unit / ground control station",
    "Airship, controlled",
    "Free balloon, uncontrolled",
    "Rocket",
    "Ground rover",
    "Surface vessel, boat, ship",
    "Submarine",
    "Hexarotor",
    "Octorotor",
    "Tricopter",
    "Flapping wing",
    "Kite"
};

std::string UTMSPAircraft::aircraftSerialNo(const mavlink_message_t &message)
{
    if (message.msgid == MAVLINK_MSG_ID_AUTOPILOT_VERSION) {
        mavlink_autopilot_version_t autopilot_version;
        mavlink_msg_autopilot_version_decode(&message, &autopilot_version);

        _mavSerialNumber = autopilot_version.uid;
    }
    std::string serialNo = std::to_string(_mavSerialNumber);

    return serialNo;
}

std::string UTMSPAircraft::aircraftModel()
{
    //TODO--> Get the Aircraft Model of pixhawk board
    std::string model = "Multi-rotor"; // Dummy model

    return model;
}

std::string UTMSPAircraft::aircraftClass()
{
    //TODO--> Get the category class
    std::string aircraftClass = "Group1"; // Dummy class

    return aircraftClass;
}

std::string UTMSPAircraft::aircraftType(const mavlink_message_t &message)
{
    switch (message.msgid)
    {
    case MAVLINK_MSG_ID_HEARTBEAT:
        mavlink_heartbeat_t heartbeat;
        mavlink_msg_heartbeat_decode(&message, &heartbeat);
        _mavType = static_cast<int>(heartbeat.type);
        break;
    }

    if (_mavType >= 0 && _mavType < static_cast<int>(_mavTypes.size())) {
        return _mavTypes[_mavType];
    }

    return "Unknown Type";
}

