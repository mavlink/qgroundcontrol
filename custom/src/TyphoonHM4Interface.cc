/*!
 * @file
 *   @brief ST16 Controller
 *   @author Gus Grubba <mavlink@grubba.com>
 *
 */

#include "QGCApplication.h"

#include "TyphoonHPlugin.h"
#include "TyphoonHM4Interface.h"
#include "TyphoonHQuickInterface.h"

#include <QNetworkAccessManager>

#include <functional>

#include <math.h>

QGC_LOGGING_CATEGORY(YuneecLog, "YuneecLog")
QGC_LOGGING_CATEGORY(YuneecLogVerbose, "YuneecLogVerbose")

TyphoonHM4Interface* TyphoonHM4Interface::pTyphoonHandler = NULL;

#if defined(__androidx86__)
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
#endif


//-----------------------------------------------------------------------------
TyphoonHM4Interface::TyphoonHM4Interface(QObject* parent)
    : QThread(parent)
    , _vehicle(NULL)
    , _rcTime(0)
    , _threadShouldStop(false)
{
    pTyphoonHandler = this;
#if defined(__androidx86__)
    _m4Lib = new M4Lib(_m4LibTimer);
#endif
    _rcTimer.setSingleShot(true);
    connect(&_rcTimer, &QTimer::timeout, this, &TyphoonHM4Interface::_rcTimeout);

#if defined(__androidx86__)
    // These are needed in order for signals containing these types to be queueable in order to
    // switch threads.
    qRegisterMetaType<M4Lib::ButtonId>("M4Lib::ButtonId");
    qRegisterMetaType<M4Lib::ButtonState>("M4Lib::ButtonState");
    qRegisterMetaType<M4Lib::SwitchId>("M4Lib::SwitchId");
    qRegisterMetaType<M4Lib::SwitchState>("M4Lib::SwitchState");
    qRegisterMetaType<M4Lib::RxBindInfo>("M4Lib::RxBindInfo");

    // We need to wrap all callbacks with a slot/signal to get them back onto the main thread.
    connect(this, &TyphoonHM4Interface::sendMavlinkBindCommand, this, &TyphoonHM4Interface::_sendMavlinkBindCommand);
    _m4Lib->setPairCommandCallback([this]() {
        emit sendMavlinkBindCommand();
    });
    _m4Lib->setButtonStateChangedCallback([this](M4Lib::ButtonId buttonId, M4Lib::ButtonState buttonState) {
        qCDebug(YuneecLogVerbose) << "in buttonStateChanged";
        emit buttonStateChanged(buttonId, buttonState);
    });
    _m4Lib->setSwitchStateChangedCallback([this](M4Lib::SwitchId switchId, M4Lib::SwitchState switchState) {
        qCDebug(YuneecLogVerbose) << "in switchStateChanged";
        emit switchStateChanged(switchId, switchState);
    });
    connect(this, &TyphoonHM4Interface::rcActiveChanged, this, &TyphoonHM4Interface::_rcActiveChanged);
    _m4Lib->setRcActiveChangedCallback([this]() {
        emit rcActiveChanged();
    });
    _m4Lib->setCalibrationCompleteChangedCallback([this]() {
        emit calibrationCompleteChanged();
    });
    _m4Lib->setCalibrationStateChangedCallback([this]() {
        emit calibrationStateChanged();
    });
    _m4Lib->setRawChannelsChangedCallback([this]() {
        emit rawChannelsChanged();
    });
    _m4Lib->setControllerLocationChangedCallback([this]() {
        emit controllerLocationChanged();
    });
    _m4Lib->setM4StateChangedCallback([this]() {
        emit m4StateChanged();
    });
    connect(this, &TyphoonHM4Interface::saveSettings, this, &TyphoonHM4Interface::_saveSettings);
    _m4Lib->setSaveSettingsCallback([this](const M4Lib::RxBindInfo rxBindInfo) {
        emit saveSettings(rxBindInfo);
    });
#endif
}

//-----------------------------------------------------------------------------
TyphoonHM4Interface::~TyphoonHM4Interface()
{
    _threadShouldStop = true;
#if defined(__androidx86__)
    if(_m4Lib) {
        _m4Lib->deinit();
        delete _m4Lib;
    }
#endif
    pTyphoonHandler = NULL;
    emit destroyed();
}

//-----------------------------------------------------------------------------
void
TyphoonHM4Interface::init(bool skipConnections)
{
#if defined(__androidx86__)
    //-- Have we bound before?
    QSettings settings;
    settings.beginGroup(kRxInfoGroup);
    M4Lib::RxBindInfo rxBindInfoFeedback {};
    if(settings.contains(knodeId) && settings.contains(kaNum)) {
        rxBindInfoFeedback.mode        = settings.value(kmode,         0).toInt();
        rxBindInfoFeedback.panId       = settings.value(kpanId,        0).toInt();
        rxBindInfoFeedback.nodeId      = settings.value(knodeId,       0).toInt();
        rxBindInfoFeedback.aNum        = settings.value(kaNum,         0).toInt();
        rxBindInfoFeedback.aBit        = settings.value(kaBit,         0).toInt();
        rxBindInfoFeedback.trNum       = settings.value(ktrNum,        0).toInt();
        rxBindInfoFeedback.trBit       = settings.value(ktrBit,        0).toInt();
        rxBindInfoFeedback.swNum       = settings.value(kswNum,        0).toInt();
        rxBindInfoFeedback.swBit       = settings.value(kswBit,        0).toInt();
        rxBindInfoFeedback.monitNum    = settings.value(kmonitNum,     0).toInt();
        rxBindInfoFeedback.monitBit    = settings.value(kmonitBit,     0).toInt();
        rxBindInfoFeedback.extraNum    = settings.value(kextraNum,     0).toInt();
        rxBindInfoFeedback.extraBit    = settings.value(kextraBit,     0).toInt();
        QByteArray tempAchName         = settings.value(kacName,       QByteArray()).toByteArray();
        QByteArray tempTrName          = settings.value(ktrName,       QByteArray()).toByteArray();
        QByteArray tempSwName          = settings.value(kswName,       QByteArray()).toByteArray();
        QByteArray tempMonitName       = settings.value(kmonitName,    QByteArray()).toByteArray();
        QByteArray tempExtraName       = settings.value(kextraName,    QByteArray()).toByteArray();
        rxBindInfoFeedback.achName     = std::vector<uint8_t>(tempAchName.begin(),   tempAchName.end());
        rxBindInfoFeedback.trName      = std::vector<uint8_t>(tempTrName.begin(),    tempTrName.end());
        rxBindInfoFeedback.swName      = std::vector<uint8_t>(tempSwName.begin(),    tempSwName.end());
        rxBindInfoFeedback.monitName   = std::vector<uint8_t>(tempMonitName.begin(), tempMonitName.end());
        rxBindInfoFeedback.extraName   = std::vector<uint8_t>(tempExtraName.begin(), tempExtraName.end());
        rxBindInfoFeedback.txAddr      = settings.value(ktxAddr,       0).toInt();
    }
    settings.endGroup();
    qCDebug(YuneecLog) << "Init M4 Handler";
    _m4Lib->init();
    _m4Lib->setSettings(rxBindInfoFeedback);
#endif

    if(!skipConnections) {
        connect(qgcApp()->toolbox()->multiVehicleManager(), &MultiVehicleManager::vehicleAdded, this, &TyphoonHM4Interface::_vehicleAdded);
        connect(qgcApp()->toolbox()->multiVehicleManager(), &MultiVehicleManager::parameterReadyVehicleAvailableChanged, this, &TyphoonHM4Interface::_vehicleReady);
        connect(qgcApp()->toolbox()->multiVehicleManager(), &MultiVehicleManager::vehicleRemoved, this, &TyphoonHM4Interface::_vehicleRemoved);
    }
    // Start thread to read from serial.
    start();
}

void
TyphoonHM4Interface::run()
{
    while (!_threadShouldStop) {
#if defined(__androidx86__)
        if (_m4Lib) {
            _m4Lib->tryRead();
        } else {
#endif
            QThread::msleep(10);
#if defined(__androidx86__)
        }
#endif
    }
}

//-----------------------------------------------------------------------------
void
TyphoonHM4Interface::resetBind()
{
#if defined(__androidx86__)
    _m4Lib->resetBind();
#endif
}

//-----------------------------------------------------------------------------
bool
TyphoonHM4Interface::rcActive()
{
#if defined(__androidx86__)
    return _m4Lib->getRcActive();
#else
    return false;
#endif
}

//-----------------------------------------------------------------------------
bool
TyphoonHM4Interface::rcCalibrationComplete()
{
#if defined(__androidx86__)
    return _m4Lib->getRcCalibrationComplete();
#else
    return false;
#endif
}

//-----------------------------------------------------------------------------
QList<uint16_t>
TyphoonHM4Interface::rawChannels()
{
#if defined(__androidx86__)
    return QList<uint16_t>::fromVector(QVector<uint16_t>::fromStdVector(_m4Lib->getRawChannels()));
#else
    return QList<uint16_t>();
#endif
}

//-----------------------------------------------------------------------------
TyphoonHQuickInterface::M4State
TyphoonHM4Interface::m4State()
{
#if defined(__androidx86__)
    return (TyphoonHQuickInterface::M4State)(int)(_m4Lib->getM4State());
#else
    return TyphoonHQuickInterface::M4_STATE_NONE;
#endif
}

//-----------------------------------------------------------------------------
const M4Lib::ControllerLocation&
TyphoonHM4Interface::controllerLocation()
{
#if defined(__androidx86__)
    return _m4Lib->getControllerLocation();
#else
    static M4Lib::ControllerLocation empty {};
    return empty;
#endif
}

//-----------------------------------------------------------------------------
QString
TyphoonHM4Interface::m4StateStr()
{
#if defined(__androidx86__)
    return QString::fromStdString(_m4Lib->m4StateStr());
#else
    return QString();
#endif
}


//-----------------------------------------------------------------------------
void
TyphoonHM4Interface::_vehicleReady(bool ready)
{
    if(_vehicle) {
        if(ready) {
            //-- Update hobbs meter
            emit _vehicle->hobbsMeterChanged();
            //-- If for some reason vehicle is armed, do nothing
            if(_vehicle->armed()) {
                qCWarning(YuneecLog) << "Booted with an armed vehicle!";
            } else {
                qCDebug(YuneecLog) << "_vehicleReady( YES )";
                //-- If we have not received RC messages yet and the M4 is running, wait a bit longer
#if defined(__androidx86__)
                _m4Lib->checkVehicleReady();
#endif
            }
        } else {
            qCDebug(YuneecLog) << "_vehicleReady( NOT )";
        }
    }
}


//-----------------------------------------------------------------------------
void
TyphoonHM4Interface::_rcTimeout()
{
    qCDebug(YuneecLog) << "RC Timeout";
#if defined(__androidx86__)
    _m4Lib->setRcActive(false);
#endif
}

//-----------------------------------------------------------------------------
void
TyphoonHM4Interface::_mavlinkMessageReceived(const mavlink_message_t& message)
{
    if(message.msgid == MAVLINK_MSG_ID_RC_CHANNELS) {
        mavlink_rc_channels_t channels;
        mavlink_msg_rc_channels_decode(&message, &channels);
        //-- Check if boot time changed
        if(channels.time_boot_ms != _rcTime) {
            _rcTime = channels.time_boot_ms;
            _rcTimer.stop();
#if defined(__androidx86__)
            _m4Lib->setRcActive(true);
#endif
        } else {
            //-- The assumption here is that the SoftReboot happens quickly enough
            //   so we don't have to care.
            //   This check is removed because we like to avoid the dependency on the
            //   M4Lib internal softReboot state.
            if(!_rcTimer.isActive() /*&& !_m4Lib->getSoftReboot()*/) {
                //-- Wait a bit before assuming RC is lost
                _rcTimer.start(1000);
            }
        }
    }
}

//-----------------------------------------------------------------------------
void
TyphoonHM4Interface::_vehicleAdded(Vehicle* vehicle)
{
    if(!_vehicle) {
        _vehicle = vehicle;

#if defined(__androidx86__)
        _m4Lib->setVehicleConnected(true);
        connect(_vehicle, &Vehicle::mavlinkMessageReceived, this, &TyphoonHM4Interface::_mavlinkMessageReceived);
        //-- Set the "Big Red Button" to bind mode
        _m4Lib->setPowerKey(Yuneec::BIND_KEY_FUNCTION_BIND);
#endif
    }
}

//-----------------------------------------------------------------------------
void
TyphoonHM4Interface::_vehicleRemoved(Vehicle* vehicle)
{
    if(_vehicle == vehicle) {
        qCDebug(YuneecLog) << "_vehicleRemoved()";
        disconnect(_vehicle, &Vehicle::mavlinkMessageReceived,  this, &TyphoonHM4Interface::_mavlinkMessageReceived);
        _vehicle = NULL;

#if defined(__androidx86__)
        _m4Lib->setVehicleConnected(false);
        _m4Lib->setRcActive(false);
        _m4Lib->setPowerKey(Yuneec::BIND_KEY_FUNCTION_BIND);
#endif
    }
}

//-----------------------------------------------------------------------------
void
TyphoonHM4Interface::enterBindMode(bool skipPairCommand)
{
#if defined(__androidx86__)
    _m4Lib->enterBindMode(skipPairCommand);
#else
    Q_UNUSED(skipPairCommand);
#endif
}

void
TyphoonHM4Interface::_sendMavlinkBindCommand()
{
    if (_vehicle) {
        //-- Send MAVLink command telling vehicle to enter bind mode
        qCDebug(YuneecLog) << "send pairRX mavlink command()";
        _vehicle->sendMavCommand(
            _vehicle->defaultComponentId(),         // target component
            MAV_CMD_START_RX_PAIR,                  // command id
            true,                                   // showError
            1,
            0);
    }
}

//-----------------------------------------------------------------------------
void
TyphoonHM4Interface::startCalibration()
{
    if(_vehicle && _vehicle->armed()) {
        qCWarning(YuneecLog) << "Cannot start calibration while armed.";
        return;
    }

#if defined(__androidx86__)
    _m4Lib->tryStartCalibration();
#endif
}

//-----------------------------------------------------------------------------
void
TyphoonHM4Interface::stopCalibration()
{
#if defined(__androidx86__)
    _m4Lib->tryStopCalibration();
#endif
}

//-----------------------------------------------------------------------------
void
TyphoonHM4Interface::softReboot()
{
#if defined(__androidx86__)
    _m4Lib->softReboot();
#endif
}

//-----------------------------------------------------------------------------
int
TyphoonHM4Interface::calChannel(int index)
{
#if defined(__androidx86__)
    return _m4Lib->calChannel(index);
#else
    Q_UNUSED(index)
    return 0;
#endif
}

//-----------------------------------------------------------------------------
void
TyphoonHM4Interface::_rcActiveChanged()
{
    //-- It just finished binding. Set the timer and see if we are indeed bound.
    _rcTimer.start(1000);
}

//-----------------------------------------------------------------------------
void
TyphoonHM4Interface::_saveSettings(const M4Lib::RxBindInfo& rxBindInfo)
{
#if defined(__androidx86__)
    //-- Store RX Info
    QSettings settings;
    settings.beginGroup(kRxInfoGroup);
    settings.setValue(kmode,        rxBindInfo.mode);
    settings.setValue(kpanId,       rxBindInfo.panId);
    settings.setValue(knodeId,      rxBindInfo.nodeId);
    settings.setValue(kaNum,        rxBindInfo.aNum);
    settings.setValue(kaBit,        rxBindInfo.aBit);
    settings.setValue(ktrNum,       rxBindInfo.trNum);  //add parameter
    settings.setValue(ktrBit,       rxBindInfo.trBit);
    settings.setValue(kswNum,       rxBindInfo.swNum);
    settings.setValue(kswBit,       rxBindInfo.swBit);
    settings.setValue(kmonitNum,    rxBindInfo.monitNum);
    settings.setValue(kmonitBit,    rxBindInfo.monitBit);
    settings.setValue(kextraNum,    rxBindInfo.extraNum);
    settings.setValue(kextraBit,    rxBindInfo.extraBit);
    settings.setValue(kacName,      QByteArray(reinterpret_cast<const char*>(rxBindInfo.achName.data())));
    settings.setValue(ktrName,      QByteArray(reinterpret_cast<const char*>(rxBindInfo.trName.data())));
    settings.setValue(kswName,      QByteArray(reinterpret_cast<const char*>(rxBindInfo.swName.data())));
    settings.setValue(kmonitName,   QByteArray(reinterpret_cast<const char*>(rxBindInfo.monitName.data())));
    settings.setValue(kextraName,   QByteArray(reinterpret_cast<const char*>(rxBindInfo.extraName.data())));
    settings.setValue(ktxAddr,      rxBindInfo.txAddr);
    settings.endGroup();
#else
    Q_UNUSED(rxBindInfo);
#endif
}
