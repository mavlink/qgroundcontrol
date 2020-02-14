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

#ifdef QGC_ENABLE_PAIRING
#include "PairingManager.h"
#endif

#include <QSettings>

static const char *kMICROHARD_GROUP     = "Microhard";
static const char *kLOCAL_IP            = "LocalIP";
static const char *kREMOTE_IP           = "RemoteIP";
static const char *kNET_MASK            = "NetMask";
static const char *kCFG_USERNAME        = "ConfigUserName";
static const char *kCFG_PASSWORD        = "ConfigPassword";
static const char *kENC_KEY             = "EncryptionKey";
static const char *kNETWORK_ID          = "NetworkId";
static const char *kPAIR_CH             = "PairingChannel";
static const char *kCONN_CH             = "ConnectingChannel";
static const char *kCONN_BW             = "ConnectingBandwidth";
static const char *kCONN_PW             = "ConnectingPower";
static const char *kMODEM_NAME          = "ModemName";

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

    connect(this, &MicrohardManager::pairingChannelChanged, this, &MicrohardManager::_updateSettings, Qt::QueuedConnection);

    QSettings settings;
    settings.beginGroup(kMICROHARD_GROUP);
    _localIPAddr         = settings.value(kLOCAL_IP,       QString("192.168.168.1")).toString();
    _remoteIPAddr        = settings.value(kREMOTE_IP,      QString("192.168.168.2")).toString();
    _netMask             = settings.value(kNET_MASK,       QString("255.255.255.0")).toString();
    _configUserName      = settings.value(kCFG_USERNAME,   QString("admin")).toString();
    _configPassword      = settings.value(kCFG_PASSWORD,   QString("admin")).toString();
    _encryptionKey       = settings.value(kENC_KEY,        QString("1234567890")).toString();
    _networkId           = settings.value(kNETWORK_ID,     QString("MH")).toString();
    _pairingChannel      = settings.value(kPAIR_CH,        DEFAULT_PAIRING_CHANNEL).toInt();
    _connectingChannel   = settings.value(kCONN_CH,        DEFAULT_PAIRING_CHANNEL).toInt();
    _connectingBandwidth = settings.value(kCONN_BW,        DEFAULT_CONNECTING_BANDWIDTH).toInt();
    _connectingPower     = settings.value(kCONN_PW,        DEFAULT_CONNECTING_POWER).toInt();
    setProductName(settings.value(kMODEM_NAME, QString("pDDL1800")).toString());
    settings.endGroup();

    //-- Start it all
    _reset();
}

//-----------------------------------------------------------------------------
void
MicrohardManager::_close()
{
    if (_mhSettingsLocThread) {
        _mhSettingsLocThread->quit();
        _mhSettingsLocThread = nullptr;
    }
    _mhSettingsLoc = nullptr;
    if (_mhSettingsRemThread) {
        _mhSettingsRemThread->quit();
        _mhSettingsRemThread = nullptr;
    }
    _mhSettingsRem = nullptr;
}

//-----------------------------------------------------------------------------
void
MicrohardManager::_reset()
{
    _close();
    _connectedStatus = _linkConnectedStatus = tr("Not Connected");
    emit connectedChanged();
    emit linkConnectedChanged();
    if (!_appSettings) {
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
MicrohardManager::switchToPairingEncryptionKey(QString pairingKey)
{
    setEncryptionKey(pairingKey);
    _usePairingSettings = true;
}

//-----------------------------------------------------------------------------
void
MicrohardManager::configure()
{
#ifdef QGC_ENABLE_PAIRING
    if (_toolbox->pairingManager()->usePairing()) {
        if (_usePairingSettings) {
            emit configureMicrohard(_encryptionKey, _pairingPower, adjustChannelToBandwitdh(_pairingChannel, _pairingBandwidth), _pairingBandwidth, _networkId);
        } else {
            emit configureMicrohard(_communicationEncryptionKey, _connectingPower, adjustChannelToBandwitdh(_connectingChannel, _connectingBandwidth), _connectingBandwidth, _connectingNetworkId);
        }
        return;
    }
#endif
    emit configureMicrohard(_encryptionKey, 0, adjustChannelToBandwitdh(_connectingChannel, _connectingBandwidth), _connectingBandwidth, _networkId);
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
    settings.setValue(kNETWORK_ID, _networkId);
    settings.setValue(kPAIR_CH, QString::number(_pairingChannel));
    settings.setValue(kCONN_CH, QString::number(_connectingChannel));
    settings.setValue(kCONN_BW, QString::number(_connectingBandwidth));
    settings.setValue(kCONN_PW, QString::number(_connectingPower));
    settings.setValue(kMODEM_NAME, _modemName);
    settings.endGroup();
}

//-----------------------------------------------------------------------------
void
MicrohardManager::updateSettings()
{
    configure();
    _updateSettings();
}

//-----------------------------------------------------------------------------
bool
MicrohardManager::setIPSettings(QString localIP, QString remoteIP, QString netMask, QString cfgUserName, QString cfgPassword,
                                QString encryptionKey, QString networkId, int channel, int bandwidth)
{
    if (_localIPAddr != localIP || _remoteIPAddr != remoteIP || _netMask != netMask ||
        _configUserName != cfgUserName || _configPassword != cfgPassword || _encryptionKey != encryptionKey ||
        _networkId != networkId || _connectingChannel != channel || _connectingBandwidth != bandwidth)
    {
#ifdef QGC_ENABLE_PAIRING
        bool pairingKeyChanged = (_encryptionKey != encryptionKey);
        bool networkIdChanged = (_networkId != networkId);
        bool connectingChannelChanged = (_connectingChannel != channel);
#endif

        _localIPAddr         = localIP;
        _remoteIPAddr        = remoteIP;
        _netMask             = netMask;
        _configUserName      = cfgUserName;
        _configPassword      = cfgPassword;
        _encryptionKey       = encryptionKey;
        _networkId           = networkId;
        _connectingChannel   = channel;
        _connectingBandwidth = bandwidth;

        updateSettings();

#ifdef QGC_ENABLE_PAIRING
        if (connectingChannelChanged) {
            emit _toolbox->pairingManager()->connectingChannelChanged();
            _toolbox->pairingManager()->setModemParameters(_connectingChannel, connectingPower(), connectingBandwidth());
        }
        if (pairingKeyChanged) {
            emit _toolbox->pairingManager()->pairingKeyChanged();
        }
        if (networkIdChanged) {
            emit _toolbox->pairingManager()->networkIdChanged();
        }
#endif

        return true;
    }

    return false;
}

//-----------------------------------------------------------------------------
void
MicrohardManager::setShowRemote(bool val)
{
    if (_showRemote != val) {
        _showRemote = val;
        _reset();
        emit showRemoteChanged();
    }
}

//-----------------------------------------------------------------------------
void
MicrohardManager::_setEnabled()
{
    bool enable = _appSettings->enableMicrohard()->rawValue().toBool();
    if (enable) {
        if (!_mhSettingsLoc) {
            _mhSettingsLocThread = new QThread(this);
            _mhSettingsLoc = new MicrohardSettings(localIPAddr(), true);
            _mhSettingsLoc->moveToThread(_mhSettingsLocThread);
            connect(_mhSettingsLocThread, &QThread::finished, _mhSettingsLoc, &QObject::deleteLater, Qt::QueuedConnection);
            connect(this, &MicrohardManager::run, _mhSettingsLoc, &MicrohardSettings::run, Qt::QueuedConnection);
            connect(this, &MicrohardManager::configureMicrohard, _mhSettingsLoc, &MicrohardSettings::configure, Qt::QueuedConnection);
            connect(_mhSettingsLoc, &MicrohardSettings::connected, this, &MicrohardManager::_connectedLoc, Qt::QueuedConnection);
            connect(_mhSettingsLoc, &MicrohardSettings::rssiUpdated, this, &MicrohardManager::_rssiUpdatedLoc, Qt::QueuedConnection);
            _mhSettingsLocThread->start();
        }
        if(!_mhSettingsRem && _showRemote) {
            _mhSettingsRemThread = new QThread(this);
            _mhSettingsRem = new MicrohardSettings(remoteIPAddr());
            _mhSettingsRem->moveToThread(_mhSettingsRemThread);
            connect(_mhSettingsLocThread, &QThread::finished, _mhSettingsRem, &QObject::deleteLater, Qt::QueuedConnection);
            connect(this, &MicrohardManager::run, _mhSettingsRem, &MicrohardSettings::run, Qt::QueuedConnection);
            connect(_mhSettingsRem, &MicrohardSettings::connected, this, &MicrohardManager::_connectedRem, Qt::QueuedConnection);
            connect(_mhSettingsRem, &MicrohardSettings::rssiUpdated, this, &MicrohardManager::_rssiUpdatedRem, Qt::QueuedConnection);
            _mhSettingsRemThread->start();
        }
        emit run();
    } else {
        //-- Stop everything
        _close();
    }
    _enabled = enable;
}

//-----------------------------------------------------------------------------
void
MicrohardManager::_connectedLoc(const QString& status)
{
    if (_connectedStatus != status) {
        _connectedStatus = status;
        qCDebug(MicrohardLog) << "Ground Microhard Settings: " << _connectedStatus;
        emit connectedChanged();
    }
}

//-----------------------------------------------------------------------------
void
MicrohardManager::_connectedRem(const QString& status)
{
    if (_linkConnectedStatus != status) {
        _linkConnectedStatus = status;
        qCDebug(MicrohardLog) << "Air Microhard Settings: " << _linkConnectedStatus;
        emit linkConnectedChanged();
    }
}

//-----------------------------------------------------------------------------
void
MicrohardManager::_rssiUpdatedLoc(int rssi)
{
    setDownlinkRSSI(rssi);
    emit connectedChanged();
}

//-----------------------------------------------------------------------------
void
MicrohardManager::_rssiUpdatedRem(int rssi)
{
    setUplinkRSSI(rssi);
    emit linkConnectedChanged();
}

//-----------------------------------------------------------------------------
void
MicrohardManager::setProductName(QString product)
{
    if (!product.isEmpty()) {
        qCDebug(MicrohardLog) << "Microhard modem: " << product;
    }
    _channelMin = 4;
    _channelMax = 78;
    _frequencyStart = 2405;

    _bandwidthLabels.clear();
    _bandwidthLabels.append("8 MHz");
    _bandwidthLabels.append("4 MHz");

    _pairingBandwidth = DEFAULT_PAIRING_BANDWIDTH;

    if (product.contains("DDL2350")) {
        _channelMin = 1;
        _channelMax = 81;
        _frequencyStart = 2310;
        _bandwidthChannelMin.clear();
        _bandwidthChannelMin.append(3);
        _bandwidthChannelMin.append(1);
        _bandwidthChannelMax.clear();
        _bandwidthChannelMax.append(79);
        _bandwidthChannelMax.append(81);
    } else if (product.contains("DDL2450")) {
        _channelMin = 4;
        _channelMax = 78;
        _frequencyStart = 2405;
        _bandwidthChannelMin.clear();
        _bandwidthChannelMin.append(6);
        _bandwidthChannelMin.append(4);
        _bandwidthChannelMax.clear();
        _bandwidthChannelMax.append(76);
        _bandwidthChannelMax.append(78);
    } else if (product.contains("DDL1800")) {
        _channelMin = 1;
        _channelMax = 59;
        _frequencyStart = 1811;
        _bandwidthLabels.clear();
        _bandwidthLabels.append("8 MHz");
        _bandwidthLabels.append("4 MHz");
        _bandwidthLabels.append("2 MHz");
        _bandwidthLabels.append("1 MHz");
        _bandwidthChannelMin.clear();
        _bandwidthChannelMin.append(4);
        _bandwidthChannelMin.append(3);
        _bandwidthChannelMin.append(2);
        _bandwidthChannelMin.append(1);
        _bandwidthChannelMax.clear();
        _bandwidthChannelMax.append(56);
        _bandwidthChannelMax.append(57);
        _bandwidthChannelMax.append(58);
        _bandwidthChannelMax.append(59);
        _pairingBandwidth = 3;
    }

    _channelLabels.clear();
    for (int i = _channelMin; i <= _channelMax; i++) {
        _channelLabels.append(QString::number(i).rightJustified(2, '0') +
                              " - " +
                              QString::number(getChannelFrequency(i)) +
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

    if (_modemName != product) {
        _modemName = product;
        updateSettings();
    }

    emit channelLabelsChanged();
    emit bandwidthLabelsChanged();
    emit pairingChannelChanged();
    emit connectingChannelChanged();
}

//-----------------------------------------------------------------------------
int
MicrohardManager::adjustChannelToBandwitdh(int channel, int bandwidth)
{
    int result = channel;
    if (_bandwidthChannelMin.length() > bandwidth && channel < _bandwidthChannelMin[bandwidth]) {
        result = _bandwidthChannelMin[bandwidth];
    } else if (_bandwidthChannelMax.length() > bandwidth && channel > _bandwidthChannelMax[bandwidth]) {
        result = _bandwidthChannelMax[bandwidth];
    }
    return result;
}

//-----------------------------------------------------------------------------
// https://www.adriangranados.com/blog/dbm-to-percent-conversion
int
MicrohardManager::downlinkRSSIPct()
{
    double dbm = static_cast<double>(_downlinkRSSI);
    if (dbm < -92) return 0;
    if (dbm > -21) return 100;
    return static_cast<int>(round((-0.0154 * dbm * dbm) - (0.3794 * dbm) + 98.182));
}

//-----------------------------------------------------------------------------
