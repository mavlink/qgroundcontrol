/*!
 * @file
 *   @brief ST16 Controller
 *   @author Gus Grubba <mavlink@grubba.com>
 *
 */

#pragma once

#include <QByteArray>
#include <QString>

#include "m4def.h"

extern uint8_t crc8(uint8_t* buffer, int len);

//-- Comments (Google) translated from Chinese.
//-- Nowhere I could find access through a struct. It's always numbered index into a byte array.
//-- Also note that throughout the code, the header is given as 10 bytes long and the payload
//   starts at index 10. The command starts at index 3 (FCF below). CRC is of payload only and
//   follows it (single uint8_t byte).
struct m4CommandHeader {
    uint16_t   start_str;           // 0x5555
    uint8_t    size;                // Len bytes, the data length, with len M3 generated
    uint16_t   FCF;                 // The frame control domain is shown in Table 2
    uint8_t    Type;                // Frame type, binding is set to a binding frame in Table 4
    uint16_t   PANID;               // Address? (Google says it's "Website address")
    uint16_t   NodeIDdest;          // The node address
    uint16_t   NodeIDsource;        // The node address
    uint8_t    command;
};

class M4SerialComm;

//-----------------------------------------------------------------------------
//-- Accessor class to handle data structure in an "Yuneec" way
class m4Packet
{
public:
    m4Packet(QByteArray data_)
        : data(data_)
    {
    }
    int type()
    {
        return data[2] & 0xFF;
    }
    int commandID()
    {
        return data[9] & 0xFF;
    }
    int commandIdFromMission()
    {
        if(data.size() > 10)
            return data[10] & 0xff;
        else
            return 0;
    }
    int subCommandIDFromMission()
    {
        if(data.size() > 11)
            return data[11] & 0xff;
        else
            return 0;
    }
    QByteArray commandValues()
    {
        return data.mid(10); //-- Data - header = payload
    }
    int mixCommandId(int type, int commandId, int subCommandId)
    {
        if(type != Yuneec::COMMAND_TYPE_MISSION) {
            return commandId;
        } else {
            return type << 16 | commandId << 8 | subCommandId;
        }
    }
    int mixCommandId()
    {
        if(type() != Yuneec::COMMAND_TYPE_MISSION) {
            return commandID();
        } else {
            return mixCommandId(type(), commandIdFromMission(), subCommandIDFromMission());
        }
    }
    QByteArray data;
};

//-----------------------------------------------------------------------------
class RxBindInfo {
public:
    RxBindInfo();
    void clear();
    enum {
        TYPE_NULL   = -1,
        TYPE_SR12S  = 0,
        TYPE_SR12E  = 1,
        TYPE_SR24S  = 2,
        TYPE_RX24   = 3,
        TYPE_SR19P  = 4,
    };
    QString getName();
    int mode;
    int panId;
    int nodeId;
    int aNum;
    int aBit;
    int swNum;
    int swBit;
    int txAddr;
};

//-----------------------------------------------------------------------------
class ControllerLocation {
public:
    ControllerLocation()
        : longitude(0.0)
        , latitude(0.0)
        , altitude(0.0)
        , satelliteCount(0)
        , accuracy(0.0f)
        , speed(0.0f)
        , angle(0.0f)
    {
    }
    ControllerLocation& operator=(ControllerLocation& other)
    {
        longitude       = other.longitude;
        latitude        = other.latitude;
        altitude        = other.altitude;
        satelliteCount  = other.satelliteCount;
        accuracy        = other.accuracy;
        speed           = other.speed;
        angle           = other.angle;
        return *this;
    }
    /**
     * Longitude of remote-controller
     */
    double longitude;
    /**
     * Latitude of remote-controller
     */
    double latitude;
    /**
     * Altitude of remote-controller
     */
    double altitude;
    /**
     * The number of satellite has searched
     */
    int satelliteCount;

    /**
     * Accuracy of remote-controller
     */
    float accuracy;

    /**
     * Speed of remote-controller
     */
    float speed;

    /**
     * Angle of remote-controller
     */
    float angle;
};

//-----------------------------------------------------------------------------
class SwitchChanged {
public:
    /**
     * Hardware ID
     */
    int hwId;
    /**
     * Old status of hardware
     */
    int oldState;
    /**
     * New status of hardware
     */
    int newState;
};

#define m4CommandHeaderLen sizeof(m4CommandHeader)

//-----------------------------------------------------------------------------
// Base Yuneec Protocol Command
class m4Command
{
public:
    m4Command(int id, int type = Yuneec::COMMAND_TYPE_NORMAL)
    {
        data.fill(0, Yuneec::COMMAND_BODY_EXCLUDE_VALUES_LENGTH);
        data[2] = (uint8_t)type;
        data[9] = (uint8_t)id;
    }
    virtual ~m4Command() {}
    QByteArray pack(QByteArray payload = QByteArray())
    {
        if(payload.size()) {
            data.append(payload);
        }
        QByteArray command;
        command.resize(3);
        command[0] = 0x55;
        command[1] = 0x55;
        command[2] = (uint8_t)data.size() + 1;
        command.append(data);
        uint8_t crc = crc8((uint8_t*)data.data(), data.size());
        command.append(crc);
        return command;
    }
    QByteArray data;
};

//-----------------------------------------------------------------------------
// Base Yuneec Protocol Message
class m4Message
{
public:
    m4Message(int id, int type = 0)
    {
        data.fill(0, 8);
        data[2] = (uint8_t)type;
        data[3] = (uint8_t)id;
    }
    virtual ~m4Message() {}
    QByteArray pack()
    {
        QByteArray command;
        command.resize(3);
        command[0] = 0x55;
        command[1] = 0x55;
        command[2] = (uint8_t)data.size() + 1;
        command.append(data);
        uint8_t crc = crc8((uint8_t*)data.data(), data.size());
        command.append(crc);
        return command;
    }
    QByteArray data;
};
