/*!
 * @file
 *   @brief ST16 Controller
 *   @author Gus Grubba <mavlink@grubba.com>
 *
 */

/*-----------------------------------------------------------------------------
 *   Original source: DroneFly/droneservice/src/main/java/com/yuneec/droneservice/parse/St16Controller.java
 */

#include "QGCCustom.h"
#include "SerialComm.h"
#include <QDebug>
#include <QSettings>
#include <math.h>

static const char* kUartName    = "/dev/ttyMFD0";

static const char* kRxInfoGroup = "YuneecRxInfo";
static const char* kmode        = "mode";
static const char* kpanId       = "panId";
static const char* knodeId      = "nodeId";
static const char* kaNum        = "aNum";
static const char* kaBit        = "aBit";
static const char* kswNum       = "swNum";
static const char* kswBit       = "swBit";
static const char* ktxAddr      = "txAddr";

#define COMMAND_WAIT_INTERVAL   250

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
// RC Channel data provided by Yuneec
#include "ChannelData.inc"

//-----------------------------------------------------------------------------
/** x^8 + x^2 + x + 1 */
uint8_t
QGCCustom::crc8(uint8_t* buffer, int len)
{
    uint8_t ret = 0;
    for(int i = 0; i < len; ++i) {
        ret = CRC8T[ret ^ buffer[i]];
    }
    return ret;
}

//-----------------------------------------------------------------------------
QGCCustom::QGCCustom(QObject* parent)
    : QObject(parent)
    , _state(STATE_NONE)
    , _enterBindCount(0)
    , _currentChannelAdd(0)
{
    _commPort = new M4SerialComm(this);
}

//-----------------------------------------------------------------------------
QGCCustom::~QGCCustom()
{
    if(_commPort) {
        delete _commPort;
    }
}

//-----------------------------------------------------------------------------
bool
QGCCustom::init(QGCApplication* /*pApp*/)
{
    qDebug() << "Init M4 Handler";
    if(!_commPort || !_commPort->init(kUartName, 230400) || !_commPort->open()) {
        qWarning() << "Could not start serial communication with M4";
        return false;
    }
    connect(_commPort, &M4SerialComm::bytesReady, this, &QGCCustom::_bytesReady);
    connect(&_timer, &QTimer::timeout, this, &QGCCustom::_checkBindState);
    _timer.setSingleShot(true);
    //-- Have we bound before?
    QSettings settings;
    settings.beginGroup(kRxInfoGroup);
    if(settings.contains(knodeId) && settings.contains(kaNum)) {
        _rxBindInfoFeedback.mode     = settings.value(kmode,   0).toInt();
        _rxBindInfoFeedback.panId    = settings.value(kpanId,  0).toInt();
        _rxBindInfoFeedback.nodeId   = settings.value(knodeId, 0).toInt();
        _rxBindInfoFeedback.aNum     = settings.value(kaNum,   0).toInt();
        _rxBindInfoFeedback.aBit     = settings.value(kaBit,   0).toInt();
        _rxBindInfoFeedback.swNum    = settings.value(kswNum,  0).toInt();
        _rxBindInfoFeedback.swBit    = settings.value(kswBit,  0).toInt();
        _rxBindInfoFeedback.txAddr   = settings.value(ktxAddr, 0).toInt();
    }
    settings.endGroup();
    bool res = false;
//    if(_rxBindInfoFeedback.nodeId) {
//        //-- We have previously bound but I don't know the proper sequence in this case
//        qDebug() << "Previously bound with:" << _rxBindInfoFeedback.nodeId;
//        _state = STATE_ENTER_RUN;
//        _enterRun();
//    } else {
        res = _startBindingSequence();
//    }
    return res;
}

bool
QGCCustom::_startBindingSequence()
{
    _enterBindCount = 0;
    _state = STATE_ENTER_BIND;
    _enterBind();
    _timer.start(COMMAND_WAIT_INTERVAL);
    return true;
}

//-----------------------------------------------------------------------------
void
QGCCustom::_checkBindState()
{
    switch(_state) {
        case STATE_ENTER_BIND:
            qDebug() << "STATE_ENTER_BIND Timeout";
            if(_enterBindCount > 10) {
                qWarning() << "Too many STATE_ENTER_BIND Timeouts";
            } else {
                _enterBind();
                _timer.start(COMMAND_WAIT_INTERVAL);
                _enterBindCount++;
            }
            break;

        case STATE_START_BIND:
            qDebug() << "STATE_START_BIND Timeout";
            _startBind();
            //-- Wait a bit longer as there may not be anyone listening
            _timer.start(1000);
            break;

        case STATE_BIND:
            qDebug() << "STATE_BIND Timeout";
            _bind(_rxBindInfoFeedback.nodeId);
            _timer.start(COMMAND_WAIT_INTERVAL);
            break;

        case STATE_QUERY_BIND:
            qDebug() << "STATE_QUERY_BIND Timeout";
            _queryBindState();
            _timer.start(COMMAND_WAIT_INTERVAL);
            break;

        case STATE_EXIT_BIND:
            qDebug() << "STATE_EXIT_BIND Timeout";
            _exitBind();
            _timer.start(COMMAND_WAIT_INTERVAL);
            break;

        case STATE_RECV_BOTH_CH:
            qDebug() << "STATE_RECV_BOTH_CH Timeout";
            _sendRecvBothCh();
            _timer.start(COMMAND_WAIT_INTERVAL);
            break;

        case STATE_SET_CHANNEL_SETTINGS:
            qDebug() << "STATE_SET_CHANNEL_SETTINGS Timeout";
            _setChannelSetting();
            _timer.start(COMMAND_WAIT_INTERVAL);
            break;

        case STATE_MIX_CHANNEL_DELETE:
            qDebug() << "STATE_MIX_CHANNEL_DELETE Timeout";
            _syncMixingDataDeleteAll();
            _timer.start(COMMAND_WAIT_INTERVAL);
            break;

        case STATE_MIX_CHANNEL_ADD:
            qDebug() << "STATE_MIX_CHANNEL_ADD Timeout";
            //-- We need to delete and send again
            _state = STATE_MIX_CHANNEL_DELETE;
            _syncMixingDataDeleteAll();
            _timer.start(COMMAND_WAIT_INTERVAL);
            break;

        case STATE_SEND_RX_INFO:
            qDebug() << "STATE_SEND_RX_INFO Timeout";
            _sendRxResInfo();
            _timer.start(COMMAND_WAIT_INTERVAL);
            break;

        case STATE_ENTER_RUN:
            qDebug() << "STATE_ENTER_RUN Timeout";
            _enterRun();
            _timer.start(COMMAND_WAIT_INTERVAL);
            break;

        default:
            qDebug() << "Timeout:" << _state;
            break;
    }
}

//-----------------------------------------------------------------------------
/**
 * This command is used for entering the progress of binding aircraft.
 * This command is the first step of the progress of binging aircraft.
 * The next command you will send may be {@link _startBind}.
 */
bool
QGCCustom::_enterRun()
{
    qDebug() << "CMD_ENTER_RUN";
    m4Command enterRunCmd(Yuneec::CMD_ENTER_RUN);
    QByteArray cmd = enterRunCmd.pack();
    qDebug() << cmd.toHex();
    return _commPort->write(cmd);
}

//-----------------------------------------------------------------------------
/**
 * This command is used for stopping control aircraft.
 */
bool
QGCCustom::_exitRun()
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
QGCCustom::_enterBind()
{
    qDebug() << "CMD_ENTER_BIND";
    m4Command enterBindCmd(Yuneec::CMD_ENTER_BIND);
    QByteArray cmd = enterBindCmd.pack();
    qDebug() << cmd.toHex();
    return _commPort->write(cmd);
}

//-----------------------------------------------------------------------------
/**
 * Use this command to set the type of channel receive original hardware signal values and encoding values.
 */
bool
QGCCustom::_sendRecvBothCh()
{
    qDebug() << "CMD_RECV_BOTH_CH";
    m4Command enterRecvCmd(Yuneec::CMD_RECV_BOTH_CH);
    QByteArray cmd = enterRecvCmd.pack();
    qDebug() << cmd.toHex();
    return _commPort->write(cmd);
}

//-----------------------------------------------------------------------------
/**
 * This command is used for exiting the progress of binding.
 */
bool
QGCCustom::_exitBind()
{
    qDebug() << "CMD_EXIT_BIND";
    m4Command exitBindCmd(Yuneec::CMD_EXIT_BIND);
    QByteArray cmd = exitBindCmd.pack();
    qDebug() << cmd.toHex();
    return _commPort->write(cmd);
}

//-----------------------------------------------------------------------------
/**
 * After {@link _enterBind} response rightly, send this command to get a list of aircraft which can be bind.
 * The next command you will send may be {@link _bind}.
 */
bool
QGCCustom::_startBind()
{
    qDebug() << "CMD_START_BIND";
    m4Message startBindMsg(Yuneec::CMD_START_BIND, Yuneec::TYPE_BIND);
    QByteArray msg = startBindMsg.pack();
    qDebug() << msg.toHex();
    return _commPort->write(msg);
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
QGCCustom::_bind(int rxAddr)
{
    qDebug() << "CMD_BIND";
    m4Message bindMsg(Yuneec::CMD_BIND, Yuneec::TYPE_BIND);
    bindMsg.data[4] = (uint8_t)(rxAddr & 0xff);
    bindMsg.data[5] = (uint8_t)((rxAddr & 0xff00) >> 8);
    bindMsg.data[6] = 5; //-- Gotta love magic numbers
    bindMsg.data[7] = 15;
    QByteArray msg = bindMsg.pack();
    qDebug() << msg.toHex();
    return _commPort->write(msg);
}

//-----------------------------------------------------------------------------
/**
 * This command is used for setting the number of analog channel and switch channel.
 * After binding aircraft successful, you need to send this command.
 * Analog channel represent which controller has a smooth step changed value, like a rocker.
 * Switch channel represent which controller has two or three state of value, like flight mode switcher.
 * The next command you will send may be {@link _syncMixingDataDeleteAll}.
 */
bool
QGCCustom::_setChannelSetting()
{
    qDebug() << "CMD_SET_CHANNEL_SETTING";
    m4Command setChannelSettingCmd(Yuneec::CMD_SET_CHANNEL_SETTING);
    QByteArray payload;
    payload.fill(0, 2);
    payload[0] = _rxBindInfoFeedback.aNum  & 0xff;
    payload[1] = _rxBindInfoFeedback.swNum & 0xff;
    QByteArray cmd = setChannelSettingCmd.pack(payload);
    qDebug() << cmd.toHex();
    return _commPort->write(cmd);
}

//-----------------------------------------------------------------------------
/**
 * This command is used for disconnecting the bound aircraft.
 * Suggest to use this command before using {@link _bind} first time.
 */
bool
QGCCustom::_unbind()
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
QGCCustom::_queryBindState()
{
    qDebug() << "CMD_QUERY_BIND_STATE";
    m4Command queryBindStateCmd(Yuneec::CMD_QUERY_BIND_STATE);
    QByteArray cmd = queryBindStateCmd.pack();
    qDebug() << cmd.toHex();
    return _commPort->write(cmd);
}

//-----------------------------------------------------------------------------
/**
 * This command is used for deleting all channel formula data synchronously.
 * See {@link _syncMixingDataAdd}.
 */
bool
QGCCustom::_syncMixingDataDeleteAll()
{
    qDebug() << "CMD_SYNC_MIXING_DATA_DELETE_ALL";
    m4Command syncMixingDataDeleteAllCmd(Yuneec::CMD_SYNC_MIXING_DATA_DELETE_ALL);
    QByteArray cmd = syncMixingDataDeleteAllCmd.pack();
    qDebug() << cmd.toHex();
    return _commPort->write(cmd);
}

//-----------------------------------------------------------------------------
/**
 * This command is used for adding channel formula data synchronously.
 * You need to send all the channel formula data successful,
 * if not, send {@link _syncMixingDataDeleteAll} again.
 * Channel formula make the same hardware signal values to the different values we get finally. (What?)
 */
bool
QGCCustom::_syncMixingDataAdd()
{
    qDebug() << "CMD_SYNC_MIXING_DATA_ADD";
    m4Command syncMixingDataAddCmd(Yuneec::CMD_SYNC_MIXING_DATA_ADD);
    QByteArray payload((const char*)&channel_data[_currentChannelAdd], CHANNEL_LENGTH);
    QByteArray cmd = syncMixingDataAddCmd.pack(payload);
    qDebug() << cmd.toHex();
    return _commPort->write(cmd);
}

//-----------------------------------------------------------------------------
/**
 * This command is used for sending the information we got from bound aircraft to the bound aircraft
 * to make a confirmation.
 * If send successful, the you can send {@link _enterRun}.
 */
bool
QGCCustom::_sendRxResInfo()
{
    qDebug() << "CMD_SEND_RX_RESINFO";
    m4Command sendRxResInfoCmd(Yuneec::CMD_SEND_RX_RESINFO);
    QByteArray payload;
    payload.fill(0, 44);
    payload[6]  =  _rxBindInfoFeedback.mode     & 0xff;
    payload[7]  = (_rxBindInfoFeedback.mode     & 0xff00) >> 8;
    payload[8]  =  _rxBindInfoFeedback.panId    & 0xff;
    payload[9]  = (_rxBindInfoFeedback.panId    & 0xff00) >> 8;
    payload[10] =  _rxBindInfoFeedback.nodeId   & 0xff;
    payload[11] = (_rxBindInfoFeedback.nodeId   & 0xff00) >> 8;
    payload[20] =  _rxBindInfoFeedback.aNum;
    payload[21] =  _rxBindInfoFeedback.aBit;
    payload[24] =  _rxBindInfoFeedback.swNum;
    payload[25] =  _rxBindInfoFeedback.swBit;
    payload[42] =  _rxBindInfoFeedback.txAddr   & 0xff;
    payload[43] = (_rxBindInfoFeedback.txAddr   & 0xff00) >> 8;
    QByteArray cmd = sendRxResInfoCmd.pack(payload);
    qDebug() << cmd.toHex();
    return _commPort->write(cmd);
}

//-----------------------------------------------------------------------------
/*
 * A full, validate message has been received.
 */
void
QGCCustom::_bytesReady(QByteArray data)
{
    m4Packet packet(data);
    int type = packet.type();
    //-- Some Yuneec voodoo
    type = (type & 0x1c) >> 2;
    if(_handleNonTypePacket(packet)) {
        return;
    }
    switch(type) {
        case Yuneec::TYPE_BIND:
            switch((uint8_t)data[3]) {
                case 2:
                    _handleRxBindInfo(packet);
                    break;
                case 4:
                    _handleBindResponse();
                    break;
                default:
                    _timer.stop();
                    qDebug() << "M4 Packet: TYPE_BIND Unknown:" << data.toHex();
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
            switch(packet.commandID()) {
                case Yuneec::CMD_QUERY_BIND_STATE:
                    //-- Response from _queryBindState()
                    _handleQueryBindResponse(data);
                    break;
                case Yuneec::CMD_ENTER_BIND:
                    //-- Response from _enterBind()
                    qDebug() << "M4 Packet: CMD_ENTER_BIND";
                    if(_state == STATE_ENTER_BIND) {
                        //-- Now we start scanning
                        _state = STATE_START_BIND;
                        _startBind();
                        _timer.start(COMMAND_WAIT_INTERVAL);
                    }
                    break;
                case Yuneec::CMD_EXIT_BIND:
                    //-- Response from _exitBind()
                    qDebug() << "M4 Packet: CMD_EXIT_BIND";
                    if(_state == STATE_EXIT_BIND) {
                        _state = STATE_RECV_BOTH_CH;
                        _sendRecvBothCh();
                        _timer.start(COMMAND_WAIT_INTERVAL);
                    }
                    break;
                case Yuneec::CMD_RECV_BOTH_CH:
                    //-- Response from _sendRecvBothCh()
                    qDebug() << "M4 Packet: CMD_RECV_BOTH_CH";
                    if(_state == STATE_RECV_BOTH_CH) {
                        _state = STATE_SET_CHANNEL_SETTINGS;
                        _setChannelSetting();
                        _timer.start(COMMAND_WAIT_INTERVAL);
                    }
                    break;
                case Yuneec::CMD_SET_CHANNEL_SETTING:
                    //-- Response from _setChannelSetting()
                    qDebug() << "M4 Packet: CMD_SET_CHANNEL_SETTING";
                    if(_state == STATE_SET_CHANNEL_SETTINGS) {
                        _state = STATE_MIX_CHANNEL_DELETE;
                        _syncMixingDataDeleteAll();
                        _timer.start(COMMAND_WAIT_INTERVAL);
                    }
                    break;
                case Yuneec::CMD_SYNC_MIXING_DATA_DELETE_ALL:
                    //-- Response from _syncMixingDataDeleteAll()
                    qDebug() << "M4 Packet: CMD_SYNC_MIXING_DATA_DELETE_ALL";
                    if(_state == STATE_MIX_CHANNEL_DELETE) {
                        _state = STATE_MIX_CHANNEL_ADD;
                        _currentChannelAdd = 0;
                        _syncMixingDataAdd();
                        //-- Wait longer
                        _timer.start(500);
                    }
                    break;
                case Yuneec::CMD_SYNC_MIXING_DATA_ADD:
                    //-- Response from _syncMixingDataAdd()
                    qDebug() << "M4 Packet: CMD_SYNC_MIXING_DATA_ADD" << _currentChannelAdd;
                    if(_state == STATE_MIX_CHANNEL_ADD) {
                        _currentChannelAdd++;
                        if(_currentChannelAdd < NUM_CHANNELS) {
                            _syncMixingDataAdd();
                            //-- Wait longer
                            _timer.start(500);
                        } else {
                            _state = STATE_SEND_RX_INFO;
                            _sendRxResInfo();
                            _timer.start(COMMAND_WAIT_INTERVAL);
                        }
                    }
                    break;
                case Yuneec::CMD_SEND_RX_RESINFO:
                    //-- Response from _sendRxResInfo()
                    qDebug() << "M4 Packet: TYPE_RSP CMD_SEND_RX_RESINFO";
                    if(_state == STATE_SEND_RX_INFO) {
                        _state = STATE_ENTER_RUN;
                        _enterRun();
                        _timer.start(COMMAND_WAIT_INTERVAL);
                    }
                    break;
                case Yuneec::CMD_ENTER_RUN:
                    //-- Response from _enterRun()
                    qDebug() << "M4 Packet: TYPE_RSP CMD_ENTER_RUN";
                    if(_state == STATE_ENTER_RUN) {
                        _state = STATE_RUNNING;
                        _timer.stop();
                        qDebug() << "M4 ready, in run state.";
                    }
                    break;
                default:
                    qDebug() << "M4 Packet: TYPE_RSP ???" << packet.commandID();
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
void
QGCCustom::_handleQueryBindResponse(QByteArray data)
{
    BindState state;
    state.state = (data[10] & 0xff) | (data[11] << 8 & 0xff00);
    qDebug() << "M4 Packet: CMD_QUERY_BIND_STATE" << state.state;
    if(_state == STATE_QUERY_BIND) {
        if(state.state == _rxBindInfoFeedback.nodeId) {
            _timer.stop();
            qInfo() << "Switched to BOUND state with:" << _rxBindInfoFeedback.getName();
            _state = STATE_EXIT_BIND;
            _exitBind();
            //-- Store RX Info
            QSettings settings;
            settings.beginGroup(kRxInfoGroup);
            settings.setValue(kmode,  _rxBindInfoFeedback.mode);
            settings.setValue(kpanId, _rxBindInfoFeedback.panId);
            settings.setValue(knodeId,_rxBindInfoFeedback.nodeId);
            settings.setValue(kaNum,  _rxBindInfoFeedback.aNum);
            settings.setValue(kaBit,  _rxBindInfoFeedback.aBit);
            settings.setValue(kswNum, _rxBindInfoFeedback.swNum);
            settings.setValue(kswBit, _rxBindInfoFeedback.swBit);
            settings.setValue(ktxAddr,_rxBindInfoFeedback.txAddr);
            settings.endGroup();
            _timer.start(COMMAND_WAIT_INTERVAL);
        } else {
            qWarning() << "Response from unkown origin:" << state.state;
        }
    }
}

//-----------------------------------------------------------------------------
bool
QGCCustom::_handleNonTypePacket(m4Packet& packet)
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
QGCCustom::_handleBindResponse()
{
    if(_state == STATE_BIND) {
        _timer.stop();
        _state = STATE_QUERY_BIND;
        _queryBindState();
        _timer.start(COMMAND_WAIT_INTERVAL);
    }
}

//-----------------------------------------------------------------------------
void
QGCCustom::_handleRxBindInfo(m4Packet& packet)
{
    qDebug() << "M4 Packet: TYPE_BIND with rxBindInfoFeedback";
    if(_state == STATE_START_BIND) {
        _timer.stop();
        _rxBindInfoFeedback.mode     = ((uint8_t)packet.data[6]  & 0xff) | ((uint8_t)packet.data[7]  << 8 & 0xff00);
        _rxBindInfoFeedback.panId    = ((uint8_t)packet.data[8]  & 0xff) | ((uint8_t)packet.data[9]  << 8 & 0xff00);
        _rxBindInfoFeedback.nodeId   = ((uint8_t)packet.data[10] & 0xff) | ((uint8_t)packet.data[11] << 8 & 0xff00);
        _rxBindInfoFeedback.aNum     = (uint8_t)packet.data[20];
        _rxBindInfoFeedback.aBit     = (uint8_t)packet.data[21];
        _rxBindInfoFeedback.swNum    = (uint8_t)packet.data[24];
        _rxBindInfoFeedback.swBit    = (uint8_t)packet.data[25];
        int p = packet.data.length() - 2;
        _rxBindInfoFeedback.txAddr = ((uint8_t)packet.data[p] & 0xff) | ((uint8_t)packet.data[p + 1] << 8 & 0xff00);
        qDebug() << "M4 Packet: RxBindInfo:" << _rxBindInfoFeedback.getName() << _rxBindInfoFeedback.nodeId;
        _state = STATE_BIND;
        _bind(_rxBindInfoFeedback.nodeId);
        _timer.start(500);
    }
}

//-----------------------------------------------------------------------------
void
QGCCustom::_handleChannel(m4Packet& packet)
{
    Q_UNUSED(packet);
    switch(packet.commandID()) {
        case Yuneec::CMD_RX_FEEDBACK_DATA:
            qDebug() << "M4 Packet: CMD_RX_FEEDBACK_DATA";
            /* From original Java code
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
QGCCustom::_handleCommand(m4Packet& packet)
{
    Q_UNUSED(packet);
    switch(packet.commandID()) {
        case Yuneec::CMD_TX_STATE_MACHINE:
            {
                QByteArray commandValues = packet.commandValues();
                int status = commandValues[0] & 0x1f;
                /* From original Java code
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
QGCCustom::_switchChanged(m4Packet& packet)
{
    Q_UNUSED(packet);
    QByteArray commandValues = packet.commandValues();
    SwitchChanged switchChanged;
    switchChanged.hwId      = commandValues[0];
    switchChanged.oldState  = commandValues[1];
    switchChanged.newState  = commandValues[2];
    // From original Java code
    //controllerFeedbackListener.onSwitchChanged(switchChanged);
    qDebug() << "M4 Packet: CMD_TX_SWITCH_CHANGED" << switchChanged.hwId << switchChanged.oldState << switchChanged.newState;
}

//-----------------------------------------------------------------------------
void
QGCCustom::_handleMixedChannelData(m4Packet& packet)
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
    // From original Java code
    //controllerFeedbackListener.onReceiveChannelValues(channels);
}

//-----------------------------------------------------------------------------
void
QGCCustom::_handControllerFeedback(m4Packet& packet) {
    QByteArray commandValues = packet.commandValues();
    ControllerLocation controllerLocation;
    controllerLocation.latitude     = byteArrayToInt(commandValues, 0) / 1e7;
    controllerLocation.longitude    = byteArrayToInt(commandValues, 4) / 1e7;
    controllerLocation.altitude     = byteArrayToFloat(commandValues, 8);
    controllerLocation.accuracy     = byteArrayToShort(commandValues, 12);
    controllerLocation.speed        = byteArrayToShort(commandValues, 14);
    controllerLocation.angle        = byteArrayToShort(commandValues, 16);
    controllerLocation.satelliteCount = commandValues[18] & 0x1f;
    // From original Java code
    //controllerFeedbackListener.onReceiveControllerLocation(controllerLocation);
    qDebug() << "M4 Packet: COMMAND_M4_SEND_GPS_DATA_TO_PA" << controllerLocation.satelliteCount << controllerLocation.latitude << controllerLocation.longitude << controllerLocation.altitude;
}

//-----------------------------------------------------------------------------
int
QGCCustom::byteArrayToInt(QByteArray data, int offset, bool isBigEndian)
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
QGCCustom::byteArrayToFloat(QByteArray data, int offset)
{
    uint32_t val = (uint32_t)byteArrayToInt(data, offset);
    return *(float*)(void*)&val;
}

//-----------------------------------------------------------------------------
short
QGCCustom::byteArrayToShort(QByteArray data, int offset, bool isBigEndian)
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
