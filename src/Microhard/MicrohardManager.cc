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

#define LONG_TIMEOUT 5000

static const char *kMICROHARD_GROUP     = "Microhard";
static const char *kLOCAL_IP            = "LocalIP";
static const char *kREMOTE_IP           = "RemoteIP";
static const char *kGROUND_IP           = "GroundIP";
static const char *kAIR_IP              = "AirIP";
static const char *kNET_MASK            = "NetMask";
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
    _groundIPAddr   = settings.value(kGROUND_IP,      QString("192.168.168.101")).toString();
    _airIPAddr      = settings.value(kAIR_IP,         QString("192.168.168.213")).toString();
    _netMask        = settings.value(kNET_MASK,       QString("255.255.255.0")).toString();
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
    _isConnected = false;
    emit connectedChanged();
    _linkConnected = false;
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
bool
MicrohardManager::setIPSettings(QString localIP_, QString remoteIP_, QString groundIP_, QString airIP_, QString netMask_, QString cfgPassword_, QString encryptionKey_)
{
    if (_localIPAddr != localIP_ || _remoteIPAddr != remoteIP_ || _netMask != netMask_ ||
        _configPassword != cfgPassword_ || _encryptionKey != encryptionKey_ || _groundIPAddr != groundIP_ || _airIPAddr != airIP_)
    {
        if (_mhSettingsLoc && _encryptionKey != encryptionKey_) {
            _mhSettingsLoc->setEncryptionKey(encryptionKey_);
        }

        _localIPAddr    = localIP_;
        _remoteIPAddr   = remoteIP_;
        _groundIPAddr   = groundIP_;
        _airIPAddr      = airIP_;
        _netMask        = netMask_;
        _configPassword = cfgPassword_;
        _encryptionKey  = encryptionKey_;

        QSettings settings;
        settings.beginGroup(kMICROHARD_GROUP);
        settings.setValue(kLOCAL_IP, localIP_);
        settings.setValue(kREMOTE_IP, remoteIP_);
        settings.setValue(kGROUND_IP, groundIP_);
        settings.setValue(kAIR_IP, airIP_);
        settings.setValue(kNET_MASK, netMask_);
        settings.setValue(kCFG_PASSWORD, cfgPassword_);
        settings.setValue(kENC_KEY, encryptionKey_);
        settings.endGroup();

        _reset();

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
        _workTimer.start(1000);
    } else {
        //-- Stop everything
        _close();
    }
    _enabled = enable;
}

//-----------------------------------------------------------------------------
void
MicrohardManager::_connectedLoc()
{
    qCDebug(MicrohardLog) << "GND Microhard Settings Connected";
    _isConnected = true;
    _locTimer.start(LONG_TIMEOUT);
    emit connectedChanged();
}

//-----------------------------------------------------------------------------
void
MicrohardManager::_connectedRem()
{
    qCDebug(MicrohardLog) << "AIR Microhard Settings Connected";
    _linkConnected = true;
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
    _isConnected = false;
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
    _linkConnected = false;
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

        if(!_isConnected) {
            _mhSettingsLoc->start();
        } else {
            _mhSettingsLoc->getStatus();
        }
        if(!_linkConnected) {
            _mhSettingsRem->start();
        } else {
            _mhSettingsRem->getStatus();
        }
    }
    _workTimer.start(_isConnected ? 1000 : LONG_TIMEOUT);
}
