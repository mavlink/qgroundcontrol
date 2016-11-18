/*!
 * @file
 *   @brief Stop gap code to handle M4 initialization
 *   @author Gus Grubba <mavlink@grubba.com>
 *
 */

/*-----------------------------------------------------------------------------
 *   Original source: DroneFly/droneservice/src/main/java/com/yuneec/droneservice/parse/St16Controller.java
 */

#if defined(MINIMALIST_BUILD)

#include "m4.h"
#include "SerialComm.h"
#include <QDebug>

static const char* kUartName = "/dev/ttyMFD0";

//-----------------------------------------------------------------------------
static const unsigned char CRC8T[] = {
    0, 7, 14, 9, 28, 27, 18, 21, 56, 63, 54, 49, 36, 35, 42, 45, 112, 119, 126, 121, 108, 107,
    98, 101, 72, 79, 70, 65, 84, 83, 90, 93, 224, 231, 238, 233, 252, 251, 242, 245, 216, 223, 214, 209, 196,
    195, 202, 205, 144, 151, 158, 153, 140, 139, 130, 133, 168, 175, 166, 161, 180, 179, 186, 189, 199, 192,
    201, 206, 219, 220, 213, 210, 255, 248, 241, 246, 227, 228, 237, 234, 183, 176, 185, 190, 171, 172, 165,
    162, 143, 136, 129, 134, 147, 148, 157, 154, 39, 32, 41, 46, 59, 60, 53, 50, 31, 24, 17, 22, 3, 4, 13, 10,
    87, 80, 89, 94, 75, 76, 69, 66, 111, 104, 97, 102, 115, 116, 125, 122, 137, 142, 135, 128, 149, 146, 155,
    156, 177, 182, 191, 184, 173, 170, 163, 164, 249, 254, 247, 240, 229, 226, 235, 236, 193, 198, 207, 200,
    221, 218, 211, 212, 105, 110, 103, 96, 117, 114, 123, 124, 81, 86, 95, 88, 77, 74, 67, 68, 25, 30, 23, 16,
    5, 2, 11, 12, 33, 38, 47, 40, 61, 58, 51, 52, 78, 73, 64, 71, 82, 85, 92, 91, 118, 113, 120, 127, 106, 109,
    100, 99, 62, 57, 48, 55, 34, 37, 44, 43, 6, 1, 8, 15, 26, 29, 20, 19, 174, 169, 160, 167, 178, 181, 188,
    187, 150, 145, 152, 159, 138, 141, 132, 131, 222, 217, 208, 215, 194, 197, 204, 203, 230, 225, 232, 239,
    250, 253, 244, 243
};

//-----------------------------------------------------------------------------
/** x^8 + x^2 + x + 1 */
uint8_t
M4Controller::crc8(uint8_t* buffer, int len)
{
    uint8_t ret = 0;
    for(int i = 0; i < len; ++i) {
        ret = CRC8T[ret ^ buffer[i]];
    }
    return ret;
}

//-----------------------------------------------------------------------------
M4Controller::M4Controller(QObject* parent)
    : QObject(parent)
{
    _commPort = new M4SerialComm(this);
}

//-----------------------------------------------------------------------------
M4Controller::~M4Controller()
{
    if(_commPort) {
        delete _commPort;
    }
}

//-----------------------------------------------------------------------------
bool
M4Controller::init()
{
    qDebug() << "Init M4 Handler";
    if(!_commPort || !_commPort->init(kUartName, 230400) || !_commPort->open()) {
        qWarning() << "Could not start serial communication with M4";
        return false;
    }
    connect(_commPort, &M4SerialComm::bytesReady, this, &M4Controller::_bytesReady);
    return _start();
}

//-----------------------------------------------------------------------------
bool
M4Controller::_start()
{
    qDebug() << "Enable M4";
    //-- Enable M4
    unsigned char buf[128];
    memset(buf, 0, sizeof(buf));
    buf[0]  = 0x55; //-- Sync bytes
    buf[1]  = 0x55;
    int payload_len = 10;
    buf[2]  = payload_len + 1;
    buf[5]  = 0x03 << 2;
    buf[12] = 0x68; //-- Enter run command
    buf[payload_len + 3] = crc8(buf + 3, payload_len);
    return _commPort->write(buf, payload_len + 4);
}

//-----------------------------------------------------------------------------
void
M4Controller::_bytesReady(QByteArray data)
{
    m4Packet packet(data);
    int type = packet.type();
    qDebug() << "M4 Packet:" << type << ((type & 0x1c) >> 2);
    //-- Some Chinese voodoo
    type = (type & 0x1c) >> 2;
    if (_handleNonTypePacket(packet)) {
        return;
    }
    switch (type) {
        case Yuneec::TYPE_BIND:
            qDebug() << "M4 Packet: TYPE_BIND" << (uint8_t)data[3];
            switch ((uint8_t)data[3]) {
                case 2:
                    _handleRxBindInfo(packet);
                    break;
                case 4:
                    break;
                case 12:
                    break;
                default:
                    break;
            }
            break;
        case Yuneec::TYPE_CHN:
            _handleChannel(packet);
            break;
        case Yuneec::TYPE_CMD:
            _handleCommand(packet);
            break;
        case Yuneec::TYPE_RSP:
            qDebug() << "M4 Packet: TYPE_RSP" << packet.commandID();
            switch (packet.commandID()) {
                case Yuneec::CMD_QUERY_BIND_STATE:
                    BindState state;
                    state.state = (data[10] & 0xff) | (data[11] << 8 & 0xff00);
                    qDebug() << "M4 Packet: CMD_QUERY_BIND_STATE" << state.state;
                    /*
                    ControllerStateManager manager = ControllerStateManager.getInstance();
                    if (manager != null) {
                        manager.onRecvBindState(state);
                    }
                    */
                    break;
                default:
                    break;
            }
            break;
        case Yuneec::TYPE_MISSION:
            qDebug() << "M4 Packet: TYPE_MISSION";
            break;
        default:
            qDebug() << "M4 Packet: Unknown Packet" << type;
            break;
    }
}

//-----------------------------------------------------------------------------
bool
M4Controller::_handleNonTypePacket(m4Packet& packet) {
    int commandId = packet.commandID();
    switch (commandId) {
        case Yuneec::COMMAND_M4_SEND_GPS_DATA_TO_PA:
            qDebug() << "M4 Packet: COMMAND_M4_SEND_GPS_DATA_TO_PA";
            /*
            if (droneFeedbackListener != null) {
                handControllerFeedback(commandData);
            }
            */
            return true;
    }
    return false;
}

//-----------------------------------------------------------------------------
void
M4Controller::_handleRxBindInfo(m4Packet& packet) {
    RxBindInfo rxBindInfoFeedback;
    rxBindInfoFeedback.mode     = ((uint8_t)packet.data[6]  & 0xff) | ((uint8_t)packet.data[7]  << 8 & 0xff00);
    rxBindInfoFeedback.panId    = ((uint8_t)packet.data[8]  & 0xff) | ((uint8_t)packet.data[9]  << 8 & 0xff00);
    rxBindInfoFeedback.nodeId   = ((uint8_t)packet.data[10] & 0xff) | ((uint8_t)packet.data[11] << 8 & 0xff00);
    rxBindInfoFeedback.aNum     = (uint8_t)packet.data[20];
    rxBindInfoFeedback.aBit     = (uint8_t)packet.data[21];
    rxBindInfoFeedback.swNum    = (uint8_t)packet.data[24];
    rxBindInfoFeedback.swBit    = (uint8_t)packet.data[25];
    int p = packet.data.length() - 2;
    rxBindInfoFeedback.txAddr = ((uint8_t)packet.data[p] & 0xff) | ((uint8_t)packet.data[p + 1] << 8 & 0xff00);
    qDebug() << "M4 Packet: RxBindInfo:" << rxBindInfoFeedback.getName();
    /*
    ControllerStateManager manager = ControllerStateManager.getInstance();
    if (manager != null) {
        manager.onRecvBindInfo(rxBindInfoFeedback);
    }
    */
}

//-----------------------------------------------------------------------------
void
M4Controller::_handleChannel(m4Packet& packet) {
    Q_UNUSED(packet);
    switch (packet.commandID()) {
        case Yuneec::CMD_RX_FEEDBACK_DATA:
            qDebug() << "M4 Packet: CMD_RX_FEEDBACK_DATA";
            /*
            if (droneFeedbackListener == null) {
                return;
            }
            handleDroneFeedback(packet);
            */
            break;
        case Yuneec::CMD_TX_CHANNEL_DATA_MIXED:
            qDebug() << "M4 Packet: CMD_TX_CHANNEL_DATA_MIXED";
            /*
            if (controllerFeedbackListener == null) {
                return;
            }
            handleMixedChannelData(packet);
            */
            break;
    }
}

//-----------------------------------------------------------------------------
bool
M4Controller::_handleCommand(m4Packet& packet) {
    Q_UNUSED(packet);
    switch (packet.commandID()) {
        case Yuneec::CMD_TX_STATE_MACHINE:
            qDebug() << "M4 Packet: CMD_TX_STATE_MACHINE";
            /*
            if (controllerFeedbackListener != null) {
                QByteArray commandValues = packet.commandValues();
                int status = commandValues[0] & 0x1f;
                ControllerStateManager manager = ControllerStateManager.getInstance();
                if (manager != null) {
                    manager.onRecvTxState(status);
                }
                controllerFeedbackListener.onHardwareStatesChange(status);
                return true;
            }
            */
            break;
        case Yuneec::CMD_TX_SWITCH_CHANGED:
            qDebug() << "M4 Packet: CMD_TX_SWITCH_CHANGED";
            /*
            if (controllerFeedbackListener != null) {
                switchChanged(packet);
                return true;
            }
            */
            break;
    }
    return false;
}

//-----------------------------------------------------------------------------
void
M4Controller::_switchChanged(m4Packet& packet) {
    Q_UNUSED(packet);
    QByteArray commandValues = packet.commandValues();
    SwitchChanged switchChanged;
    switchChanged.hwId      = commandValues[0];
    switchChanged.oldState  = commandValues[1];
    switchChanged.newState  = commandValues[2];
    //controllerFeedbackListener.onSwitchChanged(switchChanged);
    qDebug() << "M4 Packet: SwitchChanged" << switchChanged.hwId << switchChanged.oldState << switchChanged.newState;
}

#endif
