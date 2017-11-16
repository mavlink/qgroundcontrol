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


//-----------------------------------------------------------------------------
TyphoonHM4Interface::TyphoonHM4Interface(QObject* parent)
    : QThread(parent)
    , _vehicle(NULL)
    , _rcTime(0)
    , _threadShouldStop(false)
{
    pTyphoonHandler = this;
    _m4Lib = new M4Lib(this);
    _rcTimer.setSingleShot(true);
    connect(&_rcTimer, &QTimer::timeout, this, &TyphoonHM4Interface::_rcTimeout);

    connect(_m4Lib, &M4Lib::rcActiveChanged, this, &TyphoonHM4Interface::_rcActiveChanged);
    connect(_m4Lib, &M4Lib::switchStateChanged, this, &TyphoonHM4Interface::switchStateChanged);
    connect(_m4Lib, &M4Lib::calibrationCompleteChanged, this, &TyphoonHM4Interface::calibrationCompleteChanged);
    connect(_m4Lib, &M4Lib::calibrationStateChanged, this, &TyphoonHM4Interface::calibrationStateChanged);
    connect(_m4Lib, &M4Lib::rawChannelsChanged, this, &TyphoonHM4Interface::rawChannelsChanged);
    connect(_m4Lib, &M4Lib::controllerLocationChanged, this, &TyphoonHM4Interface::controllerLocationChanged);
    connect(_m4Lib, &M4Lib::m4StateChanged, this, &TyphoonHM4Interface::m4StateChanged);
    connect(_m4Lib, &M4Lib::saveSettings, this, &TyphoonHM4Interface::_saveSettings);
}

//-----------------------------------------------------------------------------
TyphoonHM4Interface::~TyphoonHM4Interface()
{
    _threadShouldStop = true;
    emit destroyed();
    if(_m4Lib) {
        _m4Lib->deinit();
        delete _m4Lib;
    }
    pTyphoonHandler = NULL;
}

//-----------------------------------------------------------------------------
void
TyphoonHM4Interface::init(bool skipConnections)
{
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
    _m4Lib->setPairCommandCallback(std::bind(&TyphoonHM4Interface::_sendMavlinkBindCommand, this));
    _m4Lib->setSettings(rxBindInfoFeedback);

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
        if (_m4Lib) {
            _m4Lib->tryRead();
        } else {
            QThread::msleep(10);
        }
    }
}

//-----------------------------------------------------------------------------
void
TyphoonHM4Interface::resetBind()
{
    _m4Lib->resetBind();
}

//-----------------------------------------------------------------------------
bool
TyphoonHM4Interface::rcActive()
{
    return _m4Lib->getRcActive();
}

//-----------------------------------------------------------------------------
bool
TyphoonHM4Interface::rcCalibrationComplete()
{
    return _m4Lib->getRcCalibrationComplete();
}

//-----------------------------------------------------------------------------
QList<uint16_t>
TyphoonHM4Interface::rawChannels()
{
    return QList<uint16_t>::fromVector(QVector<uint16_t>::fromStdVector(_m4Lib->getRawChannels()));
}

//-----------------------------------------------------------------------------
TyphoonHQuickInterface::M4State
TyphoonHM4Interface::m4State()
{
    return (TyphoonHQuickInterface::M4State)(int)(_m4Lib->getM4State());
}

//-----------------------------------------------------------------------------
const M4Lib::ControllerLocation&
TyphoonHM4Interface::controllerLocation()
{
    return _m4Lib->getControllerLocation();
}

//-----------------------------------------------------------------------------
QString
TyphoonHM4Interface::m4StateStr()
{
    return QString::fromStdString(_m4Lib->m4StateStr());
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
                _m4Lib->checkVehicleReady();
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
    _m4Lib->setRcActive(false);
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
            _m4Lib->setRcActive(true);
        } else {
            if(!_rcTimer.isActive() && !_m4Lib->getSoftReboot()) {
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
        _m4Lib->setVehicleConnected(true);
        connect(_vehicle, &Vehicle::mavlinkMessageReceived, this, &TyphoonHM4Interface::_mavlinkMessageReceived);
        //-- Set the "Big Red Button" to bind mode
        _m4Lib->setPowerKey(Yuneec::BIND_KEY_FUNCTION_BIND);
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
        _m4Lib->setVehicleConnected(false);
        _m4Lib->setRcActive(false);
        _m4Lib->setPowerKey(Yuneec::BIND_KEY_FUNCTION_BIND);
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
    _m4Lib->tryStartCalibration();
}

//-----------------------------------------------------------------------------
void
TyphoonHM4Interface::stopCalibration()
{
    _m4Lib->tryStopCalibration();
}

//-----------------------------------------------------------------------------
void
TyphoonHM4Interface::softReboot()
{
    _m4Lib->softReboot();
}

//-----------------------------------------------------------------------------
int
TyphoonHM4Interface::calChannel(int index)
{
    return _m4Lib->calChannel(index);
}

//-----------------------------------------------------------------------------
void
TyphoonHM4Interface::_rcActiveChanged()
{
    //-- It just finished binding. Set the timer and see if we are indeed bound.
    _rcTimer.start(1000);
    emit rcActiveChanged();
}

//-----------------------------------------------------------------------------
void
TyphoonHM4Interface::_saveSettings(const M4Lib::RxBindInfo& rxBindInfo)
{
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
}
