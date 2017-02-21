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

//-- I don't like having to include these two. There should be more abstraction so we
//   we don't need this deep knowledge.
#include "QGCApplication.h"

#include "typhoonh.h"
#include "m4.h"
#include "m4serial.h"
#include <QDebug>
#include <QSettings>
#include <math.h>

static const char* kUartName        = "/dev/ttyMFD0";

static const char* kRxInfoGroup     = "YuneecM4RxInfo";
static const char* kmode            = "mode";
static const char* kpanId           = "panId";
static const char* knodeId          = "nodeId";
static const char* kaNum            = "aNum";
static const char* kaBit            = "aBit";
static const char* ktrNum           = "trNum";
static const char* ktrBit           = "trBit";
static const char* kswNum           = "swNum";
static const char* kswBit           = "swBit";
static const char* kmonitNum        = "monitNum";
static const char* kmonitBit        = "monitBit";
static const char* kextraNum        = "extraNum";
static const char* kextraBit        = "extraBit";
static const char* ktxAddr          = "txAddr";

static const char* kacName          = "acName";
static const char* ktrName          = "trName";
static const char* kswName          = "swName";
static const char* kmonitName       = "monitName";
static const char* kextraName       = "extraName";

#define SEND_INTERVAL               60
#define COMMAND_RESPONSE_TRIES      4
#define COMMAND_WAIT_INTERVAL       250
#define DEBUG_DATA_DUMP             false

//-----------------------------------------------------------------------------
// RC Channel data provided by Yuneec
#include "m4channeldata.h"

//-----------------------------------------------------------------------------
TyphoonHQuickInterface::TyphoonHQuickInterface(QObject* parent)
    : QObject(parent)
    , _pHandler(NULL)
{

}

//-----------------------------------------------------------------------------
void
TyphoonHQuickInterface::init(TyphoonM4Handler* pHandler)
{
    _pHandler = pHandler;
    if(_pHandler) {
        connect(_pHandler, &TyphoonM4Handler::m4StateChanged,       this, &TyphoonHQuickInterface::_m4StateChanged);
        connect(_pHandler, &TyphoonM4Handler::destroyed,            this, &TyphoonHQuickInterface::_destroyed);
        connect(_pHandler, &TyphoonM4Handler::cameraModeChanged,    this, &TyphoonHQuickInterface::_cameraModeChanged);
        connect(_pHandler, &TyphoonM4Handler::videoStatusChanged,   this, &TyphoonHQuickInterface::_videoStatusChanged);
        connect(&_videoRecordingTimer, &QTimer::timeout,            this, &TyphoonHQuickInterface::_videoRecordingUpdate);
        _videoRecordingTimer.setSingleShot(false);
    }
}

//-----------------------------------------------------------------------------
void
TyphoonHQuickInterface::_m4StateChanged()
{
    emit m4StateChanged();
}

//-----------------------------------------------------------------------------
void
TyphoonHQuickInterface::_cameraModeChanged()
{
    emit cameraModeChanged();
}

//-----------------------------------------------------------------------------
void
TyphoonHQuickInterface::_videoStatusChanged()
{
    //-- Recording time should come from camera. As there is no interface for
    //   it, we do it ourselves.
    if(videoStatus() == VIDEO_CAPTURE_STATUS_RUNNING) {
        _videoRecordingTimer.start(1000);
    } else {
        _videoRecordingTimer.stop();
    }
    emit videoStatusChanged();
}

//-----------------------------------------------------------------------------
void
TyphoonHQuickInterface::_videoRecordingUpdate()
{
    emit recordTimeChanged();
}

//-----------------------------------------------------------------------------
void
TyphoonHQuickInterface::_destroyed()
{
    disconnect(_pHandler, &TyphoonM4Handler::m4StateChanged,       this, &TyphoonHQuickInterface::_m4StateChanged);
    disconnect(_pHandler, &TyphoonM4Handler::destroyed,            this, &TyphoonHQuickInterface::_destroyed);
    disconnect(_pHandler, &TyphoonM4Handler::cameraModeChanged,    this, &TyphoonHQuickInterface::_cameraModeChanged);
    disconnect(_pHandler, &TyphoonM4Handler::videoStatusChanged,   this, &TyphoonHQuickInterface::_videoStatusChanged);
    _pHandler = NULL;
}

//-----------------------------------------------------------------------------
TyphoonHQuickInterface::M4State
TyphoonHQuickInterface::m4State()
{
    if(_pHandler) {
        return _pHandler->m4State();
    }
    return TyphoonHQuickInterface::M4_STATE_NONE;
}

TyphoonHQuickInterface::VideoStatus
TyphoonHQuickInterface::videoStatus()
{
    if(_pHandler) {
        return _pHandler->videoStatus();
    }
    return TyphoonHQuickInterface::VIDEO_CAPTURE_STATUS_UNDEFINED;
}

TyphoonHQuickInterface::CameraMode
TyphoonHQuickInterface::cameraMode()
{
    if(_pHandler) {
        return _pHandler->cameraMode();
    }
    return TyphoonHQuickInterface::CAMERA_MODE_UNDEFINED;
}

//-----------------------------------------------------------------------------
void
TyphoonHQuickInterface::setCameraMode(TyphoonHQuickInterface::CameraMode mode)
{
    Q_UNUSED(mode);
}

//-----------------------------------------------------------------------------
QString
TyphoonHQuickInterface::m4StateStr()
{
    switch(m4State()) {
        case TyphoonHQuickInterface::M4_STATE_NONE:
            return QString("Waiting for vehicle to connect...");
        case TyphoonHQuickInterface::M4_STATE_AWAIT:
            return QString("Waiting...");
        case TyphoonHQuickInterface::M4_STATE_BIND:
            return QString("Binding...");
        case TyphoonHQuickInterface::M4_STATE_CALIBRATION:
            return QString("Calibration...");
        case TyphoonHQuickInterface::M4_STATE_SETUP:
            return QString("Setup...");
        case TyphoonHQuickInterface::M4_STATE_RUN:
            return QString("Running...");
        case TyphoonHQuickInterface::M4_STATE_SIM:
            return QString("Simulation...");
        case TyphoonHQuickInterface::M4_STATE_FACTORY_CALI:
            return QString("Factory Calibration...");
        default:
            return QString("Unknown state...");
    }
    return QString();
}

//-----------------------------------------------------------------------------
QString
TyphoonHQuickInterface::recordTime()
{
    QString timeStr("00:00:00");
    if(_pHandler) {
        timeStr = _pHandler->recordTime().toString("hh:mm:ss");
    }
    return timeStr;
}

//-----------------------------------------------------------------------------
void
TyphoonHQuickInterface::initM4()
{
    if(_pHandler) {
        _pHandler->softReboot();
    }
}

//-----------------------------------------------------------------------------
void
TyphoonHQuickInterface::enterBindMode()
{
    if(_pHandler) {
        _pHandler->enterBindMode();
    }
}

//-----------------------------------------------------------------------------
#if 0
static QString
dump_data_packet(QByteArray data)
{
    QString resp;
    QString temp;
    for(int i = 0; i < data.size(); i++) {
        temp.sprintf(" %02X, ", (uint8_t)data[i]);
        resp += temp;
    }
    return resp;
}
#endif

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
TyphoonM4Handler::TyphoonM4Handler(QObject* parent)
    : QObject(parent)
    , _state(STATE_NONE)
    , _responseTryCount(0)
    , _currentChannelAdd(0)
    , _rxLocalIndex(0)
    , _sendRxInfoEnd(false)
    , _binding(false)
    , _vehicle(NULL)
    , _m4State(TyphoonHQuickInterface::M4_STATE_NONE)
    , _video_status(TyphoonHQuickInterface::VIDEO_CAPTURE_STATUS_UNDEFINED)
    , _video_resolution_h(0)
    , _video_resolution_v(0)
    , _video_framerate(0.0f)
    , _camera_mode(TyphoonHQuickInterface::CAMERA_MODE_UNDEFINED)
{
    _rxchannelInfoIndex = 2;
    _channelNumIndex    = 6;
    _commPort = new M4SerialComm(this);
    connect(&_timer, &QTimer::timeout, this, &TyphoonM4Handler::_stateManager);
    connect(&_videoTimer, &QTimer::timeout, this, &TyphoonM4Handler::_videoCaptureUpdate);
    _timer.setSingleShot(true);
    _videoTimer.setSingleShot(false);
}

//-----------------------------------------------------------------------------
TyphoonM4Handler::~TyphoonM4Handler()
{
    emit destroyed();
    if(_commPort) {
        delete _commPort;
    }
}

//-----------------------------------------------------------------------------
void
TyphoonM4Handler::init()
{
    //-- Have we bound before?
    QSettings settings;
    settings.beginGroup(kRxInfoGroup);
    if(settings.contains(knodeId) && settings.contains(kaNum)) {
        _rxBindInfoFeedback.mode        = settings.value(kmode,         0).toInt();
        _rxBindInfoFeedback.panId       = settings.value(kpanId,        0).toInt();
        _rxBindInfoFeedback.nodeId      = settings.value(knodeId,       0).toInt();
        _rxBindInfoFeedback.aNum        = settings.value(kaNum,         0).toInt();
        _rxBindInfoFeedback.aBit        = settings.value(kaBit,         0).toInt();
        _rxBindInfoFeedback.trNum       = settings.value(ktrNum,        0).toInt();
        _rxBindInfoFeedback.trBit       = settings.value(ktrBit,        0).toInt();
        _rxBindInfoFeedback.swNum       = settings.value(kswNum,        0).toInt();
        _rxBindInfoFeedback.swBit       = settings.value(kswBit,        0).toInt();
        _rxBindInfoFeedback.monitNum    = settings.value(kmonitNum,     0).toInt();
        _rxBindInfoFeedback.monitBit    = settings.value(kmonitBit,     0).toInt();
        _rxBindInfoFeedback.extraNum    = settings.value(kextraNum,     0).toInt();
        _rxBindInfoFeedback.extraBit    = settings.value(kextraBit,     0).toInt();
        _rxBindInfoFeedback.achName     = settings.value(kacName,       QByteArray()).toByteArray();
        _rxBindInfoFeedback.trName      = settings.value(ktrName,       QByteArray()).toByteArray();
        _rxBindInfoFeedback.swName      = settings.value(kswName,       QByteArray()).toByteArray();
        _rxBindInfoFeedback.monitName   = settings.value(kmonitName,    QByteArray()).toByteArray();
        _rxBindInfoFeedback.extraName   = settings.value(kextraName,    QByteArray()).toByteArray();
        _rxBindInfoFeedback.txAddr      = settings.value(ktxAddr,       0).toInt();
    }
    settings.endGroup();
    qDebug() << "Init M4 Handler";
    if(!_commPort || !_commPort->init(kUartName, 230400) || !_commPort->open()) {
        qWarning() << "Could not start serial communication with M4";
        return;
    }
    _sendRxInfoEnd = false;
    connect(_commPort, &M4SerialComm::bytesReady, this, &TyphoonM4Handler::_bytesReady);
    connect(qgcApp()->toolbox()->multiVehicleManager(), &MultiVehicleManager::vehicleAdded, this, &TyphoonM4Handler::_vehicleAdded);
    connect(qgcApp()->toolbox()->multiVehicleManager(), &MultiVehicleManager::vehicleRemoved, this, &TyphoonM4Handler::_vehicleRemoved);
}

//-----------------------------------------------------------------------------
void
TyphoonM4Handler::_vehicleAdded(Vehicle* vehicle)
{
    if(!_vehicle) {
        _vehicle = vehicle;
        connect(_vehicle, &Vehicle::mavlinkMessageReceived, this, &TyphoonM4Handler::_mavlinkMessageReceived);
        _videoTimer.start(1000);
        _requestCameraSettings();
    }
}

//-----------------------------------------------------------------------------
void
TyphoonM4Handler::_vehicleRemoved(Vehicle* vehicle)
{
    if(_vehicle == vehicle) {
        disconnect(_vehicle, &Vehicle::mavlinkMessageReceived, this, &TyphoonM4Handler::_mavlinkMessageReceived);
        _videoTimer.stop();
        _vehicle = NULL;
    }
}

//-----------------------------------------------------------------------------
void
TyphoonM4Handler::_mavlinkMessageReceived(const mavlink_message_t& message)
{
    switch (message.msgid) {
        case MAVLINK_MSG_ID_CAMERA_CAPTURE_STATUS:
            _handleCaptureStatus(message);
            break;
        case MAVLINK_MSG_ID_CAMERA_SETTINGS:
            _handleCameraSettings(message);
            break;
    }
}

//-----------------------------------------------------------------------------
void
TyphoonM4Handler::_handleCaptureStatus(const mavlink_message_t &message)
{
    mavlink_camera_capture_status_t cap;
    mavlink_msg_camera_capture_status_decode(&message, &cap);
    TyphoonHQuickInterface::VideoStatus oldStatus = _video_status;
    _video_status       = cap.video_status == 0 ? TyphoonHQuickInterface::VIDEO_CAPTURE_STATUS_STOPPED : TyphoonHQuickInterface::VIDEO_CAPTURE_STATUS_RUNNING;
    _video_resolution_h = cap.video_resolution_h;
    _video_resolution_v = cap.video_resolution_v;
    _video_framerate    = cap.video_framerate;
    if(oldStatus != _video_status) {
        emit videoStatusChanged();
    }
    qDebug() << "Capture Status" << _video_status << _video_resolution_h << _video_resolution_v << _video_framerate;
}

//-----------------------------------------------------------------------------
void
TyphoonM4Handler::_handleCameraSettings(const mavlink_message_t &message)
{
    mavlink_camera_settings_t settings;
    mavlink_msg_camera_settings_decode(&message, &settings);
    switch(settings.mode_id) {
    case 2:
        _camera_mode = TyphoonHQuickInterface::CAMERA_MODE_PHOTO;
        break;
    case 1:
        _camera_mode = TyphoonHQuickInterface::CAMERA_MODE_VIDEO;
        break;
    default:
        _camera_mode = TyphoonHQuickInterface::CAMERA_MODE_UNDEFINED;
        break;
    }
    emit cameraModeChanged();
    qDebug() << "Camera settings" << _camera_mode;
}

//-----------------------------------------------------------------------------
void
TyphoonM4Handler::_videoCaptureUpdate()
{
    _requestCaptureStatus();
}

//-----------------------------------------------------------------------------
void
TyphoonM4Handler::_requestCameraSettings()
{
    if(_vehicle) {
        qDebug() << "Request Camera Settings";
        _vehicle->sendMavCommand(
            _vehicle->defaultComponentId(),         // target component
            MAV_CMD_REQUEST_CAMERA_SETTINGS,        // command id
            true,                                   // showError
            1,
            0);
    }
}

//-----------------------------------------------------------------------------
void
TyphoonM4Handler::_requestCaptureStatus()
{
    if(_vehicle) {
        qDebug() << "Request Capture Status";
        _vehicle->sendMavCommand(
            _vehicle->defaultComponentId(),         // target component
            MAV_CMD_REQUEST_CAMERA_CAPTURE_STATUS,  // command id
            true,                                   // showError
            1,
            0);
    }
}

//-----------------------------------------------------------------------------
void
TyphoonM4Handler::enterBindMode()
{
    qDebug() << "enterBindMode()";
    //-- Send MAVLink command telling vehicle to enter bind mode
    if(_vehicle) {
        qDebug() << "pairRX()";
        _binding = true;
        _vehicle->sendMavCommand(
            _vehicle->defaultComponentId(),    // target component
            MAV_CMD_START_RX_PAIR,             // command id
            true,                              // showError
            1,
            0);
        //-- Set M4 into bind mode
        _rxBindInfoFeedback.clear();
        if(_m4State == TyphoonHQuickInterface::M4_STATE_BIND) {
            _exitBind();
        } else if(_m4State == TyphoonHQuickInterface::M4_STATE_RUN) {
            _exitRun();
        }
        QTimer::singleShot(1000, this, &TyphoonM4Handler::_initSequence);
    }
}

//-----------------------------------------------------------------------------
void
TyphoonM4Handler::softReboot()
{
    qDebug() << "softReboot()";
    _timer.stop();
    if(_commPort) {
        disconnect(_commPort, &M4SerialComm::bytesReady, this, &TyphoonM4Handler::_bytesReady);
        delete _commPort;
    }
    _state              = STATE_NONE;
    _responseTryCount   = 0;
    _currentChannelAdd  = 0;
    _m4State            = TyphoonHQuickInterface::M4_STATE_NONE;
    _rxLocalIndex       = 0;
    _sendRxInfoEnd      = false;
    _rxchannelInfoIndex = 2;
    _channelNumIndex    = 6;
    QThread::msleep(SEND_INTERVAL);
    _commPort = new M4SerialComm(this);
    init();
}

//-----------------------------------------------------------------------------
void
TyphoonM4Handler::takePhoto()
{
    qDebug() << "takePhoto()";
    //-- Send MAVLink command telling vehicle to take photo
    if(_vehicle) {
        _vehicle->sendMavCommand(
            _vehicle->defaultComponentId(),     // target component
            MAV_CMD_IMAGE_START_CAPTURE,        // command id
            true,                               // showError
            0,                                  // Duration between two consecutive pictures (in seconds)
            1,                                  // Number of images to capture total - 0 for unlimited capture
            -1);                                // Resolution in megapixels (max)
    }
}

//-----------------------------------------------------------------------------
void
TyphoonM4Handler::toggleVideo()
{
    qDebug() << "toggleVideo()";
    if(_video_status == TyphoonHQuickInterface::VIDEO_CAPTURE_STATUS_UNDEFINED) {
        //-- We don't know what the current status is. We have to wait for it before we can do anything
        return;
    }
    if (_vehicle) {
        if(_video_status == TyphoonHQuickInterface::VIDEO_CAPTURE_STATUS_STOPPED) {
            //-- State is now undefined until we get confirmation
            _video_status = TyphoonHQuickInterface::VIDEO_CAPTURE_STATUS_UNDEFINED;
            _vehicle->sendMavCommand(
                _vehicle->defaultComponentId(), // target component
                MAV_CMD_VIDEO_START_CAPTURE,    // command id
                true,                           // showError
                0,                              // Camera ID (0 for all cameras), 1 for first, 2 for second, etc.
                -1,                             // Frames per second (max)
                -1);                            // Resolution (max)
            _recordTime.start();
        } else {
            _vehicle->sendMavCommand(
                _vehicle->defaultComponentId(), // target component
                MAV_CMD_VIDEO_STOP_CAPTURE ,    // command id
                true,                           // showError
                0);                             // Camera ID (0 for all cameras), 1 for first, 2 for second, etc.
        }
    }
}

//-----------------------------------------------------------------------------
void
TyphoonM4Handler::_initSequence()
{
    //-- There are two defines for the argument to this function. One sets the
    //   button to turn the screen on/off (BIND_KEY_FUNCTION_PWR). The other is
    //   anybody's guess (BIND_KEY_FUNCTION_BIND).
    _setPowerKey(Yuneec::BIND_KEY_FUNCTION_BIND);
    QThread::msleep(SEND_INTERVAL);
    _responseTryCount = 0;
    //-- Check and see if we have binding info
    if(_rxBindInfoFeedback.nodeId) {
        qDebug() << "Previously bound with:" << _rxBindInfoFeedback.nodeId << "(" << _rxBindInfoFeedback.aNum << "Analog Channels ) (" << _rxBindInfoFeedback.swNum << "Switches )";
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
TyphoonM4Handler::_stateManager()
{
    switch(_state) {
        case STATE_EXIT_RUN:
            qDebug() << "STATE_EXIT_RUN Timeout";
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
            qDebug() << "STATE_ENTER_BIND Timeout";
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
            qDebug() << "STATE_START_BIND Timeout";
            if(_responseTryCount > COMMAND_RESPONSE_TRIES) {
                qWarning() << "Too many STATE_START_BIND Timeouts. Giving up...";
                _state = STATE_EXIT_BIND;
                _exitBind();
                _timer.start(COMMAND_WAIT_INTERVAL);
                _responseTryCount = 0;
            } else {
                _startBind();
                //-- Wait a bit longer as there may not be anyone listening
                _timer.start(1000);
                _responseTryCount++;
            }
            break;
        case STATE_UNBIND:
            qDebug() << "STATE_UNBIND Timeout";
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
            qDebug() << "STATE_BIND Timeout";
            _bind(_rxBindInfoFeedback.nodeId);
            _timer.start(COMMAND_WAIT_INTERVAL);
            //-- TODO: This can't wait for ever...
            break;
        case STATE_QUERY_BIND:
            qDebug() << "STATE_QUERY_BIND Timeout";
            _queryBindState();
            _timer.start(COMMAND_WAIT_INTERVAL);
            //-- TODO: This can't wait for ever...
            break;
        case STATE_EXIT_BIND:
            qDebug() << "STATE_EXIT_BIND Timeout";
            _exitBind();
            _timer.start(COMMAND_WAIT_INTERVAL);
            //-- TODO: This can't wait for ever...
            break;
        case STATE_RECV_BOTH_CH:
            if(_responseTryCount > COMMAND_RESPONSE_TRIES) {
                qWarning() << "Too many STATE_RECV_BOTH_CH Timeouts. Giving up...";
            } else {
                qDebug() << "STATE_RECV_BOTH_CH Timeout";
                _sendRecvBothCh();
                _timer.start(COMMAND_WAIT_INTERVAL);
                _responseTryCount++;
            }
            break;
        case STATE_SET_CHANNEL_SETTINGS:
            qDebug() << "STATE_SET_CHANNEL_SETTINGS Timeout";
            _setChannelSetting();
            _timer.start(COMMAND_WAIT_INTERVAL);
            //-- TODO: This can't wait for ever...
            break;
        case STATE_MIX_CHANNEL_DELETE:
            qDebug() << "STATE_MIX_CHANNEL_DELETE Timeout";
            _syncMixingDataDeleteAll();
            _timer.start(COMMAND_WAIT_INTERVAL);
            //-- TODO: This can't wait for ever...
            break;
        case STATE_MIX_CHANNEL_ADD:
            qDebug() << "STATE_MIX_CHANNEL_ADD Timeout";
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
                qDebug() << "STATE_SEND_RX_INFO Timeout";
                _sendRxResInfo();
                _timer.start(COMMAND_WAIT_INTERVAL);
                _responseTryCount++;
            }
            break;
        case STATE_ENTER_RUN:
            if(_responseTryCount > COMMAND_RESPONSE_TRIES) {
                qWarning() << "Too many STATE_ENTER_RUN Timeouts. Giving up...";
            } else {
                qDebug() << "STATE_ENTER_RUN Timeout";
                _enterRun();
                _timer.start(COMMAND_WAIT_INTERVAL);
                _responseTryCount++;
            }
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
TyphoonM4Handler::_enterRun()
{
    qDebug() << "Sending: CMD_ENTER_RUN";
    m4Command enterRunCmd(Yuneec::CMD_ENTER_RUN);
    QByteArray cmd = enterRunCmd.pack();
    return _commPort->write(cmd, DEBUG_DATA_DUMP);
}

//-----------------------------------------------------------------------------
/**
 * This command is used for stopping control aircraft.
 */
bool
TyphoonM4Handler::_exitRun()
{
    qDebug() << "Sending: CMD_EXIT_RUN";
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
TyphoonM4Handler::_enterBind()
{
    qDebug() << "Sending: CMD_ENTER_BIND";
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
TyphoonM4Handler::_sendRecvBothCh()
{
    qDebug() << "Sending: CMD_RECV_BOTH_CH";
    m4Command enterRecvCmd(Yuneec::CMD_RECV_BOTH_CH);
    QByteArray cmd = enterRecvCmd.pack();
    return _commPort->write(cmd, DEBUG_DATA_DUMP);
}

//-----------------------------------------------------------------------------
/**
 * This command is used for exiting the progress of binding.
 */
bool
TyphoonM4Handler::_exitBind()
{
    qDebug() << "Sending: CMD_EXIT_BIND";
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
TyphoonM4Handler::_startBind()
{
    qDebug() << "Sending: CMD_START_BIND";
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
TyphoonM4Handler::_bind(int rxAddr)
{
    qDebug() << "Sending: CMD_BIND";
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
TyphoonM4Handler::_setChannelSetting()
{
    qDebug() << "Sending: CMD_SET_CHANNEL_SETTING";
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
TyphoonM4Handler::_setPowerKey(int function)
{
    qDebug() << "Sending: CMD_SET_BINDKEY_FUNCTION";
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
TyphoonM4Handler::_unbind()
{
    qDebug() << "Sending: CMD_UNBIND";
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
TyphoonM4Handler::_queryBindState()
{
    qDebug() << "Sending: CMD_QUERY_BIND_STATE";
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
TyphoonM4Handler::_syncMixingDataDeleteAll()
{
    qDebug() << "Sending: CMD_SYNC_MIXING_DATA_DELETE_ALL";
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
TyphoonM4Handler::_syncMixingDataAdd()
{
    /*
     *  "Mixing Data" is an array of NUM_CHANNELS (25) sets of CHANNEL_LENGTH (96)
     *  magic bytes each. Each set is sent using this command. The documentation states
     *  that if there is an error you should send the CMD_SYNC_MIXING_DATA_DELETE_ALL
     *  command again and start over.
     *  I have not seen a way to identify an error other than getting no response once
     *  the command is sent. There doesn't appear to be a "NAK" type response.
     */
    qDebug() << "Sending: CMD_SYNC_MIXING_DATA_ADD";
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
TyphoonM4Handler::_sendRxResInfo()
{
    _sendRxInfoEnd = false;
    TableDeviceChannelInfo_t channelInfo ;
    memset(&channelInfo, 0, sizeof(TableDeviceChannelInfo_t));
    TableDeviceLocalInfo_t localInfo;
    memset(&localInfo, 0, sizeof(TableDeviceLocalInfo_t));
    if(!_generateTableDeviceChannelInfo(&channelInfo)) {
        return _sendRxInfoEnd;
    }
    if(!_sendTableDeviceChannelInfo(channelInfo)) {
        return _sendRxInfoEnd;
    }
    QThread::msleep(SEND_INTERVAL);
    _generateTableDeviceLocalInfo(&localInfo);
    if(!_sendTableDeviceLocalInfo(localInfo)) {
        return _sendRxInfoEnd;
    }
    _sendRxInfoEnd = true;
    return _sendRxInfoEnd;
}

//-----------------------------------------------------------------------------
/**
 * This funtion is used for sending the Local information to aircraft
 * Local information such as structure TableDeviceLocalInfo_t
 */
bool
TyphoonM4Handler::_sendTableDeviceLocalInfo(TableDeviceLocalInfo_t localInfo)
{
    m4Command sendRxResInfoCmd(Yuneec::CMD_SEND_RX_RESINFO);
    QByteArray payload;
    int len = 11;
    payload.fill(0, len);
    payload[0]  = localInfo.index;
    payload[1]  = (uint8_t)(localInfo.mode     & 0xff);
    payload[2]  = (uint8_t)((localInfo.mode    & 0xff00) >> 8);
    payload[3]  = (uint8_t)(localInfo.nodeId   & 0xff);
    payload[4]  = (uint8_t)((localInfo.nodeId  & 0xff00) >> 8);
    payload[5]  = localInfo.parseIndex;
    payload[7]  = (uint8_t)(localInfo.panId    & 0xff);
    payload[8]  = (uint8_t)((localInfo.panId   & 0xff00) >> 8);
    payload[9]  = (uint8_t)(localInfo.txAddr   & 0xff);
    payload[10] = (uint8_t)((localInfo.txAddr  & 0xff00) >> 8);
    QByteArray cmd = sendRxResInfoCmd.pack(payload);
    //qDebug() << "_sendTableDeviceLocalInfo" <<dump_data_packet(cmd);
    return _commPort->write(cmd, DEBUG_DATA_DUMP);
}

//-----------------------------------------------------------------------------
/**
 * This funtion is used for sending the Channel information to aircraft
 * Channel information such as structure TableDeviceChannelInfo_t
 */
bool
TyphoonM4Handler::_sendTableDeviceChannelInfo(TableDeviceChannelInfo_t channelInfo)
{
    m4Command sendRxResInfoCmd(Yuneec::CMD_SEND_RX_RESINFO);
    QByteArray payload;
    int len = sizeof(channelInfo);
    payload.fill(0, len);
    payload[0]  = channelInfo.index;
    payload[1]  = channelInfo.aNum;
    payload[2]  = channelInfo.aBits;
    payload[3]  = channelInfo.trNum;
    payload[4]  = channelInfo.trBits;
    payload[5]  = channelInfo.swNum;
    payload[6]  = channelInfo.swBits;
    payload[7]  = channelInfo.replyChannelNum;
    payload[8]  = channelInfo.replyChannelBits;
    payload[9]  = channelInfo.requestChannelNum;
    payload[10] = channelInfo.requestChannelBits;
    payload[11] = channelInfo.extraNum;
    payload[12] = channelInfo.extraBits;
    payload[13] = channelInfo.analogType;
    payload[14] = channelInfo.trimType;
    payload[15] = channelInfo.switchType;
    payload[16] = channelInfo.replyChannelType;
    payload[17] = channelInfo.requestChannelType;
    payload[18] = channelInfo.extraType;
    QByteArray cmd = sendRxResInfoCmd.pack(payload);
    //qDebug() << "_sendTableDeviceChannelInfo" <<dump_data_packet(cmd);
    return _commPort->write(cmd, DEBUG_DATA_DUMP);
}

//-----------------------------------------------------------------------------
/**
 * This funtion is used for filling TableDeviceLocalInfo_t with the Local information
 * information from RxBindInfo
 */
void
TyphoonM4Handler::_generateTableDeviceLocalInfo(TableDeviceLocalInfo_t* localInfo)
{
    localInfo->index        = _rxLocalIndex;
    localInfo->mode         = _rxBindInfoFeedback.mode;
    localInfo->nodeId       = _rxBindInfoFeedback.nodeId;
    localInfo->parseIndex   = _rxchannelInfoIndex - 1;
    localInfo->panId        = _rxBindInfoFeedback.panId;
    localInfo->txAddr       = _rxBindInfoFeedback.txAddr;
    _rxLocalIndex++;
}

//-----------------------------------------------------------------------------
/**
 * This funtion is used for filling TableDeviceChannelInfo_t with the channel information
 * information from RxBindInfo
 */
bool
TyphoonM4Handler::_generateTableDeviceChannelInfo(TableDeviceChannelInfo_t* channelInfo)
{
    channelInfo->index              = _rxchannelInfoIndex;
    channelInfo->aNum               = _rxBindInfoFeedback.aNum;
    channelInfo->aBits              = _rxBindInfoFeedback.aBit;
    channelInfo->trNum              = _rxBindInfoFeedback.trNum;
    channelInfo->trBits             = _rxBindInfoFeedback.trBit;
    channelInfo->swNum              = _rxBindInfoFeedback.swNum;
    channelInfo->swBits             = _rxBindInfoFeedback.swBit;
    channelInfo->replyChannelNum    = _rxBindInfoFeedback.monitNum;
    channelInfo->replyChannelBits   = _rxBindInfoFeedback.monitBit;
    channelInfo->requestChannelNum  = _rxBindInfoFeedback.monitNum;
    channelInfo->requestChannelBits = _rxBindInfoFeedback.monitBit;
    channelInfo->extraNum           = _rxBindInfoFeedback.extraNum;
    channelInfo->extraBits          = _rxBindInfoFeedback.extraBit;
    if(!_sendTableDeviceChannelNumInfo(ChannelNumAanlog)) {
        return false;
    }
    QThread::msleep(SEND_INTERVAL);
    channelInfo->analogType = _channelNumIndex - 1;
    if(!_sendTableDeviceChannelNumInfo(ChannelNumTrim)) {
        return false;
    }
    QThread::msleep(SEND_INTERVAL);
    channelInfo->trimType = _channelNumIndex - 1;
    if(!_sendTableDeviceChannelNumInfo(ChannelNumSwitch)) {
        return false;
    }
    QThread::msleep(SEND_INTERVAL);
    channelInfo->switchType = _channelNumIndex - 1;
    // generate reply channel map
    if(!_sendTableDeviceChannelNumInfo(ChannelNumMonitor)) {
        return false;
    }
    QThread::msleep(SEND_INTERVAL);
    channelInfo->replyChannelType   = _channelNumIndex - 1;
    channelInfo->requestChannelType = _channelNumIndex - 1;
    // generate extra channel map
    if(!_sendTableDeviceChannelNumInfo(ChannelNumExtra)) {
        return false;
    }
    channelInfo->extraType = _channelNumIndex - 1;
    QThread::msleep(SEND_INTERVAL);
    _rxchannelInfoIndex++;
    return true;
}

//-----------------------------------------------------------------------------
/**
 * This funtion is used for sending the Channel number information to aircraft
 * Channel information such as structure TableDeviceChannelNumInfo_t
 * This feature is distributed according to enum ChannelNumType_t
 */
bool
TyphoonM4Handler::_sendTableDeviceChannelNumInfo(ChannelNumType_t channelNumTpye)
{
    TableDeviceChannelNumInfo_t channelNumInfo;
    memset(&channelNumInfo, 0, sizeof(TableDeviceChannelNumInfo_t));
    int num =  0;
    if(_generateTableDeviceChannelNumInfo(&channelNumInfo, channelNumTpye, num)) {
        m4Command sendRxResInfoCmd(Yuneec::CMD_SEND_RX_RESINFO);
        QByteArray payload;
        int len = num + 1;
        payload.fill(0, len);
        payload[0] = channelNumInfo.index;
        for(int i = 0; i < num; i++) {
            payload[i + 1] = channelNumInfo.channelMap[i];
        }
        QByteArray cmd = sendRxResInfoCmd.pack(payload);
        //qDebug() << channelNumTpye <<dump_data_packet(cmd);
        return _commPort->write(cmd, DEBUG_DATA_DUMP);
    }
    return true;
}

//-----------------------------------------------------------------------------
/**
 * This feature is based on different types of fill information
 *
 */
bool
TyphoonM4Handler::_generateTableDeviceChannelNumInfo(TableDeviceChannelNumInfo_t* channelNumInfo, ChannelNumType_t channelNumTpye, int& num)
{
    switch(channelNumTpye) {
        case ChannelNumAanlog:
            num = _rxBindInfoFeedback.aNum;
            if(!_fillTableDeviceChannelNumMap(channelNumInfo, num, _rxBindInfoFeedback.achName)) {
                return false;
            }
            break;
        case ChannelNumTrim:
            num = _rxBindInfoFeedback.trNum;
            if(!_fillTableDeviceChannelNumMap(channelNumInfo, num, _rxBindInfoFeedback.trName)) {
                return false;
            }
            break;
        case ChannelNumSwitch:
            num = _rxBindInfoFeedback.swNum;
            if(!_fillTableDeviceChannelNumMap(channelNumInfo, num, _rxBindInfoFeedback.swName)) {
                return false;
            }
            break;
        case ChannelNumMonitor:
            num = _rxBindInfoFeedback.monitNum;
            if(num <= 0) {
                return false;
            }
            if(!_fillTableDeviceChannelNumMap(channelNumInfo, num, _rxBindInfoFeedback.monitName)) {
                return false;
            }
            break;
        case ChannelNumExtra:
            num = _rxBindInfoFeedback.extraNum;
            if(num <= 0) {
                return false;
            }
            if(!_fillTableDeviceChannelNumMap(channelNumInfo, num, _rxBindInfoFeedback.extraName)) {
                return false;
            }
            break;
        default:
            return false;
    }
    return true;
}

//-----------------------------------------------------------------------------
/**
 * This funtion is used for filling TableDeviceChannelNumInfo_t with the channel number information
 * information from RxBindInfo
 */
bool
TyphoonM4Handler::_fillTableDeviceChannelNumMap(TableDeviceChannelNumInfo_t* channelNumInfo, int num, QByteArray list)
{
    bool res = false;
    if(num) {
        if(num <= (int)list.count()) {
            channelNumInfo->index = _channelNumIndex;
            for(int i = 0; i < num; i++) {
                channelNumInfo->channelMap[i] = (uint8_t)list[i];
            }
            res = true;
        } else {
            qCritical() << "_fillTableDeviceChannelNumMap() called with mismatching list size. Num =" << num << "List =" << list.count();
        }
    }
    _channelNumIndex++;
    return res;
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
TyphoonM4Handler::_bytesReady(QByteArray data)
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
                    qDebug() << "Received: TYPE_BIND Unknown:" << data.toHex();
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
                    qDebug() << "Received TYPE_RSP: CMD_EXIT_RUN";
                    if(_state == STATE_EXIT_RUN) {
                        //-- Now we start initsequence
                        _initSequence();
                    }
                    break;
                case Yuneec::CMD_ENTER_BIND:
                    //-- Response from _enterBind()
                    qDebug() << "Received TYPE_RSP: CMD_ENTER_BIND";
                    if(_state == STATE_ENTER_BIND) {
                        //-- Now we start scanning
                        _responseTryCount = 0;
                        _state = STATE_START_BIND;
                        _startBind();
                        _timer.start(COMMAND_WAIT_INTERVAL);
                    }
                    break;
                case Yuneec::CMD_UNBIND:
                    qDebug() << "Received TYPE_RSP: CMD_UNBIND";
                    if(_state == STATE_UNBIND) {
                        _state = STATE_BIND;
                        _bind(_rxBindInfoFeedback.nodeId);
                        _timer.start(COMMAND_WAIT_INTERVAL);
                    }
                    break;
                case Yuneec::CMD_EXIT_BIND:
                    //-- Response from _exitBind()
                    qDebug() << "Received TYPE_RSP: CMD_EXIT_BIND";
                    if(_state == STATE_EXIT_BIND) {
                        _responseTryCount = 0;
                        _state = STATE_RECV_BOTH_CH;
                        _sendRecvBothCh();
                        _timer.start(COMMAND_WAIT_INTERVAL);
                    }
                    break;
                case Yuneec::CMD_RECV_BOTH_CH:
                    //-- Response from _sendRecvBothCh()
                    qDebug() << "Received TYPE_RSP: CMD_RECV_BOTH_CH";
                    if(_state == STATE_RECV_BOTH_CH) {
                        _state = STATE_SET_CHANNEL_SETTINGS;
                        _setChannelSetting();
                        _timer.start(COMMAND_WAIT_INTERVAL);
                    }
                    break;
                case Yuneec::CMD_SET_CHANNEL_SETTING:
                    //-- Response from _setChannelSetting()
                    qDebug() << "Received TYPE_RSP: CMD_SET_CHANNEL_SETTING";
                    if(_state == STATE_SET_CHANNEL_SETTINGS) {
                        _state = STATE_MIX_CHANNEL_DELETE;
                        _syncMixingDataDeleteAll();
                        _timer.start(COMMAND_WAIT_INTERVAL);
                    }
                    break;
                case Yuneec::CMD_SYNC_MIXING_DATA_DELETE_ALL:
                    //-- Response from _syncMixingDataDeleteAll()
                    qDebug() << "Received TYPE_RSP: CMD_SYNC_MIXING_DATA_DELETE_ALL";
                    if(_state == STATE_MIX_CHANNEL_DELETE) {
                        _state = STATE_MIX_CHANNEL_ADD;
                        _currentChannelAdd = 0;
                        _syncMixingDataAdd();
                        _timer.start(COMMAND_WAIT_INTERVAL);
                    }
                    break;
                case Yuneec::CMD_SYNC_MIXING_DATA_ADD:
                    //-- Response from _syncMixingDataAdd()
                    qDebug() << "Received TYPE_RSP: CMD_SYNC_MIXING_DATA_ADD" << _currentChannelAdd;
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
                    if(_state == STATE_SEND_RX_INFO && _sendRxInfoEnd) {
                        qDebug() << "Received TYPE_RSP: CMD_SEND_RX_RESINFO";
                        _state = STATE_ENTER_RUN;
                        _responseTryCount = 0;
                        _enterRun();
                        _timer.start(COMMAND_WAIT_INTERVAL);
                    }
                    break;
                case Yuneec::CMD_ENTER_RUN:
                    //-- Response from _enterRun()
                    qDebug() << "Received TYPE_RSP: CMD_ENTER_RUN";
                    if(_state == STATE_ENTER_RUN) {
                        _state = STATE_RUNNING;
                        _timer.stop();
                        if(_binding) {
                            _binding = false;
                            qDebug() << "Soft reboot...";
                            QTimer::singleShot(1000, this, &TyphoonM4Handler::softReboot);
                        } else {
                            qDebug() << "M4 ready, in run state.";
                        }
                    }
                    break;
                case Yuneec::CMD_SET_BINDKEY_FUNCTION:
                    qDebug() << "Received TYPE_RSP: CMD_SET_BINDKEY_FUNCTION";
                    break;
                default:
                    qDebug() << "Received TYPE_RSP: ???" << packet.commandID() << data.toHex();
                    break;
            }
            break;
        case Yuneec::TYPE_MISSION:
            qDebug() << "Received TYPE_MISSION (?)";
            break;
        default:
            qDebug() << "Received: Unknown Packet" << type << data.toHex();
            break;
    }
}

//-----------------------------------------------------------------------------
void
TyphoonM4Handler::_handleQueryBindResponse(QByteArray data)
{
    int nodeID = (data[10] & 0xff) | (data[11] << 8 & 0xff00);
    qDebug() << "Received TYPE_RSP: CMD_QUERY_BIND_STATE" << nodeID;
    if(_state == STATE_QUERY_BIND) {
        if(nodeID == _rxBindInfoFeedback.nodeId) {
            _timer.stop();
            qDebug() << "Switched to BOUND state with:" << _rxBindInfoFeedback.getName();
            _state = STATE_EXIT_BIND;
            _exitBind();
            //-- Store RX Info
            QSettings settings;
            settings.beginGroup(kRxInfoGroup);
            settings.setValue(kmode,        _rxBindInfoFeedback.mode);
            settings.setValue(kpanId,       _rxBindInfoFeedback.panId);
            settings.setValue(knodeId,      _rxBindInfoFeedback.nodeId);
            settings.setValue(kaNum,        _rxBindInfoFeedback.aNum);
            settings.setValue(kaBit,        _rxBindInfoFeedback.aBit);
            settings.setValue(ktrNum,       _rxBindInfoFeedback.trNum);  //add parameter
            settings.setValue(ktrBit,       _rxBindInfoFeedback.trBit);
            settings.setValue(kswNum,       _rxBindInfoFeedback.swNum);
            settings.setValue(kswBit,       _rxBindInfoFeedback.swBit);
            settings.setValue(kmonitNum,    _rxBindInfoFeedback.monitNum);
            settings.setValue(kmonitBit,    _rxBindInfoFeedback.monitBit);
            settings.setValue(kextraNum,    _rxBindInfoFeedback.extraNum);
            settings.setValue(kextraBit,    _rxBindInfoFeedback.extraBit);
            settings.setValue(kacName,      _rxBindInfoFeedback.achName);
            settings.setValue(ktrName,      _rxBindInfoFeedback.trName);
            settings.setValue(kswName,      _rxBindInfoFeedback.swName);
            settings.setValue(kmonitName,   _rxBindInfoFeedback.monitName);
            settings.setValue(kextraName,   _rxBindInfoFeedback.extraName);
            settings.setValue(ktxAddr,      _rxBindInfoFeedback.txAddr);
            settings.endGroup();
            _timer.start(COMMAND_WAIT_INTERVAL);
        } else {
            qWarning() << "Response CMD_QUERY_BIND_STATE from unkown origin:" << nodeID;
        }
    }
}

//-----------------------------------------------------------------------------
bool
TyphoonM4Handler::_handleNonTypePacket(m4Packet& packet)
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
TyphoonM4Handler::_handleBindResponse()
{
    qDebug() << "Received TYPE_BIND: BIND Response";
    if(_state == STATE_BIND) {
        _timer.stop();
        _state = STATE_QUERY_BIND;
        _queryBindState();
        _timer.start(COMMAND_WAIT_INTERVAL);
    }
}

//-----------------------------------------------------------------------------
void
TyphoonM4Handler::_handleRxBindInfo(m4Packet& packet)
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
    qDebug() << "Received: TYPE_BIND with rxBindInfoFeedback";
    if(_state == STATE_START_BIND) {
        //qDebug() << dump_data_packet(packet.data);
        _timer.stop();
        _rxBindInfoFeedback.mode     = ((uint8_t)packet.data[6]  & 0xff) | ((uint8_t)packet.data[7]  << 8 & 0xff00);
        _rxBindInfoFeedback.panId    = ((uint8_t)packet.data[8]  & 0xff) | ((uint8_t)packet.data[9]  << 8 & 0xff00);
        _rxBindInfoFeedback.nodeId   = ((uint8_t)packet.data[10] & 0xff) | ((uint8_t)packet.data[11] << 8 & 0xff00);
        _rxBindInfoFeedback.aNum     = (uint8_t)packet.data[20];
        _rxBindInfoFeedback.aBit     = (uint8_t)packet.data[21];
        _rxBindInfoFeedback.trNum    = (uint8_t)packet.data[22];
        _rxBindInfoFeedback.trBit    = (uint8_t)packet.data[23];
        _rxBindInfoFeedback.swNum    = (uint8_t)packet.data[24];
        _rxBindInfoFeedback.swBit    = (uint8_t)packet.data[25];
        _rxBindInfoFeedback.monitNum = (uint8_t)packet.data[26];
        _rxBindInfoFeedback.monitBit = (uint8_t)packet.data[27];
        _rxBindInfoFeedback.extraNum = (uint8_t)packet.data[28];
        _rxBindInfoFeedback.extraBit = (uint8_t)packet.data[29];
        int ilen = 30;
        _rxBindInfoFeedback.achName.clear();
        for(int i = 0; i < _rxBindInfoFeedback.aNum ; i++) {
            _rxBindInfoFeedback.achName.append((uint8_t)packet.data[ilen++]);
        }
        _rxBindInfoFeedback.trName.clear();
        for(int i = 0; i < _rxBindInfoFeedback.trNum ; i++) {
            _rxBindInfoFeedback.trName.append((uint8_t)packet.data[ilen++]);
        }
        _rxBindInfoFeedback.swName.clear();
        for(int i = 0; i < _rxBindInfoFeedback.swNum ; i++) {
            _rxBindInfoFeedback.swName.append((uint8_t)packet.data[ilen++]);
        }
        _rxBindInfoFeedback.monitName.clear();
        for(int i = 0; i < _rxBindInfoFeedback.monitNum ; i++) {
            _rxBindInfoFeedback.monitName.append((uint8_t)packet.data[ilen++]);
        }
        _rxBindInfoFeedback.extraName.clear();
        for(int i = 0; i < _rxBindInfoFeedback.extraNum ; i++) {
            _rxBindInfoFeedback.extraName.append((uint8_t)packet.data[ilen++]);
        }
        int p = packet.data.length() - 2;
        _rxBindInfoFeedback.txAddr = ((uint8_t)packet.data[p] & 0xff) | ((uint8_t)packet.data[p + 1] << 8 & 0xff00);
        qDebug() << "RxBindInfo:" << _rxBindInfoFeedback.getName() << _rxBindInfoFeedback.nodeId;
        _state = STATE_UNBIND;
        _unbind();
        _timer.start(COMMAND_WAIT_INTERVAL);
    } else {
        qDebug() << "RxBindInfo discarded (out of sequence)";
    }
}

//-----------------------------------------------------------------------------
void
TyphoonM4Handler::_handleChannel(m4Packet& packet)
{
    Q_UNUSED(packet);
    switch(packet.commandID()) {
        case Yuneec::CMD_RX_FEEDBACK_DATA:
            qDebug() << "Received TYPE_CHN: CMD_RX_FEEDBACK_DATA";
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
            qDebug() << "Received Unknown TYPE_CHN:" << packet.data.toHex();
            break;
    }
}

//-----------------------------------------------------------------------------
void
TyphoonM4Handler::_handleInitialState()
{
    qDebug() << "Initial state:" << _m4State;
    if(_m4State == TyphoonHQuickInterface::M4_STATE_BIND) {
        //-- TX is not bound. Not much we can do.
        _exitBind();
        _state = STATE_EXIT_BIND;
        QThread::msleep(SEND_INTERVAL);
        _timer.start(COMMAND_WAIT_INTERVAL);
    } else if(_m4State == TyphoonHQuickInterface::M4_STATE_AWAIT) {
        //-- TX seems to be bound. Just start it all.
        _initSequence();
    } else if(_m4State == TyphoonHQuickInterface::M4_STATE_RUN) {
        //-- TX seems to be bound and running. Restart.
        _exitRun();
        QThread::msleep(150);
        _initSequence();
    } else {
        //-- Anyting else we don't have much to do
        qDebug() << "Idle state";
    }
}

//-----------------------------------------------------------------------------
bool
TyphoonM4Handler::_handleCommand(m4Packet& packet)
{
    Q_UNUSED(packet);
    switch(packet.commandID()) {
        case Yuneec::CMD_TX_STATE_MACHINE: {
                QByteArray commandValues = packet.commandValues();
                TyphoonHQuickInterface::M4State state = (TyphoonHQuickInterface::M4State)(commandValues[0] & 0x1f);
                if(state != _m4State) {
                    TyphoonHQuickInterface::M4State oldState = _m4State;
                    _m4State = state;
                    if(oldState == TyphoonHQuickInterface::M4_STATE_NONE) {
                        _handleInitialState();
                    }
                    emit m4StateChanged();
                    qDebug() << "New State:" << _m4State;
                }
            }
            break;
        case Yuneec::CMD_TX_SWITCH_CHANGED:
            _switchChanged(packet);
            return true;
        default:
            qDebug() << "Received Unknown TYPE_CMD:" << packet.commandID() << packet.data.toHex();
            break;
    }
    return false;
}

//-----------------------------------------------------------------------------
void
TyphoonM4Handler::_switchChanged(m4Packet& packet)
{
    Q_UNUSED(packet);
    QByteArray commandValues = packet.commandValues();
    SwitchChanged switchChanged;
    switchChanged.hwId      = (int)commandValues[0];
    switchChanged.oldState  = (int)commandValues[1];
    switchChanged.newState  = (int)commandValues[2];
    emit switchStateChanged(switchChanged.hwId, switchChanged.oldState, switchChanged.newState);
    qDebug() << "Switches:" << switchChanged.hwId << switchChanged.oldState << switchChanged.newState;
    //-- On Button Down
    if(switchChanged.newState == 1) {
        switch(switchChanged.hwId) {
            case 53: // Camera Shutter
                takePhoto();
                break;
            case 54: // Video Button
                toggleVideo();
                break;
            default:
                break;
        }
    }
}

//-----------------------------------------------------------------------------
void
TyphoonM4Handler::_handleMixedChannelData(m4Packet& packet)
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
    emit channelDataStatus(channels);
}

//-----------------------------------------------------------------------------
void
TyphoonM4Handler::getControllerLocation(ControllerLocation& location)
{
    location = _controllerLocation;
}

//-----------------------------------------------------------------------------
void
TyphoonM4Handler::_handControllerFeedback(m4Packet& packet)
{
    QByteArray commandValues = packet.commandValues();
    _controllerLocation.latitude     = byteArrayToInt(commandValues, 0) / 1e7;
    _controllerLocation.longitude    = byteArrayToInt(commandValues, 4) / 1e7;
    _controllerLocation.altitude     = byteArrayToFloat(commandValues, 8);
    _controllerLocation.accuracy     = byteArrayToShort(commandValues, 12);
    _controllerLocation.speed        = byteArrayToShort(commandValues, 14);
    _controllerLocation.angle        = byteArrayToShort(commandValues, 16);
    _controllerLocation.satelliteCount = commandValues[18] & 0x1f;
    emit controllerLocationChanged();
}

//-----------------------------------------------------------------------------
int
TyphoonM4Handler::byteArrayToInt(QByteArray data, int offset, bool isBigEndian)
{
    int iRetVal = -1;
    if(data.size() < offset + 4) {
        return iRetVal;
    }
    int iLowest;
    int iLow;
    int iMid;
    int iHigh;
    if(isBigEndian) {
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
TyphoonM4Handler::byteArrayToFloat(QByteArray data, int offset)
{
    uint32_t val = (uint32_t)byteArrayToInt(data, offset);
    return (float)val;
}

//-----------------------------------------------------------------------------
short
TyphoonM4Handler::byteArrayToShort(QByteArray data, int offset, bool isBigEndian)
{
    short iRetVal = -1;
    if(data.size() < offset + 2) {
        return iRetVal;
    }
    int iLow;
    int iHigh;
    if(isBigEndian) {
        iLow    = data[offset + 1];
        iHigh   = data[offset + 0];
    } else {
        iLow    = data[offset + 0];
        iHigh   = data[offset + 1];
    }
    iRetVal = (iHigh << 8) | (0xFF & iLow);
    return iRetVal;
}
