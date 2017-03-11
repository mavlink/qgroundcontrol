/*!
 * @file
 *   @brief ST16 QtQuick Interface
 *   @author Gus Grubba <mavlink@grubba.com>
 *
 */

#include "TyphoonHQuickInterface.h"
#include "TyphoonHM4Interface.h"

#if defined __android__
#include <jni.h>
#include <QtAndroidExtras/QtAndroidExtras>
#include <QtAndroidExtras/QAndroidJniObject>
extern const char* jniClassName;
#endif

//-----------------------------------------------------------------------------
TyphoonHQuickInterface::TyphoonHQuickInterface(QObject* parent)
    : QObject(parent)
    , _pHandler(NULL)
    , _scanningWiFi(false)
    , _bindingWiFi(false)
{

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
        connect(_pHandler, &TyphoonHM4Interface::scanComplete,                 this, &TyphoonHQuickInterface::_scanComplete);
        connect(_pHandler, &TyphoonHM4Interface::authenticationError,          this, &TyphoonHQuickInterface::_authenticationError);
        connect(_pHandler, &TyphoonHM4Interface::wifiConnected,                this, &TyphoonHQuickInterface::_wifiConnected);
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
TyphoonHQuickInterface::startScan()
{
    _ssidList.clear();
    _scanningWiFi = true;
    emit scanningWiFiChanged();
    emit ssidListChanged();
#if defined __android__
    QAndroidJniEnvironment env;
    if (env->ExceptionCheck()) {
        env->ExceptionDescribe();
        env->ExceptionClear();
    }
    QAndroidJniObject::callStaticMethod<void>(jniClassName, "startWifiScan", "()V");
#else
    _newSSID(QString("Some SSID"));
    _newSSID(QString("Another SSID"));
    _newSSID(QString("Yet Another SSID"));
    _newSSID(QString("More SSID"));
    _newSSID(QString("CIA Headquarters"));
    _newSSID(QString("Trump Putin Direct"));
    _newSSID(QString("Short"));
    _newSSID(QString("A Whole Lot Longer and Useless"));
#endif
}

//-----------------------------------------------------------------------------
void
TyphoonHQuickInterface::bindWIFI(QString ssid, QString password)
{
    _ssidList.clear();
    emit ssidListChanged();
    _bindingWiFi = true;
    emit bindingWiFiChanged();
#if defined __android__
    QAndroidJniEnvironment env;
    if (env->ExceptionCheck()) {
        env->ExceptionDescribe();
        env->ExceptionClear();
    }
    QAndroidJniObject javaSSID = QAndroidJniObject::fromString(ssid);
    QAndroidJniObject javaPassword = QAndroidJniObject::fromString(password);
    QAndroidJniObject::callStaticMethod<void>(jniClassName, "bindSSID", "(Ljava/lang/String;Ljava/lang/String;)V", javaSSID.object<jstring>(), javaPassword.object<jstring>());
#else
    Q_UNUSED(ssid);
    Q_UNUSED(password);
#endif
}

//-----------------------------------------------------------------------------
bool
TyphoonHQuickInterface::isWIFIConnected()
{
    bool res = false;
#if defined __android__
    QAndroidJniEnvironment env;
    if (env->ExceptionCheck()) {
        env->ExceptionDescribe();
        env->ExceptionClear();
    }
    res = (bool)QAndroidJniObject::callStaticMethod<jboolean>(jniClassName, "isWIFIConnected", "()B");
#endif
    return res;
}

//-----------------------------------------------------------------------------
QString
TyphoonHQuickInterface::connectedSSID()
{
    QString ssid;
#if defined __android__
    QAndroidJniEnvironment env;
    if (env->ExceptionCheck()) {
        env->ExceptionDescribe();
        env->ExceptionClear();
    }
    QAndroidJniObject str = QAndroidJniObject::callStaticObjectMethod(jniClassName, "connectedSSID", "()Ljava/lang/String;");
    ssid = str.toString();
    if(ssid.startsWith("\"")) ssid.remove(0,1);
    if(ssid.endsWith("\""))   ssid.remove(ssid.size()-1,1);
#endif
    return ssid;
}

//-----------------------------------------------------------------------------
void
TyphoonHQuickInterface::_newSSID(QString ssid)
{
#if defined(QT_DEBUG)
    if(!_ssidList.contains(ssid)) {
        _ssidList << ssid;
        emit ssidListChanged();
    }
#else
    if(ssid.startsWith("CGO3P")) {
        if(!_ssidList.contains(ssid)) {
            _ssidList << ssid;
            emit ssidListChanged();
        }
    }
#endif
}

//-----------------------------------------------------------------------------
void
TyphoonHQuickInterface::_scanComplete()
{
    _scanningWiFi = false;
    emit scanningWiFiChanged();
}

//-----------------------------------------------------------------------------
void
TyphoonHQuickInterface::_authenticationError()
{
    _bindingWiFi = false;
    emit bindingWiFiChanged();
    emit connectedSSIDChanged();
    emit authenticationError();
}

//-----------------------------------------------------------------------------
void
TyphoonHQuickInterface::_wifiConnected()
{
    _bindingWiFi = false;
    emit bindingWiFiChanged();
    emit connectedSSIDChanged();
    emit wifiConnected();
}
