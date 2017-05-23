/*!
 * @file
 *   @brief ST16 QtQuick Interface
 *   @author Gus Grubba <mavlink@grubba.com>
 *
 */

#include "QGCApplication.h"
#include "AppSettings.h"
#include "SettingsManager.h"
#include "MAVLinkLogManager.h"

#include <QDirIterator>
#include <QtAlgorithms>

#include "TyphoonHQuickInterface.h"
#include "TyphoonHM4Interface.h"

#if defined __android__
#include <jni.h>
#include <QtAndroidExtras/QtAndroidExtras>
#include <QtAndroidExtras/QAndroidJniObject>
extern const char* jniClassName;
#endif

static const char* kWifiConfig = "WifiConfig";

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
    , _copyingFiles(false)
    , _wifiAlertEnabled(true)
    , _copyResult(0)
{
    qCDebug(YuneecLog) << "TyphoonHQuickInterface Created";
}

//-----------------------------------------------------------------------------
TyphoonHQuickInterface::~TyphoonHQuickInterface()
{
    qCDebug(YuneecLog) << "TyphoonHQuickInterface Destroyed";
    _clearSSids();
}

//-----------------------------------------------------------------------------
static bool
created_greater_than(const QFileInfo &f1, const QFileInfo &f2)
 {
     return f1.created() > f2.created();
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
        connect(_pHandler, &TyphoonHM4Interface::wifiDisconnected,             this, &TyphoonHQuickInterface::_wifiDisconnected);
        connect(_pHandler, &TyphoonHM4Interface::batteryUpdate,                this, &TyphoonHQuickInterface::_batteryUpdate);
        connect(_pHandler, &TyphoonHM4Interface::armedChanged,                 this, &TyphoonHQuickInterface::_armedChanged);
        connect(_pHandler, &TyphoonHM4Interface::rawChannelsChanged,           this, &TyphoonHQuickInterface::_rawChannelsChanged);
        connect(_pHandler, &TyphoonHM4Interface::switchStateChanged,           this, &TyphoonHQuickInterface::_switchStateChanged);
        connect(_pHandler, &TyphoonHM4Interface::calibrationStateChanged,      this, &TyphoonHQuickInterface::_calibrationStateChanged);
        connect(_pHandler, &TyphoonHM4Interface::calibrationCompleteChanged,   this, &TyphoonHQuickInterface::_calibrationCompleteChanged);
        connect(&_scanTimer,    &QTimer::timeout, this, &TyphoonHQuickInterface::_scanWifi);
        connect(&_flightTimer,  &QTimer::timeout, this, &TyphoonHQuickInterface::_flightUpdate);
        connect(&_powerTimer,   &QTimer::timeout, this, &TyphoonHQuickInterface::_powerTrigger);
        _flightTimer.setSingleShot(false);
        _powerTimer.setSingleShot(true);
        _loadWifiConfigurations();
        //-- Make sure uLog is disabled
        qgcApp()->toolbox()->mavlinkLogManager()->setEnableAutoUpload(false);
        qgcApp()->toolbox()->mavlinkLogManager()->setEnableAutoStart(false);
        //-- See how many logs we have stored
        QString filter = "*.";
        filter += qgcApp()->toolbox()->settingsManager()->appSettings()->telemetryFileExtension;
        QDir logDir(qgcApp()->toolbox()->settingsManager()->appSettings()->telemetrySavePath(), filter);
        QFileInfoList logs = logDir.entryInfoList();
        qSort(logs.begin(), logs.end(), created_greater_than);
        if(logs.size() > 1) {
            qint64 totalLogSize = 0;
            for(int i = 0; i < logs.size(); i++) {
               totalLogSize += logs[i].size();
            }
            //-- We want to limit at 1G
            while(totalLogSize > (1024 * 1024 * 1024) && logs.size()) {
                qDebug() << "Removing old log file:" << logs[0].fileName();
                totalLogSize -= logs[0].size();
                QFile::remove(logs[0].filePath());
                logs.removeAt(0);
            }
        }
    }
}

//-----------------------------------------------------------------------------
void
TyphoonHQuickInterface::_powerTrigger()
{
    //-- If RC is not working
    if(!_pHandler->rcActive()) {
        //-- Panic button held down
        emit powerHeld();
    }
}

//-----------------------------------------------------------------------------
void
TyphoonHQuickInterface::_switchStateChanged(int swId, int /*oldState*/, int newState)
{
    if(swId == Yuneec::BUTTON_POWER) {
        if(newState) {
            _powerTimer.start(1000);
        } else {
            _powerTimer.stop();
        }
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
double
TyphoonHQuickInterface::speed()
{
    if(_pHandler) {
        return _pHandler->controllerLocation().speed;
    }
    return 0.0;
}

//-----------------------------------------------------------------------------
double
TyphoonHQuickInterface::gpsCount()
{
    if(_pHandler) {
        return _pHandler->controllerLocation().satelliteCount;
    }
    return 0.0;
}

//-----------------------------------------------------------------------------
double
TyphoonHQuickInterface::gpsAccuracy()
{
    if(_pHandler) {
        return _pHandler->controllerLocation().accuracy;
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
    _clearSSids();
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
TyphoonHQuickInterface::connected()
{
    return rssi() < 0;
}

//-----------------------------------------------------------------------------
void
TyphoonHQuickInterface::resetWifi()
{
#if defined __android__
    //-- Stop scanning and clear list
    stopScan();
    _clearSSids();
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
bool
TyphoonHQuickInterface::isWifiConfigured(QString ssid)
{
    return _configurations.contains(ssid);
}

//-----------------------------------------------------------------------------
void
TyphoonHQuickInterface::calibrateGimbalMV()
{
    if(_pHandler && _pHandler->vehicle()) {
        _pHandler->vehicle()->sendMavCommand(0, MAV_CMD_PREFLIGHT_CALIBRATION, true, 0,0,0,0,0,0,4);
    }
}

//-----------------------------------------------------------------------------
void
TyphoonHQuickInterface::bindWIFI(QString ssid, QString password)
{
    stopScan();
    _clearSSids();
    emit ssidListChanged();
    _bindingWiFi = true;
    emit bindingWiFiChanged();
    _ssid = ssid;
    _password = password;
    if(password.isEmpty()) {
        if(_configurations.contains(ssid)) {
            _password = _configurations[ssid];
        }
    }
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
    if(ssid.startsWith("E90")) {
        return QString("E90");
    }
    if(ssid.startsWith("E50")) {
        return QString("E50");
    }
    return QString();
}

//-----------------------------------------------------------------------------
bool
TyphoonHQuickInterface::isTyphoon()
{
    QString ssid = connectedSSID();
    if(ssid.startsWith("CGO3P") || ssid.startsWith("CGOET") || ssid.startsWith("CGOPRO") ||
        ssid.startsWith("E90_") || ssid.startsWith("E50_")) {
        return true;
    }
    return false;
}

//-----------------------------------------------------------------------------
int
TyphoonHQuickInterface::rawChannel(int channel)
{
    if(_pHandler) {
        if(channel < _pHandler->rawChannels().size()) {
            uint16_t t = _pHandler->rawChannels()[channel];
            return t;
        }
    }
    return 0;
}

//-----------------------------------------------------------------------------
int
TyphoonHQuickInterface::calChannelState(int channel)
{
    if(_pHandler) {
        return _pHandler->calChannel(channel);
    }
    return 0;
}

//-----------------------------------------------------------------------------
bool
TyphoonHQuickInterface::calibrationComplete()
{
    if(_pHandler) {
        return _pHandler->rcCalibrationComplete();
    }
    return false;
}

//-----------------------------------------------------------------------------
int
TyphoonHQuickInterface::_copyFilesInPath(const QString src, const QString dst)
{
    int fileCount = 0;
    QDir dir(dst);
    if (!dir.exists()) {
        if(!dir.mkpath(".")) {
            return -1;
        }
    }
    QDirIterator it(src, QStringList() << "*", QDir::Files, QDirIterator::NoIteratorFlags);
    while(it.hasNext()) {
        QFileInfo fi(it.next());
        QString output = dst + "/" + fi.fileName();
        QFileInfo fo(output);
        if(fo.exists()) {
            QFile::remove(fo.filePath());
        }
        if(!QFile::copy(fi.filePath(), fo.filePath())) {
            return -1;
        }
        fileCount++;
    }
    return fileCount;
}

//-----------------------------------------------------------------------------
void
TyphoonHQuickInterface::exportData()
{
    _copyingFiles = true;
    emit copyingFilesChanged();
    QTimer::singleShot(10, this, &TyphoonHQuickInterface::_exportData);
}

//-----------------------------------------------------------------------------
void
TyphoonHQuickInterface::_exportData()
{
    _copyResult = -1;
    int res = _copyFilesInPath(qgcApp()->toolbox()->settingsManager()->appSettings()->missionSavePath(),
                               QStringLiteral("/storage/sdcard1/") + AppSettings::missionDirectory);
    if(res >= 0) {
        _copyResult = res;
        emit copyResultChanged();
        res = _copyFilesInPath(qgcApp()->toolbox()->settingsManager()->appSettings()->telemetrySavePath(),
                               QStringLiteral("/storage/sdcard1/") + AppSettings::telemetryDirectory);
        if(res >= 0) {
            _copyResult += res;
            emit copyResultChanged();
            res = _copyFilesInPath(qgcApp()->toolbox()->settingsManager()->appSettings()->logSavePath(),
                                   QStringLiteral("/storage/sdcard1/") + AppSettings::logDirectory);
            if(res >= 0) {
                _copyResult += res;
                emit copyResultChanged();
            } else {
                _copyResult = -1;
            }
        }
    }
    _copyingFiles = false;
    emit copyingFilesChanged();
    emit copyResultChanged();
}

//-----------------------------------------------------------------------------
void
TyphoonHQuickInterface::importMission()
{
    _copyingFiles = true;
    emit copyingFilesChanged();
    QTimer::singleShot(10, this, &TyphoonHQuickInterface::_importMissions);
}

//-----------------------------------------------------------------------------
void
TyphoonHQuickInterface::_importMissions()
{
    _copyResult = 0;
    //-- See if there is a mission in the SD card
    QDirIterator it(QStringLiteral("/storage/sdcard1"), QStringList() << "*.plan", QDir::Files, QDirIterator::Subdirectories);
    QString missionDir = qgcApp()->toolbox()->settingsManager()->appSettings()->missionSavePath();
    while(it.hasNext()) {
        QFileInfo fi(it.next());
        QString output = missionDir + "/" + fi.fileName();
        QFileInfo fo(output);
        if(fo.exists()) {
            QFile::remove(fo.filePath());
        }
        if(!QFile::copy(fi.filePath(), fo.filePath())) {
            _copyResult = -1;
            break;
        }
        _copyResult++;
        emit copyResultChanged();
    }
    _copyingFiles = false;
    emit copyingFilesChanged();
    emit copyResultChanged();
}

//-----------------------------------------------------------------------------
void
TyphoonHQuickInterface::manualBind()
{
    if(_pHandler) {
        _pHandler->enterBindMode(true);
    }
}

//-----------------------------------------------------------------------------
void
TyphoonHQuickInterface::startCalibration()
{
    if(_pHandler) {
        _pHandler->startCalibration();
    }
}

//-----------------------------------------------------------------------------
void
TyphoonHQuickInterface::stopCalibration()
{
    if(_pHandler) {
        _pHandler->stopCalibration();
    }
}

//-----------------------------------------------------------------------------
static bool
compareRSSI(const QVariant &v1, const QVariant &v2)
{
    TyphoonSSIDItem* s1 = qobject_cast<TyphoonSSIDItem*>(qvariant_cast<QObject*>(v1));
    TyphoonSSIDItem* s2 = qobject_cast<TyphoonSSIDItem*>(qvariant_cast<QObject*>(v2));
    qDebug() << "Sort" << s1->rssi() << s2->rssi();
     return s1->rssi() > s2->rssi();
}

//-----------------------------------------------------------------------------
TyphoonSSIDItem*
TyphoonHQuickInterface::_findSsid(QString ssid, int rssi)
{
    for(QVariantList::const_iterator it = _ssidList.begin(); it != _ssidList.end(); ++it) {
        TyphoonSSIDItem* s = qobject_cast<TyphoonSSIDItem*>(qvariant_cast<QObject*>(*it));
        if(s->ssid() == ssid) {
            s->setRssi(rssi);
            return s;
        }
    }
    return NULL;
}

//-----------------------------------------------------------------------------
void
TyphoonHQuickInterface::_clearSSids()
{
    for(QVariantList::const_iterator it = _ssidList.begin(); it != _ssidList.end(); ++it) {
        TyphoonSSIDItem* s = qobject_cast<TyphoonSSIDItem*>(qvariant_cast<QObject*>(*it));
        if(s) {
           delete s;
        }
    }
    _ssidList.clear();
}

//-----------------------------------------------------------------------------
void
TyphoonHQuickInterface::_newSSID(QString ssid, int rssi)
{
    qDebug() << "New SSID" << ssid << rssi;
#if !defined(QT_DEBUG)
    if(ssid.startsWith("CGO3P") || ssid.startsWith("CGOET") || ssid.startsWith("CGOPRO") ||
        ssid.startsWith("E90_") || ssid.startsWith("E50_")) {
#endif
        if(!_findSsid(ssid, rssi)) {
            TyphoonSSIDItem* ssidInfo = new TyphoonSSIDItem(ssid, rssi);
            _ssidList.append(QVariant::fromValue((TyphoonSSIDItem*)ssidInfo));
            qSort(_ssidList.begin(), _ssidList.end(), compareRSSI);
            emit ssidListChanged();
        }
#if !defined(QT_DEBUG)
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
    //-- Remove configuration if we had it
    if(_configurations.contains(_ssid)) {
        _configurations.remove(_ssid);
        _saveWifiConfigurations();
    }
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
    //-- Save configuration
    _configurations[_ssid] = _password;
    _saveWifiConfigurations();
    _bindingWiFi = false;
    emit bindingWiFiChanged();
    emit connectedSSIDChanged();
    emit wifiConnectedChanged();
}

//-----------------------------------------------------------------------------
void
TyphoonHQuickInterface::_wifiDisconnected()
{
    emit connectedSSIDChanged();
    emit wifiConnectedChanged();
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
void
TyphoonHQuickInterface::_rawChannelsChanged()
{
    emit rawChannelChanged();
}

//-----------------------------------------------------------------------------
void
TyphoonHQuickInterface::_calibrationCompleteChanged()
{
    emit calibrationCompleteChanged();
}

//-----------------------------------------------------------------------------
void
TyphoonHQuickInterface::_calibrationStateChanged()
{
    emit calibrationStateChanged();
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

//-----------------------------------------------------------------------------
void
TyphoonHQuickInterface::_loadWifiConfigurations()
{
    qCDebug(YuneecLog) << "Loading WIFI Configurations";
    QSettings settings;
    settings.beginGroup(kWifiConfig);
    QStringList keys = settings.childKeys();
    foreach (QString key, keys) {
         _configurations[key] = settings.value(key).toString();
         qCDebug(YuneecLog) << key << _configurations[key];
    }
    settings.endGroup();
}

//-----------------------------------------------------------------------------
void
TyphoonHQuickInterface::_saveWifiConfigurations()
{
    QSettings settings;
    settings.beginGroup(kWifiConfig);
    settings.remove("");
    QMap<QString, QString>::const_iterator i = _configurations.constBegin();
    while (i != _configurations.constEnd()) {
        if(!i.key().isEmpty()) {
            settings.setValue(i.key(), i.value());
        }
        i++;
     }
    settings.endGroup();
}
