/*!
 * @file
 *   @brief ST16 QtQuick Interface
 *   @author Gus Grubba <mavlink@grubba.com>
 *
 */

#include "QGCApplication.h"

#include "TyphoonHQuickInterface.h"
#include "TyphoonHM4Interface.h"

#if defined __android__
#include <jni.h>
#include <QtAndroidExtras/QtAndroidExtras>
#include <QtAndroidExtras/QAndroidJniObject>
extern const char* jniClassName;
#endif

#if defined __android__
void
reset_jni()
{
    QAndroidJniEnvironment env;
    if (env->ExceptionCheck()) {
        env->ExceptionDescribe();
        env->ExceptionClear();
    }
}
#endif

//-----------------------------------------------------------------------------
TyphoonHQuickInterface::TyphoonHQuickInterface(QObject* parent)
    : QObject(parent)
    , _pHandler(NULL)
    , _scanEnabled(false)
    , _scanningWiFi(false)
    , _bindingWiFi(false)
{
    qCDebug(YuneecLog) << "TyphoonHQuickInterface Created";
}

//-----------------------------------------------------------------------------
TyphoonHQuickInterface::~TyphoonHQuickInterface()
{
    qCDebug(YuneecLog) << "TyphoonHQuickInterface Destroyed";
}

//-----------------------------------------------------------------------------
void
TyphoonHQuickInterface::init(TyphoonHM4Interface* pHandler)
{
    _pHandler = pHandler;
    if(_pHandler) {
        connect(_pHandler, &TyphoonHM4Interface::m4StateChanged,               this, &TyphoonHQuickInterface::_m4StateChanged);
        connect(_pHandler, &TyphoonHM4Interface::destroyed,                    this, &TyphoonHQuickInterface::_destroyed);
        connect(_pHandler, &TyphoonHM4Interface::controllerLocationChanged,    this, &TyphoonHQuickInterface::_controllerLocationChanged);
        connect(_pHandler, &TyphoonHM4Interface::newWifiSSID,                  this, &TyphoonHQuickInterface::_newSSID);
        connect(_pHandler, &TyphoonHM4Interface::newWifiRSSI,                  this, &TyphoonHQuickInterface::_newRSSI);
        connect(_pHandler, &TyphoonHM4Interface::scanComplete,                 this, &TyphoonHQuickInterface::_scanComplete);
        connect(_pHandler, &TyphoonHM4Interface::authenticationError,          this, &TyphoonHQuickInterface::_authenticationError);
        connect(_pHandler, &TyphoonHM4Interface::wifiConnected,                this, &TyphoonHQuickInterface::_wifiConnected);
        connect(_pHandler, &TyphoonHM4Interface::batteryUpdate,                this, &TyphoonHQuickInterface::_batteryUpdate);
        connect(_pHandler, &TyphoonHM4Interface::armedChanged,                 this, &TyphoonHQuickInterface::_armedChanged);
        connect(&_scanTimer, &QTimer::timeout, this, &TyphoonHQuickInterface::_scanWifi);
        connect(&_flightTimer, &QTimer::timeout, this, &TyphoonHQuickInterface::_flightUpdate);
        _flightTimer.setSingleShot(false);
    }
}

//-----------------------------------------------------------------------------
CameraControl*
TyphoonHQuickInterface::cameraControl()
{
    if(_pHandler) {
        return _pHandler->cameraControl();
    }
    return NULL;
}

//-----------------------------------------------------------------------------
void
TyphoonHQuickInterface::_m4StateChanged()
{
    emit m4StateChanged();
}

//-----------------------------------------------------------------------------
void
TyphoonHQuickInterface::_controllerLocationChanged()
{
    emit controllerLocationChanged();
}

//-----------------------------------------------------------------------------
void
TyphoonHQuickInterface::_destroyed()
{
    disconnect(_pHandler, &TyphoonHM4Interface::m4StateChanged,       this, &TyphoonHQuickInterface::_m4StateChanged);
    disconnect(_pHandler, &TyphoonHM4Interface::destroyed,            this, &TyphoonHQuickInterface::_destroyed);
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

//-----------------------------------------------------------------------------
double
TyphoonHQuickInterface::latitude()
{
    if(_pHandler) {
        return _pHandler->controllerLocation().latitude;
    }
    return 0.0;
}

//-----------------------------------------------------------------------------
double
TyphoonHQuickInterface::longitude()
{
    if(_pHandler) {
        return _pHandler->controllerLocation().longitude;
    }
    return 0.0;
}

//-----------------------------------------------------------------------------
double
TyphoonHQuickInterface::altitude()
{
    if(_pHandler) {
        return _pHandler->controllerLocation().altitude;
    }
    return 0.0;
}

//-----------------------------------------------------------------------------
QString
TyphoonHQuickInterface::m4StateStr()
{
    if(_pHandler) {
        _pHandler->m4StateStr();
    }
    return QString();
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
void
TyphoonHQuickInterface::startScan(int delay)
{
    _ssidList.clear();
    _scanEnabled  = true;
    emit ssidListChanged();
#if defined __android__
    if(delay) {
        QTimer::singleShot(delay, this, &TyphoonHQuickInterface::_scanWifi);
    } else {
        _scanWifi();
    }
#else
    Q_UNUSED(delay);
    _newSSID(QString("Some SSID"), 0);
    _newSSID(QString("Another SSID"), -10);
    _newSSID(QString("Yet Another SSID"), -20);
    _newSSID(QString("More SSID"), -30);
    _newSSID(QString("CIA Headquarters"), -40);
    _newSSID(QString("Trump Putin Direct"), -50);
    _newSSID(QString("Short"), -60);
    _newSSID(QString("A Whole Lot Longer and Useless"), -90);
#endif
}

//-----------------------------------------------------------------------------
void
TyphoonHQuickInterface::stopScan()
{
    _scanTimer.stop();
    _scanEnabled = false;
    _scanningWiFi = false;
    emit scanningWiFiChanged();
}

//-----------------------------------------------------------------------------
void
TyphoonHQuickInterface::_scanWifi()
{
#if defined __android__
    reset_jni();
    QAndroidJniObject::callStaticMethod<void>(jniClassName, "startWifiScan", "()V");
#endif
    _scanningWiFi = true;
    emit scanningWiFiChanged();
}

//-----------------------------------------------------------------------------
bool
TyphoonHQuickInterface::isWIFIConnected()
{
    bool res = false;
#if defined __android__
    reset_jni();
    res = (bool)QAndroidJniObject::callStaticMethod<jboolean>(jniClassName, "isWIFIConnected", "()B");
#endif
    return res;
}

//-----------------------------------------------------------------------------
void
TyphoonHQuickInterface::resetWifi()
{
#if defined __android__
    //-- Stop scanning and clear list
    stopScan();
    _ssidList.clear();
    emit ssidListChanged();
    //-- Reset all wifi configurations
    if(_pHandler) {
        _pHandler->resetBind();
    }
    reset_jni();
    QAndroidJniObject::callStaticMethod<void>(jniClassName, "resetWifi", "()V");
    emit connectedSSIDChanged();
    //-- Start scanning again in a bit
    startScan(1000);
#endif
}

//-----------------------------------------------------------------------------
void
TyphoonHQuickInterface::bindWIFI(QString ssid, QString password)
{
    stopScan();
    _ssidList.clear();
    emit ssidListChanged();
    _bindingWiFi = true;
    emit bindingWiFiChanged();
    _ssid = ssid;
    _password = password;
#if defined __android__
    if(_pHandler) {
        _pHandler->resetBind();
    }
    reset_jni();
    QAndroidJniObject::callStaticMethod<void>(jniClassName, "disconnectWifi", "()V");
    //-- There isn't currently a way to disconnect and remove a Vehicle from here.
    //   The Wifi disconnect above will cause the vehicle to disconnect on its own
    //   after a heartbeat timeout.
    QTimer::singleShot(5000, this, &TyphoonHQuickInterface::_delayedBind);
#else
    Q_UNUSED(ssid);
    Q_UNUSED(password);
#endif
}

//-----------------------------------------------------------------------------
void
TyphoonHQuickInterface::_delayedBind()
{
#if defined __android__
    reset_jni();
    QAndroidJniObject javaSSID = QAndroidJniObject::fromString(_ssid);
    QAndroidJniObject javaPassword = QAndroidJniObject::fromString(_password);
    QAndroidJniObject::callStaticMethod<void>(jniClassName, "bindSSID", "(Ljava/lang/String;Ljava/lang/String;)V", javaSSID.object<jstring>(), javaPassword.object<jstring>());
    QTimer::singleShot(15000, this, &TyphoonHQuickInterface::_bindTimeout);
#endif
}

//-----------------------------------------------------------------------------
int
TyphoonHQuickInterface::rssi()
{
    int res = 0;
#if defined __android__
    reset_jni();
    res = (int)QAndroidJniObject::callStaticMethod<jint>(jniClassName, "wifiRssi", "()I");
#endif
   return res;
}

//-----------------------------------------------------------------------------
qreal
TyphoonHQuickInterface::rcBattery()
{
    qreal res = 0.0;
#if defined __android__
    reset_jni();
    res = (qreal)QAndroidJniObject::callStaticMethod<jfloat>(jniClassName, "getBatteryLevel", "()F");
#endif
   return res;
}

//-----------------------------------------------------------------------------
QString
TyphoonHQuickInterface::connectedSSID()
{
    QString ssid;
#if defined __android__
    reset_jni();
    QAndroidJniObject str = QAndroidJniObject::callStaticObjectMethod(jniClassName, "connectedSSID", "()Ljava/lang/String;");
    ssid = str.toString();
    if(ssid.startsWith("\"")) ssid.remove(0,1);
    if(ssid.endsWith("\""))   ssid.remove(ssid.size()-1,1);
#else
    ssid = "CIA Headquarters";
#endif
    return ssid;
}

//-----------------------------------------------------------------------------
QString
TyphoonHQuickInterface::connectedCamera()
{
    QString ssid = connectedSSID();
    if(ssid.startsWith("CGO3P")) {
        return QString("CGO-3+");
    }
    if(ssid.startsWith("CGOET")) {
        return QString("CGO-ET");
    }
    if(ssid.startsWith("CGOPRO")) {
        return QString("CGO-PRO");
    }
    return QString();
}

//-----------------------------------------------------------------------------
void
TyphoonHQuickInterface::_newSSID(QString ssid, int rssi)
{
    Q_UNUSED(rssi)
#if 0 // defined(QT_DEBUG)
    if(_scanningWiFi && !_ssidList.contains(ssid)) {
        _ssidList << ssid;
        _ssidList.sort(Qt::CaseInsensitive);
        emit ssidListChanged();
    }
#else
    if(ssid.startsWith("CGO3P") || ssid.startsWith("CGOPRO") || ssid.startsWith("CGOET")) {
        if(!_ssidList.contains(ssid)) {
            _ssidList << ssid;
            emit ssidListChanged();
        }
    }
#endif
}

//-----------------------------------------------------------------------------
void
TyphoonHQuickInterface::_newRSSI()
{
    emit rssiChanged();
}

//-----------------------------------------------------------------------------
void
TyphoonHQuickInterface::_scanComplete()
{
    if(_scanEnabled) {
        _scanTimer.setSingleShot(true);
        _scanTimer.start(2000);
    }
    _scanningWiFi = false;
    emit scanningWiFiChanged();
}

//-----------------------------------------------------------------------------
void
TyphoonHQuickInterface::_authenticationError()
{
    qCDebug(YuneecLog) << "TyphoonHQuickInterface::_authenticationError()";
    _bindingWiFi = false;
    emit bindingWiFiChanged();
    emit connectedSSIDChanged();
    emit authenticationError();
    startScan();
}

//-----------------------------------------------------------------------------
void
TyphoonHQuickInterface::_wifiConnected()
{
    qCDebug(YuneecLog) << "TyphoonHQuickInterface::_wifiConnected()";
    _bindingWiFi = false;
    emit bindingWiFiChanged();
    emit connectedSSIDChanged();
    emit wifiConnected();
}

//-----------------------------------------------------------------------------
void
TyphoonHQuickInterface::_batteryUpdate()
{
    emit rcBatteryChanged();
}

//-----------------------------------------------------------------------------
void
TyphoonHQuickInterface::_bindTimeout()
{
    if(_bindingWiFi) {
        _bindingWiFi = false;
        emit bindingWiFiChanged();
        emit bindTimeout();
        startScan();
    }
}

//-----------------------------------------------------------------------------
void
TyphoonHQuickInterface::_armedChanged(bool armed)
{
    if(armed) {
        _flightTime.start();
        _flightTimer.start(500);
    } else {
        _flightTimer.stop();
    }
}

//-----------------------------------------------------------------------------
QString
TyphoonHQuickInterface::flightTime()
{
    return QTime(0, 0).addMSecs(_flightTime.elapsed()).toString("hh:mm:ss");
}

//-----------------------------------------------------------------------------
void
TyphoonHQuickInterface::_flightUpdate()
{
    emit flightTimeChanged();
}
