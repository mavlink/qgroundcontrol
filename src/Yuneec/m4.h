/*!
 * @file
 *   @brief Stop gap code to handle M4 initialization
 *   @author Gus Grubba <mavlink@grubba.com>
 *
 */

#ifndef YUNEEC_M4_H
#define YUNEEC_M4_H
#if defined(MINIMALIST_BUILD)

#include <QObject>
#include "m4Defines.h"

class M4SerialComm;

//-- Comments (Google) translated from Chinese.
//-- Nowhere I could find access through a struct. It's always numbered index into a byte array.
//-- Also note that throughout the code, the header is given as 10 bytes long and the payload starts at index 10.
struct m4CommandHeader {
    uint16_t   start_str;           // 0x5555
    uint8_t    size;                // Len bytes, the data length, with len M3 generated
    uint16_t   FCF;                 // The frame control domain is shown in Table 2
    uint8_t    Type;                // Frame type, binding is set to a binding frame in Table 4
    uint16_t   PANID;               // Website address (Address?)
    uint16_t   NodeIDdest;          // The node address
    uint16_t   NodeIDsource;        // The node address
    uint8_t    command;
};

class RxBindInfo {
public:
    int mode;
    int panId;
    int nodeId;
    int aNum;
    int aBit;
    int swNum;
    int swBit;
    int txAddr;
    enum {
        TYPE_NULL   = -1,
        TYPE_SR12S  = 0,
        TYPE_SR12E  = 1,
        TYPE_SR24S  = 2,
        TYPE_RX24   = 3,
        TYPE_SR19P  = 4,
    };
    QString getName() {
        switch (mode) {
            case TYPE_SR12S:
                return QString("SR12S_%1").arg(nodeId);
            case TYPE_SR12E:
                return QString("SR12E_%1").arg(nodeId);
            case TYPE_SR24S:
                return QString("SR24S_%1 v1.03").arg(nodeId);
            case TYPE_RX24:
                return QString("RX24_%1").arg(nodeId);
            case TYPE_SR19P:
                return QString("SR19P_%1").arg(nodeId);
            default:
                if (mode >= 105) {
                    QString fmt;
                    fmt.sprintf("SR24S_%dv%.2f", nodeId, (float)mode / 100.0f);
                    return fmt;
                } else {
                    return QString::number(nodeId);
                }
        }
    }
};

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

class BindState {
public:
    enum {
        NOT_BOUND = 0,
        BOUND = 1
    };
    int state;
};

#define m4CommandHeaderLen sizeof(m4CommandHeader)

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
        return data[10] & 0xff;
    }
    int subCommandIDFromMission()
    {
        return data[11] & 0xff;
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
class M4Controller : public QObject
{
    Q_OBJECT
public:
    M4Controller(QObject* parent = NULL);
    ~M4Controller();
    bool    init();
    static  uint8_t crc8(uint8_t* buffer, int len);
private slots:
    void    _bytesReady (QByteArray data);
private:
    bool    _start();
    bool    _handleNonTypePacket(m4Packet& packet);
    void    _handleRxBindInfo   (m4Packet& packet);
    void    _handleChannel      (m4Packet& packet);
    bool    _handleCommand      (m4Packet& packet);
    void    _switchChanged      (m4Packet& packet);
private:
    M4SerialComm* _commPort;
};

#endif
#endif
