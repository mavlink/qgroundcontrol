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
#include "PairingManager.h"
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
static const char *kPAIR_CH             = "PairingChannel";
static const char *kCONN_CH             = "ConnectingChannel";

//-----------------------------------------------------------------------------
MicrohardManager::MicrohardManager(QGCApplication* app, QGCToolbox* toolbox)
    : QGCTool(app, toolbox)
{
}

//-----------------------------------------------------------------------------
MicrohardManager::~MicrohardManager()
{
    _close();
}

//-----------------------------------------------------------------------------
void
MicrohardManager::setToolbox(QGCToolbox* toolbox)
{
    QGCTool::setToolbox(toolbox);

    connect(&_workTimer, &QTimer::timeout, this, &MicrohardManager::_checkMicrohard, Qt::QueuedConnection);
    _workTimer.setSingleShot(true);
    connect(&_locTimer, &QTimer::timeout, this, &MicrohardManager::_locTimeout, Qt::QueuedConnection);
    connect(&_remTimer, &QTimer::timeout, this, &MicrohardManager::_remTimeout, Qt::QueuedConnection);
    connect(this, &MicrohardManager::pairingChannelChanged, this, &MicrohardManager::_updateSettings, Qt::QueuedConnection);

    QSettings settings;
    settings.beginGroup(kMICROHARD_GROUP);
    _localIPAddr       = settings.value(kLOCAL_IP,       QString("192.168.168.1")).toString();
    _remoteIPAddr      = settings.value(kREMOTE_IP,      QString("192.168.168.2")).toString();
    _netMask           = settings.value(kNET_MASK,       QString("255.255.255.0")).toString();
    _configUserName    = settings.value(kCFG_USERNAME,   QString("admin")).toString();
    _configPassword    = settings.value(kCFG_PASSWORD,   QString("admin")).toString();
    _encryptionKey     = settings.value(kENC_KEY,        QString("1234567890")).toString();
    _pairingChannel    = settings.value(kPAIR_CH,        DEFAULT_PAIRING_CHANNEL).toInt();
    _connectingChannel = settings.value(kCONN_CH,        DEFAULT_PAIRING_CHANNEL).toInt();
    settings.endGroup();

    setProductName("");

    //-- Start it all
    _reset();
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
        connect(_appSettings->enableMicrohard(), &Fact::rawValueChanged, this, &MicrohardManager::_setEnabled, Qt::QueuedConnection);
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
MicrohardManager::switchToConnectionEncryptionKey(QString encryptionKey)
{
    _communicationEncryptionKey = encryptionKey;
    _usePairingSettings = false;
}

//-----------------------------------------------------------------------------
void
MicrohardManager::switchToPairingEncryptionKey()
{
    _usePairingSettings = true;
}

//-----------------------------------------------------------------------------
void
MicrohardManager::configure()
{
    if (_mhSettingsLoc) {
        if (_toolbox->pairingManager()->usePairing()) {
            if (_usePairingSettings) {
                _mhSettingsLoc->configure(_encryptionKey, _pairingPower, _pairingChannel);
            } else {
                _mhSettingsLoc->configure(_communicationEncryptionKey, _connectingPower, _connectingChannel);
            }
        } else {
            _mhSettingsLoc->configure(_encryptionKey, _pairingPower, _connectingChannel);
        }
    }
}

//-----------------------------------------------------------------------------
void
MicrohardManager::_updateSettings()
{
    QSettings settings;
    settings.beginGroup(kMICROHARD_GROUP);
    settings.setValue(kLOCAL_IP, _localIPAddr);
    settings.setValue(kREMOTE_IP, _remoteIPAddr);
    settings.setValue(kNET_MASK, _netMask);
    settings.setValue(kCFG_PASSWORD, _configPassword);
    settings.setValue(kENC_KEY, _encryptionKey);
    settings.setValue(kPAIR_CH, QString::number(_pairingChannel));
    settings.setValue(kCONN_CH, QString::number(_connectingChannel));
    settings.endGroup();
}

//-----------------------------------------------------------------------------
void
MicrohardManager::updateSettings()
{
    configure();
    _updateSettings();
    _reset();
}

//-----------------------------------------------------------------------------
bool
MicrohardManager::setIPSettings(QString localIP, QString remoteIP, QString netMask, QString cfgUserName, QString cfgPassword, QString encryptionKey, int channel)
{
    if (_localIPAddr != localIP || _remoteIPAddr != remoteIP || _netMask != netMask ||
        _configUserName != cfgUserName || _configPassword != cfgPassword || _encryptionKey != encryptionKey || _connectingChannel != channel)
    {
        _localIPAddr       = localIP;
        _remoteIPAddr      = remoteIP;
        _netMask           = netMask;
        _configUserName    = cfgUserName;
        _configPassword    = cfgPassword;
        _encryptionKey     = encryptionKey;
        _connectingChannel = channel;

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
            connect(_mhSettingsLoc, &MicrohardSettings::connected,      this, &MicrohardManager::_connectedLoc, Qt::QueuedConnection);
            connect(_mhSettingsLoc, &MicrohardSettings::rssiUpdated,    this, &MicrohardManager::_rssiUpdatedLoc, Qt::QueuedConnection);
        }
        if(!_mhSettingsRem) {
            _mhSettingsRem = new MicrohardSettings(remoteIPAddr(), this);
            connect(_mhSettingsRem, &MicrohardSettings::connected,      this, &MicrohardManager::_connectedRem, Qt::QueuedConnection);
            connect(_mhSettingsRem, &MicrohardSettings::rssiUpdated,    this, &MicrohardManager::_rssiUpdatedRem, Qt::QueuedConnection);
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

//-----------------------------------------------------------------------------
void
MicrohardManager::setProductName(QString product)
{
    qCDebug(MicrohardLog) << "Detected Microhard modem: " << product;

    _channelMin = 6;
    _channelMax = 76;
    int frequencyStart = 2407;

    if (product == "pMDDL2350" || product == "pDDL2350") {
        _channelMin = 1;
        _channelMax = 81;
        frequencyStart = 2310;
    } else if (product == "pMDDL2450" || product == "pDDL2450") {
        _channelMin = 6;
        _channelMax = 76;
        frequencyStart = 2407;
    } else if (product == "pMDDL1800" || product == "pDDL1800" ) {
        _channelMin = 3;
        _channelMax = 57;
        frequencyStart = 1813;
    }

    _channelLabels.clear();
    for (int i = _channelMin; i <= _channelMax; i++) {
        _channelLabels.append(QString::number(i) +
                              " - " +
                              QString::number(i + frequencyStart - _channelMin) +
                              " MHz");
    }

    if (_pairingChannel < _channelMin) {
        _pairingChannel = _channelMin;
    } else if (_pairingChannel > _channelMax) {
        _pairingChannel = _channelMax;
    }
    if (_connectingChannel < _channelMin) {
        _connectingChannel = _channelMin;
    } else if (_connectingChannel > _channelMax) {
        _connectingChannel = _channelMax;
    }

    emit channelLabelsChanged();
    emit pairingChannelChanged();
    emit connectingChannelChanged();
}

//-----------------------------------------------------------------------------
