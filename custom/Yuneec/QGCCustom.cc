/*!
 * @file
 *   @brief ST16 Controller
 *   @author Gus Grubba <mavlink@grubba.com>
 *
 */

/*-----------------------------------------------------------------------------
 *   Original source:
 *
 *   DroneFly/droneservice/src/main/java/com/yuneec/droneservice/parse/St16Controller.java
 *
 *   All comments within the command send functions came from the original file above.
 *   The functions themselves have been completely rewriten from scratch.
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

#define COMMAND_RESPONSE_TRIES  4
#define COMMAND_WAIT_INTERVAL   250
#define DEBUG_DATA_DUMP         false

QGC_LOGGING_CATEGORY(YuneecLog, "YuneecLog")

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
#if 0
static QString
dump_data_packet(QByteArray data)
{
    QString resp;
    QString temp;
    resp += "\n";
    for(int i = 0; i < data.size(); i++) {
        temp.sprintf("%03d, ", i);
        resp += temp;
    }
    resp += "\n";
    for(int i = 0; i < data.size(); i++) {
        temp.sprintf("%03d, ", (uint8_t)data[i]);
        resp += temp;
    }
    resp += "\n";
    for(int i = 0; i < data.size(); i++) {
        temp.sprintf(" %02X, ", (uint8_t)data[i]);
        resp += temp;
    }
    return resp;
}
#endif
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
    , _responseTryCount(0)
    , _currentChannelAdd(0)
    , _m4State(M4_STATE_NONE)
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
    //-- Doing this here for the time being as there is no alternative on Android
#if defined(QT_DEBUG)
    QLoggingCategory::setFilterRules(QStringLiteral("*.debug=false"));
    QLoggingCategory::setFilterRules(QStringLiteral("YuneecLog.debug=true"));
#endif
    qCDebug(YuneecLog) << "Init M4 Handler";
    if(!_commPort || !_commPort->init(kUartName, 230400) || !_commPort->open()) {
        qWarning() << "Could not start serial communication with M4";
        return false;
    }
    connect(_commPort, &M4SerialComm::bytesReady, this, &QGCCustom::_bytesReady);
    connect(&_timer, &QTimer::timeout, this, &QGCCustom::_stateManager);
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
    //-- For now, make Power Key (Start/Stop on top of the ST16) work as a power button.
    _setPowerKey(Yuneec::BIND_KEY_FUNCTION_PWR);
    QThread::msleep(50);
    //-- Start with Exit run mode first. Eventually, when this is complete we
    //   will check the current state (_m4State) to decide what to do.
    _exitRun();
    QThread::msleep(50);
    _state = STATE_EXIT_RUN;
    _timer.start(COMMAND_WAIT_INTERVAL);
    return true;
}

//-----------------------------------------------------------------------------
void
QGCCustom::_initSequence()
{
    _responseTryCount = 0;
    //-- Check and see if we have binding info
    if(_rxBindInfoFeedback.nodeId) {
        qCDebug(YuneecLog) << "Previously bound with:" << _rxBindInfoFeedback.nodeId << "(" << _rxBindInfoFeedback.aNum << "Analog Channels ) (" << _rxBindInfoFeedback.swNum << "Switches )";
        //-- Initialize M4
        _state = STATE_RECV_BOTH_CH;
        _sendRecvBothCh();
    } else {
        //-- First run. Start binding sequence
        _responseTryCount = 0;
        _state = STATE_ENTER_BIND;
        _enterBind();
    }
    _timer.start(COMMAND_WAIT_INTERVAL);
}

//-----------------------------------------------------------------------------
/*
 * This handles the sequence of events/commands sent to the MCU. A following
 * command is only sent once we receive an response from the previous one. If
 * no response is received, the command is sent again.
 *
 */
void
QGCCustom::_stateManager()
{
    switch(_state) {

        case STATE_EXIT_RUN:
            qCDebug(YuneecLog) << "STATE_EXIT_RUN Timeout";
            if(_responseTryCount > COMMAND_RESPONSE_TRIES) {
                qWarning() << "Too many STATE_EXIT_RUN Timeouts. Switching to initial run.";
                _initSequence();
            } else {
                _exitRun();
                _timer.start(COMMAND_WAIT_INTERVAL);
                _responseTryCount++;
            }
            break;

        case STATE_ENTER_BIND:
            qCDebug(YuneecLog) << "STATE_ENTER_BIND Timeout";
            if(_responseTryCount > COMMAND_RESPONSE_TRIES) {
                qWarning() << "Too many STATE_ENTER_BIND Timeouts.";
                if(_rxBindInfoFeedback.nodeId) {
                    _responseTryCount = 0;
                    _state = STATE_SEND_RX_INFO;
                    _sendRxResInfo();
                    _timer.start(COMMAND_WAIT_INTERVAL);
                } else {
                    //-- We're stuck. Wen can't enter bind and we have no binding info.
                    qCritical() << "Cannot enter binding mode";
                    _state = STATE_ENTER_BIND_ERROR;
                }
            } else {
                _enterBind();
                _timer.start(COMMAND_WAIT_INTERVAL);
                _responseTryCount++;
            }
            break;

        case STATE_START_BIND:
            qCDebug(YuneecLog) << "STATE_START_BIND Timeout";
            _startBind();
            //-- Wait a bit longer as there may not be anyone listening
            _timer.start(1000);
            //-- TODO: This can't wait for ever...
            break;

        case STATE_UNBIND:
            qCDebug(YuneecLog) << "STATE_UNBIND Timeout";
            if(_responseTryCount > COMMAND_RESPONSE_TRIES) {
                qWarning() << "Too many STATE_UNBIND Timeouts. Go straight to bind.";
                _responseTryCount = 0;
                _state = STATE_BIND;
                _bind(_rxBindInfoFeedback.nodeId);
                _timer.start(COMMAND_WAIT_INTERVAL);
            } else {
                _unbind();
                _timer.start(COMMAND_WAIT_INTERVAL);
                _responseTryCount++;
            }
            break;

        case STATE_BIND:
            qCDebug(YuneecLog) << "STATE_BIND Timeout";
            _bind(_rxBindInfoFeedback.nodeId);
            _timer.start(COMMAND_WAIT_INTERVAL);
            //-- TODO: This can't wait for ever...
            break;

        case STATE_QUERY_BIND:
            qCDebug(YuneecLog) << "STATE_QUERY_BIND Timeout";
            _queryBindState();
            _timer.start(COMMAND_WAIT_INTERVAL);
            //-- TODO: This can't wait for ever...
            break;

        case STATE_EXIT_BIND:
            qCDebug(YuneecLog) << "STATE_EXIT_BIND Timeout";
            _exitBind();
            _timer.start(COMMAND_WAIT_INTERVAL);
            //-- TODO: This can't wait for ever...
            break;

        case STATE_RECV_BOTH_CH:
            if(_responseTryCount > COMMAND_RESPONSE_TRIES) {
                qWarning() << "Too many STATE_RECV_BOTH_CH Timeouts. Giving up...";
            } else {
                qCDebug(YuneecLog) << "STATE_RECV_BOTH_CH Timeout";
                _sendRecvBothCh();
                _timer.start(COMMAND_WAIT_INTERVAL);
                _responseTryCount++;
            }
            break;

        case STATE_SET_CHANNEL_SETTINGS:
            qCDebug(YuneecLog) << "STATE_SET_CHANNEL_SETTINGS Timeout";
            _setChannelSetting();
            _timer.start(COMMAND_WAIT_INTERVAL);
            //-- TODO: This can't wait for ever...
            break;

        case STATE_MIX_CHANNEL_DELETE:
            qCDebug(YuneecLog) << "STATE_MIX_CHANNEL_DELETE Timeout";
            _syncMixingDataDeleteAll();
            _timer.start(COMMAND_WAIT_INTERVAL);
            //-- TODO: This can't wait for ever...
            break;

        case STATE_MIX_CHANNEL_ADD:
            qCDebug(YuneecLog) << "STATE_MIX_CHANNEL_ADD Timeout";
            //-- We need to delete and send again
            _state = STATE_MIX_CHANNEL_DELETE;
            _syncMixingDataDeleteAll();
            _timer.start(COMMAND_WAIT_INTERVAL);
            //-- TODO: This can't wait for ever...
            break;

        case STATE_SEND_RX_INFO:
            if(_responseTryCount > COMMAND_RESPONSE_TRIES) {
                qWarning() << "Too many STATE_SEND_RX_INFO Timeouts. Giving up...";
            } else {
                qCDebug(YuneecLog) << "STATE_SEND_RX_INFO Timeout";
                _sendRxResInfo();
                _timer.start(COMMAND_WAIT_INTERVAL);
                _responseTryCount++;
            }
            break;

        case STATE_ENTER_RUN:
            if(_responseTryCount > COMMAND_RESPONSE_TRIES) {
                qWarning() << "Too many STATE_ENTER_RUN Timeouts. Giving up...";
            } else {
                qCDebug(YuneecLog) << "STATE_ENTER_RUN Timeout";
                _enterRun();
                _timer.start(COMMAND_WAIT_INTERVAL);
                _responseTryCount++;
            }
            break;

        default:
            qCDebug(YuneecLog) << "Timeout:" << _state;
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
    qCDebug(YuneecLog) << "Sending: CMD_ENTER_RUN";
    m4Command enterRunCmd(Yuneec::CMD_ENTER_RUN);
    QByteArray cmd = enterRunCmd.pack();
    return _commPort->write(cmd, DEBUG_DATA_DUMP);
}

//-----------------------------------------------------------------------------
/**
 * This command is used for stopping control aircraft.
 */
bool
QGCCustom::_exitRun()
{
    qCDebug(YuneecLog) << "Sending: CMD_EXIT_RUN";
    m4Command exitRunCmd(Yuneec::CMD_EXIT_RUN);
    QByteArray cmd = exitRunCmd.pack();
    return _commPort->write(cmd, DEBUG_DATA_DUMP);
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
    qCDebug(YuneecLog) << "Sending: CMD_ENTER_BIND";
    m4Command enterBindCmd(Yuneec::CMD_ENTER_BIND);
    QByteArray cmd = enterBindCmd.pack();
    return _commPort->write(cmd, DEBUG_DATA_DUMP);
}

//-----------------------------------------------------------------------------
/**
 * Use this command to set the type of channel receive original hardware signal values and encoding values.
 */
//-- TODO: Do we really need raw data? Maybe CMD_RECV_MIXED_CH_ONLY would be enough.
bool
QGCCustom::_sendRecvBothCh()
{
    qCDebug(YuneecLog) << "Sending: CMD_RECV_BOTH_CH";
    m4Command enterRecvCmd(Yuneec::CMD_RECV_BOTH_CH);
    QByteArray cmd = enterRecvCmd.pack();
    return _commPort->write(cmd, DEBUG_DATA_DUMP);
}

//-----------------------------------------------------------------------------
/**
 * This command is used for exiting the progress of binding.
 */
bool
QGCCustom::_exitBind()
{
    qCDebug(YuneecLog) << "Sending: CMD_EXIT_BIND";
    m4Command exitBindCmd(Yuneec::CMD_EXIT_BIND);
    QByteArray cmd = exitBindCmd.pack();
    return _commPort->write(cmd, DEBUG_DATA_DUMP);
}

//-----------------------------------------------------------------------------
/**
 * After {@link _enterBind} response rightly, send this command to get a list of aircraft which can be bind.
 * The next command you will send may be {@link _bind}.
 */
bool
QGCCustom::_startBind()
{
    qCDebug(YuneecLog) << "Sending: CMD_START_BIND";
    m4Message startBindMsg(Yuneec::CMD_START_BIND, Yuneec::TYPE_BIND);
    QByteArray msg = startBindMsg.pack();
    return _commPort->write(msg, DEBUG_DATA_DUMP);
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
    qCDebug(YuneecLog) << "Sending: CMD_BIND";
    m4Message bindMsg(Yuneec::CMD_BIND, Yuneec::TYPE_BIND);
    bindMsg.data[4] = (uint8_t)(rxAddr & 0xff);
    bindMsg.data[5] = (uint8_t)((rxAddr & 0xff00) >> 8);
    bindMsg.data[6] = 5; //-- Gotta love magic numbers
    bindMsg.data[7] = 15;
    QByteArray msg = bindMsg.pack();
    return _commPort->write(msg, DEBUG_DATA_DUMP);
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
    qCDebug(YuneecLog) << "Sending: CMD_SET_CHANNEL_SETTING";
    m4Command setChannelSettingCmd(Yuneec::CMD_SET_CHANNEL_SETTING);
    QByteArray payload;
    payload.fill(0, 2);
    payload[0] = (uint8_t)(_rxBindInfoFeedback.aNum  & 0xff);
    payload[1] = (uint8_t)(_rxBindInfoFeedback.swNum & 0xff);
    QByteArray cmd = setChannelSettingCmd.pack(payload);
    return _commPort->write(cmd, DEBUG_DATA_DUMP);
}

//-----------------------------------------------------------------------------
/**
 * This command is used for setting if power key is working.
 * Parameter of {@link #_setPowerKey(int)}, represent the function
 * of power key is working. For example, when you click power key, the screen will light up or
 * go out if set the value {@link BaseCommand#BIND_KEY_FUNCTION_PWR}.
 */
bool
QGCCustom::_setPowerKey(int function)
{
    qCDebug(YuneecLog) << "Sending: CMD_SET_BINDKEY_FUNCTION";
    m4Command setPowerKeyCmd(Yuneec::CMD_SET_BINDKEY_FUNCTION);
    QByteArray payload;
    payload.resize(1);
    payload[0] = (uint8_t)(function & 0xff);
    QByteArray cmd = setPowerKeyCmd.pack(payload);
    return _commPort->write(cmd, DEBUG_DATA_DUMP);
}

//-----------------------------------------------------------------------------
/**
 * This command is used for disconnecting the bound aircraft.
 * Suggest to use this command before using {@link _bind} first time.
 */
bool
QGCCustom::_unbind()
{
    qCDebug(YuneecLog) << "Sending: CMD_UNBIND";
    m4Command unbindCmd(Yuneec::CMD_UNBIND);
    QByteArray cmd = unbindCmd.pack();
    return _commPort->write(cmd, DEBUG_DATA_DUMP);
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
    qCDebug(YuneecLog) << "Sending: CMD_QUERY_BIND_STATE";
    m4Command queryBindStateCmd(Yuneec::CMD_QUERY_BIND_STATE);
    QByteArray cmd = queryBindStateCmd.pack();
    return _commPort->write(cmd, DEBUG_DATA_DUMP);
}

//-----------------------------------------------------------------------------
/**
 * This command is used for deleting all channel formula data synchronously.
 * See {@link _syncMixingDataAdd}.
 */
bool
QGCCustom::_syncMixingDataDeleteAll()
{
    qCDebug(YuneecLog) << "Sending: CMD_SYNC_MIXING_DATA_DELETE_ALL";
    m4Command syncMixingDataDeleteAllCmd(Yuneec::CMD_SYNC_MIXING_DATA_DELETE_ALL);
    QByteArray cmd = syncMixingDataDeleteAllCmd.pack();
    return _commPort->write(cmd, DEBUG_DATA_DUMP);
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
    /*
     *  "Mixing Data" is an array of NUM_CHANNELS (25) sets of CHANNEL_LENGTH (96)
     *  magic bytes each. Each set is sent using this command. The documentation states
     *  that if there is an error you should send the CMD_SYNC_MIXING_DATA_DELETE_ALL
     *  command again and start over.
     *  I have not seen a way to identify an error other than getting no response once
     *  the command is sent. There doesn't appear to be a "NAK" type response.
     */

    qCDebug(YuneecLog) << "Sending: CMD_SYNC_MIXING_DATA_ADD";
    m4Command syncMixingDataAddCmd(Yuneec::CMD_SYNC_MIXING_DATA_ADD);
    QByteArray payload((const char*)&channel_data[_currentChannelAdd], CHANNEL_LENGTH);
    QByteArray cmd = syncMixingDataAddCmd.pack(payload);
    return _commPort->write(cmd, DEBUG_DATA_DUMP);
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
    /*
     * This is not working. Even though, as far as I can tell it follows the
     * directions to the letter, it seems it's wrong. We get no negative response
     * from the MCU so I can't tell what's going on. What I do know is that when
     * we receive this data structure from the MCU, it comes in a 45-byte array,
     * yet I'm told to build a 44-byte array as laid out below.
     *
     * This is what the original Java code looks like:

        public byte[] toByte() {
            int len = 44;
            byte data[] = new byte[len];
            data[6] = (byte) (mode & 0xff);
            data[7] = (byte) ((mode & 0xff00) >> 8);
            data[8] = (byte) (panId & 0xff);
            data[9] = (byte) ((panId & 0xff00) >> 8);
            data[10] = (byte) (nodeId & 0xff);
            data[11] = (byte) ((nodeId & 0xff00) >> 8);
            data[20] = (byte) aNum;
            data[21] = (byte) aBit;
            data[24] = (byte) swNum;
            data[25] = (byte) swBit;
            data[len - 2] = (byte) (txAddr & 0xff);
            data[len - 1] = (byte) ((txAddr & 0xff00) >> 8);
            return data;
        }

     */

    qCDebug(YuneecLog) << "Sending: CMD_SEND_RX_RESINFO";
    m4Command sendRxResInfoCmd(Yuneec::CMD_SEND_RX_RESINFO);
    QByteArray payload;
    int len = 44;
    payload.fill(0, len); //-- Creates a 44-byte array and fill it with zeroes
    payload[6]  = (uint8_t)( _rxBindInfoFeedback.mode     & 0xff);
    payload[7]  = (uint8_t)((_rxBindInfoFeedback.mode     & 0xff00) >> 8);
    payload[8]  = (uint8_t)( _rxBindInfoFeedback.panId    & 0xff);
    payload[9]  = (uint8_t)((_rxBindInfoFeedback.panId    & 0xff00) >> 8);
    payload[10] = (uint8_t)( _rxBindInfoFeedback.nodeId   & 0xff);
    payload[11] = (uint8_t)((_rxBindInfoFeedback.nodeId   & 0xff00) >> 8);
    payload[20] = (uint8_t)( _rxBindInfoFeedback.aNum);
    payload[21] = (uint8_t)( _rxBindInfoFeedback.aBit);
    payload[24] = (uint8_t)( _rxBindInfoFeedback.swNum);
    payload[25] = (uint8_t)( _rxBindInfoFeedback.swBit);
    payload[len - 2] = (uint8_t)( _rxBindInfoFeedback.txAddr & 0xff);
    payload[len - 1] = (uint8_t)((_rxBindInfoFeedback.txAddr & 0xff00) >> 8);
    QByteArray cmd = sendRxResInfoCmd.pack(payload);
    //qCDebug(YuneecLog) << dump_data_packet(cmd);
    return _commPort->write(cmd, DEBUG_DATA_DUMP);
}

//-----------------------------------------------------------------------------
/*
 *
 * A full, validated data packet has been received. The data argument contains
 * only the data portion. The 0x55, 0x55 header and length (3 bytes) have been
 * removed as well as the trailing CRC (1 byte).
 *
 * Code largely based on original Java code found in the St16Controller class.
 * The main difference is we also handle the machine state here.
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
                    qCDebug(YuneecLog) << "Received: TYPE_BIND Unknown:" << data.toHex();
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
                case Yuneec::CMD_EXIT_RUN:
                    //-- Response from _exitRun()
                    qCDebug(YuneecLog) << "Received TYPE_RSP: CMD_EXIT_RUN";
                    if(_state == STATE_EXIT_RUN) {
                        //-- Now we start initsequence
                        _initSequence();
                    }
                    break;
                case Yuneec::CMD_ENTER_BIND:
                    //-- Response from _enterBind()
                    qCDebug(YuneecLog) << "Received TYPE_RSP: CMD_ENTER_BIND";
                    if(_state == STATE_ENTER_BIND) {
                        //-- Now we start scanning
                        _state = STATE_START_BIND;
                        _startBind();
                        _timer.start(COMMAND_WAIT_INTERVAL);
                    }
                    break;
                case Yuneec::CMD_UNBIND:
                    qCDebug(YuneecLog) << "Received TYPE_RSP: CMD_UNBIND";
                    if(_state == STATE_UNBIND) {
                        _state = STATE_BIND;
                        _bind(_rxBindInfoFeedback.nodeId);
                        _timer.start(COMMAND_WAIT_INTERVAL);
                    }
                    break;
                case Yuneec::CMD_EXIT_BIND:
                    //-- Response from _exitBind()
                    qCDebug(YuneecLog) << "Received TYPE_RSP: CMD_EXIT_BIND";
                    if(_state == STATE_EXIT_BIND) {
                        _responseTryCount = 0;
                        _state = STATE_RECV_BOTH_CH;
                        _sendRecvBothCh();
                        _timer.start(COMMAND_WAIT_INTERVAL);
                    }
                    break;
                case Yuneec::CMD_RECV_BOTH_CH:
                    //-- Response from _sendRecvBothCh()
                    qCDebug(YuneecLog) << "Received TYPE_RSP: CMD_RECV_BOTH_CH";
                    if(_state == STATE_RECV_BOTH_CH) {
                        _state = STATE_SET_CHANNEL_SETTINGS;
                        _setChannelSetting();
                        _timer.start(COMMAND_WAIT_INTERVAL);
                    }
                    break;
                case Yuneec::CMD_SET_CHANNEL_SETTING:
                    //-- Response from _setChannelSetting()
                    qCDebug(YuneecLog) << "Received TYPE_RSP: CMD_SET_CHANNEL_SETTING";
                    if(_state == STATE_SET_CHANNEL_SETTINGS) {
                        _state = STATE_MIX_CHANNEL_DELETE;
                        _syncMixingDataDeleteAll();
                        _timer.start(COMMAND_WAIT_INTERVAL);
                    }
                    break;
                case Yuneec::CMD_SYNC_MIXING_DATA_DELETE_ALL:
                    //-- Response from _syncMixingDataDeleteAll()
                    qCDebug(YuneecLog) << "Received TYPE_RSP: CMD_SYNC_MIXING_DATA_DELETE_ALL";
                    if(_state == STATE_MIX_CHANNEL_DELETE) {
                        _state = STATE_MIX_CHANNEL_ADD;
                        _currentChannelAdd = 0;
                        _syncMixingDataAdd();
                        _timer.start(COMMAND_WAIT_INTERVAL);
                    }
                    break;
                case Yuneec::CMD_SYNC_MIXING_DATA_ADD:
                    //-- Response from _syncMixingDataAdd()
                    qCDebug(YuneecLog) << "Received TYPE_RSP: CMD_SYNC_MIXING_DATA_ADD" << _currentChannelAdd;
                    if(_state == STATE_MIX_CHANNEL_ADD) {
                        _currentChannelAdd++;
                        if(_currentChannelAdd < NUM_CHANNELS) {
                            _syncMixingDataAdd();
                            _timer.start(COMMAND_WAIT_INTERVAL);
                        } else {
                            _responseTryCount = 0;
                            _state = STATE_SEND_RX_INFO;
                            _sendRxResInfo();
                            _timer.start(COMMAND_WAIT_INTERVAL);
                        }
                    }
                    break;
                case Yuneec::CMD_SEND_RX_RESINFO:
                    //-- Response from _sendRxResInfo()
                    qCDebug(YuneecLog) << "Received TYPE_RSP: CMD_SEND_RX_RESINFO";
                    if(_state == STATE_SEND_RX_INFO) {
                        _state = STATE_ENTER_RUN;
                        _responseTryCount = 0;
                        _enterRun();
                        _timer.start(COMMAND_WAIT_INTERVAL);
                    }
                    break;
                case Yuneec::CMD_ENTER_RUN:
                    //-- Response from _enterRun()
                    qCDebug(YuneecLog) << "Received TYPE_RSP: CMD_ENTER_RUN";
                    if(_state == STATE_ENTER_RUN) {
                        _state = STATE_RUNNING;
                        _timer.stop();
                        qCDebug(YuneecLog) << "M4 ready, in run state.";
                    }
                    break;
                case Yuneec::CMD_SET_BINDKEY_FUNCTION:
                    qCDebug(YuneecLog) << "Received TYPE_RSP: CMD_SET_BINDKEY_FUNCTION";
                    break;
                default:
                    qCDebug(YuneecLog) << "Received TYPE_RSP: ???" << packet.commandID() << data.toHex();
                    break;
            }
            break;
        case Yuneec::TYPE_MISSION:
            qCDebug(YuneecLog) << "Received TYPE_MISSION (?)";
            break;
        default:
            qCDebug(YuneecLog) << "Received: Unknown Packet" << type << data.toHex();
            break;
    }
}

//-----------------------------------------------------------------------------
void
QGCCustom::_handleQueryBindResponse(QByteArray data)
{
    int nodeID = (data[10] & 0xff) | (data[11] << 8 & 0xff00);
    qCDebug(YuneecLog) << "Received TYPE_RSP: CMD_QUERY_BIND_STATE" << nodeID;
    if(_state == STATE_QUERY_BIND) {
        if(nodeID == _rxBindInfoFeedback.nodeId) {
            _timer.stop();
            qCDebug(YuneecLog) << "Switched to BOUND state with:" << _rxBindInfoFeedback.getName();
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
            qWarning() << "Response CMD_QUERY_BIND_STATE from unkown origin:" << nodeID;
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
    qCDebug(YuneecLog) << "Received TYPE_BIND: BIND Response";
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
    //-- TODO: If for some reason this is done where two or more Typhoons are in
    //   binding mode, we will be receiving multiple responses. No check for this
    //   situation is done below.
    /*
     * Based on original Java code as below:
     *
        private void handleRxBindInfo(byte[] data) {
            RxBindInfo rxBindInfoFeedback = new RxBindInfo();
            rxBindInfoFeedback.mode = (data[6] & 0xff) | (data[7] << 8 & 0xff00);
            rxBindInfoFeedback.panId = (data[8] & 0xff) | (data[9] << 8 & 0xff00);
            rxBindInfoFeedback.nodeId = (data[10] & 0xff) | (data[11] << 8 & 0xff00);
            rxBindInfoFeedback.aNum = data[20];
            rxBindInfoFeedback.aBit = data[21];
            rxBindInfoFeedback.swNum = data[24];
            rxBindInfoFeedback.swBit = data[25];
            int p = data.length - 2;
            rxBindInfoFeedback.txAddr = (data[p] & 0xff) | (data[p + 1] << 8 & 0xff00);//data[12]~data[19]
            ControllerStateManager manager = ControllerStateManager.getInstance();
            if (manager != null) {
                manager.onRecvBindInfo(rxBindInfoFeedback);
            }
        }
     *
     */
    qCDebug(YuneecLog) << "Received: TYPE_BIND with rxBindInfoFeedback";
    if(_state == STATE_START_BIND) {
        //qCDebug(YuneecLog) << dump_data_packet(packet.data);
        _timer.stop();
        _rxBindInfoFeedback.mode     = ((uint8_t)packet.data[6]  & 0xff) | ((uint8_t)packet.data[7]  << 8 & 0xff00);
        _rxBindInfoFeedback.panId    = ((uint8_t)packet.data[8]  & 0xff) | ((uint8_t)packet.data[9]  << 8 & 0xff00);
        _rxBindInfoFeedback.nodeId   = ((uint8_t)packet.data[10] & 0xff) | ((uint8_t)packet.data[11] << 8 & 0xff00);
        _rxBindInfoFeedback.aNum     = (uint8_t)packet.data[20];
        _rxBindInfoFeedback.aBit     = (uint8_t)packet.data[21];
        _rxBindInfoFeedback.swNum    = (uint8_t)packet.data[24];
        _rxBindInfoFeedback.swBit    = (uint8_t)packet.data[25];
        int p = packet.data.length() - 2;
        _rxBindInfoFeedback.txAddr   = ((uint8_t)packet.data[p] & 0xff) | ((uint8_t)packet.data[p + 1] << 8 & 0xff00);
        qCDebug(YuneecLog) << "RxBindInfo:" << _rxBindInfoFeedback.getName() << _rxBindInfoFeedback.nodeId;
        _state = STATE_UNBIND;
        _unbind();
        _timer.start(COMMAND_WAIT_INTERVAL);
    } else {
        qCDebug(YuneecLog) << "RxBindInfo discarded (out of sequence)";
    }
}

//-----------------------------------------------------------------------------
void
QGCCustom::_handleChannel(m4Packet& packet)
{
    Q_UNUSED(packet);
    switch(packet.commandID()) {
        case Yuneec::CMD_RX_FEEDBACK_DATA:
            qCDebug(YuneecLog) << "Received TYPE_CHN: CMD_RX_FEEDBACK_DATA";
            /* From original Java code
             *
             * We're not going to ever receive this unless the Typhoon is running
             * the factory firmware.
             *
            if (droneFeedbackListener == null) {
                return;
            }
            handleDroneFeedback(packet);
            */
            break;
        case Yuneec::CMD_TX_CHANNEL_DATA_MIXED:
            _handleMixedChannelData(packet);
            break;
        case Yuneec::CMD_TX_CHANNEL_DATA_RAW:
            //-- We don't yet use this
            break;
        case 0x82:
            //-- Not sure what this is
            break;
        default:
            qCDebug(YuneecLog) << "Received Unknown TYPE_CHN:" << packet.data.toHex();
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
                _m4State = (int)(commandValues[0] & 0x1f);
                emit _m4StateChanged(_m4State);
            }
            break;
        case Yuneec::CMD_TX_SWITCH_CHANGED:
            _switchChanged(packet);
            return true;
        default:
            qCDebug(YuneecLog) << "Received Unknown TYPE_CMD:" << packet.commandID() << packet.data.toHex();
            break;
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
    switchChanged.hwId      = (int)commandValues[0];
    switchChanged.oldState  = (int)commandValues[1];
    switchChanged.newState  = (int)commandValues[2];
    emit _switchStateChanged(switchChanged.hwId, switchChanged.oldState, switchChanged.newState);
}

//-----------------------------------------------------------------------------
void
QGCCustom::_handleMixedChannelData(m4Packet& packet)
{
    int analogChannelCount = _rxBindInfoFeedback.aNum  ? _rxBindInfoFeedback.aNum  : 10;
    int switchChannelCount = _rxBindInfoFeedback.swNum ? _rxBindInfoFeedback.swNum : 2;
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
    emit _channelDataStatus(channels);
}

//-----------------------------------------------------------------------------
void
QGCCustom::getControllerLocation(ControllerLocation& location)
{
    location = _controllerLocation;
}

//-----------------------------------------------------------------------------
void
QGCCustom::_handControllerFeedback(m4Packet& packet) {
    QByteArray commandValues = packet.commandValues();
    _controllerLocation.latitude     = byteArrayToInt(commandValues, 0) / 1e7;
    _controllerLocation.longitude    = byteArrayToInt(commandValues, 4) / 1e7;
    _controllerLocation.altitude     = byteArrayToFloat(commandValues, 8);
    _controllerLocation.accuracy     = byteArrayToShort(commandValues, 12);
    _controllerLocation.speed        = byteArrayToShort(commandValues, 14);
    _controllerLocation.angle        = byteArrayToShort(commandValues, 16);
    _controllerLocation.satelliteCount = commandValues[18] & 0x1f;
    emit _controllerLocationChanged();
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
