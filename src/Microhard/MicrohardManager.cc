/****************************************************************************
 *
 *   (c) 2019 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "MicrohardManager.h"
#include "MicrohardSettings.h"
#include "SettingsManager.h"
#include "QGCApplication.h"
#include "QGCCorePlugin.h"

#include <QSettings>

#define SHORT_TIMEOUT 2500
#define LONG_TIMEOUT  5000

static const char *kMICROHARD_GROUP     = "Microhard";
static const char *kLOCAL_IP            = "LocalIP";
static const char *kREMOTE_IP           = "RemoteIP";
static const char *kNET_MASK            = "NetMask";
static const char *kCFG_USERNAME        = "ConfigUserName";
static const char *kCFG_PASSWORD        = "ConfigPassword";
static const char *kENC_KEY             = "EncryptionKey";

//-----------------------------------------------------------------------------
MicrohardManager::MicrohardManager(QGCApplication* app, QGCToolbox* toolbox)
    : QGCTool(app, toolbox)
{
    connect(&_workTimer, &QTimer::timeout, this, &MicrohardManager::_checkMicrohard);
    _workTimer.setSingleShot(true);
    connect(&_locTimer, &QTimer::timeout, this, &MicrohardManager::_locTimeout);
    connect(&_remTimer, &QTimer::timeout, this, &MicrohardManager::_remTimeout);
    QSettings settings;
    settings.beginGroup(kMICROHARD_GROUP);
    _localIPAddr    = settings.value(kLOCAL_IP,       QString("192.168.168.1")).toString();
    _remoteIPAddr   = settings.value(kREMOTE_IP,      QString("192.168.168.2")).toString();
    _netMask        = settings.value(kNET_MASK,       QString("255.255.255.0")).toString();
    _configUserName = settings.value(kCFG_USERNAME,   QString("admin")).toString();
    _configPassword = settings.value(kCFG_PASSWORD,   QString("admin")).toString();
    _encryptionKey  = settings.value(kENC_KEY,        QString("1234567890")).toString();
    settings.endGroup();
}

//-----------------------------------------------------------------------------
MicrohardManager::~MicrohardManager()
{
    _close();
}

//-----------------------------------------------------------------------------
void
MicrohardManager::_close()
{
    _workTimer.stop();
    _locTimer.stop();
    _remTimer.stop();
    if(_mhSettingsLoc) {
        _mhSettingsLoc->close();
        _mhSettingsLoc->deleteLater();
        _mhSettingsLoc = nullptr;
    }
    if(_mhSettingsRem) {
        _mhSettingsRem->close();
        _mhSettingsRem->deleteLater();
        _mhSettingsRem = nullptr;
    }
}

//-----------------------------------------------------------------------------
void
MicrohardManager::_reset()
{
    _close();
    _connectedStatus = 0;
    emit connectedChanged();
    _linkConnectedStatus = 0;
    emit linkConnectedChanged();
    if(!_appSettings) {
        _appSettings = _toolbox->settingsManager()->appSettings();
        connect(_appSettings->enableMicrohard(), &Fact::rawValueChanged, this, &MicrohardManager::_setEnabled);
    }
    _setEnabled();
}

//-----------------------------------------------------------------------------
FactMetaData*
MicrohardManager::_createMetadata(const char* name, QStringList enums)
{
    FactMetaData* metaData = new FactMetaData(FactMetaData::valueTypeUint32, name, this);
    QQmlEngine::setObjectOwnership(metaData, QQmlEngine::CppOwnership);
    metaData->setShortDescription(name);
    metaData->setLongDescription(name);
    metaData->setRawDefaultValue(QVariant(0));
    metaData->setHasControl(true);
    metaData->setReadOnly(false);
    for(int i = 0; i < enums.size(); i++) {
        metaData->addEnumInfo(enums[i], QVariant(i));
    }
    metaData->setRawMin(0);
    metaData->setRawMin(enums.size() - 1);
    return metaData;
}

//-----------------------------------------------------------------------------
void
MicrohardManager::setToolbox(QGCToolbox* toolbox)
{
    QGCTool::setToolbox(toolbox);
    //-- Start it all
    _reset();
}

//-----------------------------------------------------------------------------
void
MicrohardManager::switchToConnectionEncryptionKey(QString encryptionKey)
{
    _communicationEncryptionKey = encryptionKey;
    _useCommunicationEncryptionKey = true;
}

//-----------------------------------------------------------------------------
void
MicrohardManager::switchToPairingEncryptionKey()
{
    _useCommunicationEncryptionKey = false;
}

//-----------------------------------------------------------------------------
void
MicrohardManager::setEncryptionKey()
{
    if (_mhSettingsLoc) {
        _mhSettingsLoc->setEncryptionKey(_useCommunicationEncryptionKey ? _communicationEncryptionKey : _encryptionKey);
    }
}

//-----------------------------------------------------------------------------
void
MicrohardManager::updateSettings()
{
    setEncryptionKey();
    QSettings settings;
    settings.beginGroup(kMICROHARD_GROUP);
    settings.setValue(kLOCAL_IP, _localIPAddr);
    settings.setValue(kREMOTE_IP, _remoteIPAddr);
    settings.setValue(kNET_MASK, _netMask);
    settings.setValue(kCFG_PASSWORD, _configPassword);
    settings.setValue(kENC_KEY, _encryptionKey);
    settings.endGroup();

    _reset();
}

//-----------------------------------------------------------------------------
bool
MicrohardManager::setIPSettings(QString localIP_, QString remoteIP_, QString netMask_, QString cfgUserName_, QString cfgPassword_, QString encryptionKey_)
{
    if (_localIPAddr != localIP_ || _remoteIPAddr != remoteIP_ || _netMask != netMask_ ||
        _configUserName != cfgUserName_ || _configPassword != cfgPassword_ || _encryptionKey != encryptionKey_)
    {
        _localIPAddr    = localIP_;
        _remoteIPAddr   = remoteIP_;
        _netMask        = netMask_;
        _configUserName = cfgUserName_;
        _configPassword = cfgPassword_;
        _encryptionKey  = encryptionKey_;

        updateSettings();

        return true;
    }

    return false;
}

//-----------------------------------------------------------------------------
void
MicrohardManager::_setEnabled()
{
    bool enable = _appSettings->enableMicrohard()->rawValue().toBool();
    if(enable) {
        if(!_mhSettingsLoc) {
            _mhSettingsLoc = new MicrohardSettings(localIPAddr(), this, true);
            connect(_mhSettingsLoc, &MicrohardSettings::connected,      this, &MicrohardManager::_connectedLoc);
            connect(_mhSettingsLoc, &MicrohardSettings::rssiUpdated,    this, &MicrohardManager::_rssiUpdatedLoc);
        }
        if(!_mhSettingsRem) {
            _mhSettingsRem = new MicrohardSettings(remoteIPAddr(), this);
            connect(_mhSettingsRem, &MicrohardSettings::connected,      this, &MicrohardManager::_connectedRem);
            connect(_mhSettingsRem, &MicrohardSettings::rssiUpdated,    this, &MicrohardManager::_rssiUpdatedRem);
        }
        _workTimer.start(SHORT_TIMEOUT);
    } else {
        //-- Stop everything
        _close();
    }
    _enabled = enable;
}

//-----------------------------------------------------------------------------
void
MicrohardManager::_connectedLoc(int status)
{
    static const char* msg = "GND Microhard Settings: ";
    if(status > 0)
        qCDebug(MicrohardLog) << msg << "Connected";
    else if(status < 0)
        qCDebug(MicrohardLog) << msg << "Error";
    else
        qCDebug(MicrohardLog) << msg << "Not Connected";
    _connectedStatus = status;
    _locTimer.start(LONG_TIMEOUT);
    emit connectedChanged();
}

//-----------------------------------------------------------------------------
void
MicrohardManager::_connectedRem(int status)
{
    static const char* msg = "AIR Microhard Settings: ";
    if(status > 0)
        qCDebug(MicrohardLog) << msg << "Connected";
    else if(status < 0)
        qCDebug(MicrohardLog) << msg << "Error";
    else
        qCDebug(MicrohardLog) << msg << "Not Connected";
    _linkConnectedStatus = status;
    _remTimer.start(LONG_TIMEOUT);
    emit linkConnectedChanged();
}

//-----------------------------------------------------------------------------
void
MicrohardManager::_rssiUpdatedLoc(int rssi)
{
    _downlinkRSSI = rssi;
    _locTimer.stop();
    _locTimer.start(LONG_TIMEOUT);
    emit connectedChanged();
    emit linkChanged();
}

//-----------------------------------------------------------------------------
void
MicrohardManager::_rssiUpdatedRem(int rssi)
{
    _uplinkRSSI = rssi;
    _remTimer.stop();
    _remTimer.start(LONG_TIMEOUT);
    emit linkConnectedChanged();
    emit linkChanged();
}

//-----------------------------------------------------------------------------
void
MicrohardManager::_locTimeout()
{
    _locTimer.stop();
    _connectedStatus = 0;
    if(_mhSettingsLoc) {
        _mhSettingsLoc->close();
        _mhSettingsLoc->deleteLater();
        _mhSettingsLoc = nullptr;
    }
    emit connectedChanged();
}

//-----------------------------------------------------------------------------
void
MicrohardManager::_remTimeout()
{
    _remTimer.stop();
    _linkConnectedStatus = 0;
    if(_mhSettingsRem) {
        _mhSettingsRem->close();
        _mhSettingsRem->deleteLater();
        _mhSettingsRem = nullptr;
    }
    emit linkConnectedChanged();
}

//-----------------------------------------------------------------------------
void
MicrohardManager::_checkMicrohard()
{
    if(_enabled) {
        if(!_mhSettingsLoc || !_mhSettingsRem) {
            _setEnabled();
            return;
        }

        if(_connectedStatus <= 0) {
            _mhSettingsLoc->start();
        } else {
            _mhSettingsLoc->getStatus();
        }
        if(_linkConnectedStatus <= 0) {
            _mhSettingsRem->start();
        } else {
            _mhSettingsRem->getStatus();
        }
    }
    _workTimer.start(_connectedStatus > 0 ? SHORT_TIMEOUT : LONG_TIMEOUT);
}
