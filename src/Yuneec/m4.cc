/*!
 * @file
 *   @brief ST16 Controller
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
    //-- Enable M4: This is temporary. We will eventualy need to go through the
    //   whole initialization now done by the default Yuneec app.
    return _enterRun();
}

//-----------------------------------------------------------------------------
/**
 * This command is used for entering the progress of binding aircraft.
 * This command is the first step of the progress of binging aircraft.
 * The next command you will send may be {@link _startBind}.
 */
bool
M4Controller::_enterRun()
{
    m4Command enterRunCmd(Yuneec::CMD_ENTER_RUN);
    QByteArray cmd = enterRunCmd.pack();
    return _commPort->write(cmd);
}

//-----------------------------------------------------------------------------
/**
 * This command is used for stopping control aircraft.
 */
bool
M4Controller::_exitRun()
{
    m4Command exitRunCmd(Yuneec::CMD_EXIT_RUN);
    QByteArray cmd = exitRunCmd.pack();
    return _commPort->write(cmd);
}

//-----------------------------------------------------------------------------
/**
 * This command is used for entering the progress of binding aircraft.
 * This command is the first step of the progress of binging aircraft.
 * The next command you will send may be {@link _startBind}.
 */
bool
M4Controller::_enterBind()
{
    m4Command enterBindCmd(Yuneec::CMD_ENTER_BIND);
    QByteArray cmd = enterBindCmd.pack();
    return _commPort->write(cmd);
}

//-----------------------------------------------------------------------------
/**
 * This command is used for exiting the progress of binding.
 */
bool
M4Controller::_exitBind()
{
    m4Command exitBindCmd(Yuneec::CMD_EXIT_BIND);
    QByteArray cmd = exitBindCmd.pack();
    return _commPort->write(cmd);
}

//-----------------------------------------------------------------------------
/**
 * After {@link _enterBind} response rightly, send this command to get a list of aircraft which can be bind.
 * The next command you will send may be {@link _bind}.
 */
bool
M4Controller::_startBind()
{
    m4Command startBindCmd(Yuneec::CMD_START_BIND);
    QByteArray bindPayload;
    bindPayload.resize(8);
    bindPayload[3] = Yuneec::CMD_START_BIND;
    bindPayload[6] = 0x05;  // 跳频间隔时间1 (Frequency hopping interval 1)
    bindPayload[7] = 0x0F;  // 跳频间隔时间2 (Frequency hopping interval 2)
    startBindCmd.data.append(bindPayload);
    QByteArray cmd = startBindCmd.pack();
    return _commPort->write(cmd);
}

//-----------------------------------------------------------------------------
/**
 * Use this command to bind specified aircraft.
 * After {@link _startBind} response rightly, you get a list of aircraft which can be bound.
 * Then you send this command and {@link _queryBindState} repeatedly several times until get a right
 * response from {@link _queryBindState}. If not bind successful after sending commands several times,
 * the progress of bind exits.
 */
bool
M4Controller::_bind(int rxAddr)
{
    m4Command bindCmd(Yuneec::CMD_BIND);
    QByteArray bindPayload;
    bindPayload.resize(8);
    bindPayload[3] = Yuneec::CMD_START_BIND;
    bindPayload[4] = (uint8_t)(rxAddr & 0xff);
    bindPayload[5] = (uint8_t)((rxAddr & 0xff00) >> 8);
    bindPayload[6] = 5; //-- Gotta love magic numbers
    bindPayload[7] = 15;
    bindCmd.data.append(bindPayload);
    QByteArray cmd = bindCmd.pack();
    return _commPort->write(cmd);
}

//-----------------------------------------------------------------------------
/**
 * This command is used for disconnecting the bound aircraft.
 * Suggest to use this command before using {@link _bind} first time.
 */
bool
M4Controller::_unbind()
{
    m4Command unbindCmd(Yuneec::CMD_UNBIND);
    QByteArray cmd = unbindCmd.pack();
    return _commPort->write(cmd);
}

//-----------------------------------------------------------------------------
/**
 * This command is used for querying the state of whether bind was succeed.
 * This command always be sent follow {@link _bind} with a transient time.
 * The next command you will send may be {@link _setChannelSetting}.
 */
bool
M4Controller::_queryBindState()
{
    m4Command queryBindStateCmd(Yuneec::CMD_QUERY_BIND_STATE);
    QByteArray cmd = queryBindStateCmd.pack();
    return _commPort->write(cmd);
}

//-----------------------------------------------------------------------------
void
M4Controller::_bytesReady(QByteArray data)
{
    m4Packet packet(data);
    int type = packet.type();
    //-- Some Chinese voodoo
    type = (type & 0x1c) >> 2;
    if(_handleNonTypePacket(packet)) {
        return;
    }
    switch(type) {
        case Yuneec::TYPE_BIND:
            qDebug() << "M4 Packet: TYPE_BIND" << (uint8_t)data[3];
            switch((uint8_t)data[3]) {
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
            switch(packet.commandID()) {
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
M4Controller::_handleNonTypePacket(m4Packet& packet)
{
    int commandId = packet.commandID();
    switch(commandId) {
        case Yuneec::COMMAND_M4_SEND_GPS_DATA_TO_PA:
            _handControllerFeedback(packet);
            return true;
    }
    return false;
}

//-----------------------------------------------------------------------------
void
M4Controller::_handleRxBindInfo(m4Packet& packet)
{
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
M4Controller::_handleChannel(m4Packet& packet)
{
    Q_UNUSED(packet);
    switch(packet.commandID()) {
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
            _handleMixedChannelData(packet);
            break;
    }
}

//-----------------------------------------------------------------------------
bool
M4Controller::_handleCommand(m4Packet& packet)
{
    Q_UNUSED(packet);
    switch(packet.commandID()) {
        case Yuneec::CMD_TX_STATE_MACHINE:
            {
                QByteArray commandValues = packet.commandValues();
                int status = commandValues[0] & 0x1f;
                /*
                ControllerStateManager manager = ControllerStateManager.getInstance();
                if (manager != null) {
                    manager.onRecvTxState(status);
                }
                controllerFeedbackListener.onHardwareStatesChange(status);
                */
                qDebug() << "M4 Packet: CMD_TX_STATE_MACHINE:" << status;
            }
            break;
        case Yuneec::CMD_TX_SWITCH_CHANGED:
            _switchChanged(packet);
            return true;
    }
    return false;
}

//-----------------------------------------------------------------------------
void
M4Controller::_switchChanged(m4Packet& packet)
{
    Q_UNUSED(packet);
    QByteArray commandValues = packet.commandValues();
    SwitchChanged switchChanged;
    switchChanged.hwId      = commandValues[0];
    switchChanged.oldState  = commandValues[1];
    switchChanged.newState  = commandValues[2];
    //controllerFeedbackListener.onSwitchChanged(switchChanged);
    qDebug() << "M4 Packet: CMD_TX_SWITCH_CHANGED" << switchChanged.hwId << switchChanged.oldState << switchChanged.newState;
}

//-----------------------------------------------------------------------------
void
M4Controller::_handleMixedChannelData(m4Packet& packet)
{
    int analogChannelCount = 10;
    int switchChannelCount = 2;
    QByteArray values = packet.commandValues();
    int value, val1, val2, startIndex;
    QByteArray channels;
    for(int i = 0; i < analogChannelCount + switchChannelCount; i++) {
        if(i < analogChannelCount) {
            startIndex = (int)floor(i * 1.5);
            val1 = values[startIndex] & 0xff;
            val2 = values[startIndex + 1] & 0xff;
            if(i % 2 == 0) {
                value = val1 << 4 | val2 >> 4;
            } else {
                value = (val1 & 0x0f) << 8 | val2;
            }
        } else {
            val1 = values[(int)(ceil((analogChannelCount - 1) * 1.5) + ceil((i - analogChannelCount + 1) * 0.25f))] & 0xff;
            switch((i - analogChannelCount + 1) % 4) {
                case 1:
                    value = val1 >> 6 & 0x03;
                    break;
                case 2:
                    value = val1 >> 4 & 0x03;
                    break;
                case 3:
                    value = val1 >> 2 & 0x03;
                    break;
                case 0:
                    value = val1 >> 0 & 0x03;
                    break;
                default:
                    value = 0;
                    break;
            }
        }
        channels.append(value);
    }
    qDebug() << "M4 Packet: CMD_TX_CHANNEL_DATA_MIXED:" << channels;
    //controllerFeedbackListener.onReceiveChannelValues(channels);
}

//-----------------------------------------------------------------------------
void
M4Controller::_handControllerFeedback(m4Packet& packet) {
    QByteArray commandValues = packet.commandValues();
    ControllerLocation controllerLocation;
    controllerLocation.latitude     = byteArrayToInt(commandValues, 0) / 1e7;
    controllerLocation.longitude    = byteArrayToInt(commandValues, 4) / 1e7;
    controllerLocation.altitude     = byteArrayToFloat(commandValues, 8);
    controllerLocation.accuracy     = byteArrayToShort(commandValues, 12);
    controllerLocation.speed        = byteArrayToShort(commandValues, 14);
    controllerLocation.angle        = byteArrayToShort(commandValues, 16);
    controllerLocation.satelliteCount = commandValues[18] & 0x1f;
    //controllerFeedbackListener.onReceiveControllerLocation(controllerLocation);
    qDebug() << "M4 Packet: COMMAND_M4_SEND_GPS_DATA_TO_PA" << controllerLocation.satelliteCount << controllerLocation.latitude << controllerLocation.longitude << controllerLocation.altitude;
}

//-----------------------------------------------------------------------------
int
M4Controller::byteArrayToInt(QByteArray data, int offset, bool isBigEndian)
{
    int iRetVal = -1;
    if (data.size() < offset + 4)
        return iRetVal;
    int iLowest;
    int iLow;
    int iMid;
    int iHigh;
    if (isBigEndian) {
        iLowest = data[offset + 3];
        iLow    = data[offset + 2];
        iMid    = data[offset + 1];
        iHigh   = data[offset + 0];
    } else {
        iLowest = data[offset + 0];
        iLow    = data[offset + 1];
        iMid    = data[offset + 2];
        iHigh   = data[offset + 3];
    }
    iRetVal = (iHigh << 24) | ((iMid & 0xFF) << 16) | ((iLow & 0xFF) << 8) | (0xFF & iLowest);
    return iRetVal;
}

//-----------------------------------------------------------------------------
float
M4Controller::byteArrayToFloat(QByteArray data, int offset)
{
    uint32_t val = (uint32_t)byteArrayToInt(data, offset);
    return *(float*)(void*)&val;
}

//-----------------------------------------------------------------------------
short
M4Controller::byteArrayToShort(QByteArray data, int offset, bool isBigEndian)
{
    short iRetVal = -1;
    if (data.size() < offset + 2)
        return iRetVal;
    int iLow;
    int iHigh;
    if (isBigEndian) {
        iLow    = data[offset + 1];
        iHigh   = data[offset + 0];
    } else {
        iLow    = data[offset + 0];
        iHigh   = data[offset + 1];
    }
    iRetVal = (iHigh << 8) | (0xFF & iLow);
    return iRetVal;
}

#endif
