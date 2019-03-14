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

static const char *kMICROHARD_GROUP     = "Microhard";
static const char *kLOCAL_IP            = "LocalIP";
static const char *kREMOTE_IP           = "RemoteIP";
static const char *kNET_MASK            = "NetMask";
static const char *kCFG_PASSWORD        = "ConfigPassword";

//-----------------------------------------------------------------------------
MicrohardManager::MicrohardManager(QGCApplication* app, QGCToolbox* toolbox)
    : QGCTool(app, toolbox)
{
    connect(&_workTimer, &QTimer::timeout, this, &MicrohardManager::_checkMicrohard);
    _workTimer.setSingleShot(true);
    QSettings settings;
    settings.beginGroup(kMICROHARD_GROUP);
    _localIPAddr    = settings.value(kLOCAL_IP,       QString("192.168.168.1")).toString();
    _remoteIPAddr   = settings.value(kREMOTE_IP,      QString("192.168.168.2")).toString();
    _netMask        = settings.value(kNET_MASK,       QString("255.255.255.0")).toString();
    _configPassword = settings.value(kCFG_PASSWORD,   QString("admin")).toString();
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
    if(_mhSettings) {
        _mhSettings->close();
        _mhSettings->deleteLater();
        _mhSettings = nullptr;
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
MicrohardManager::setIPSettings(QString localIP_, QString remoteIP_, QString netMask_)
{
    bool res = false;
    if(_localIPAddr != localIP_ || _remoteIPAddr != remoteIP_ || _netMask != netMask_) {
        //-- If we are connected to the Microhard
        if(_isConnected) {
            if(_mhSettings) {
                //-- Change IP settings
                res = _mhSettings->setIPSettings(localIP_, remoteIP_, netMask_);
                if(res) {
                    _needReboot = true;
                    emit needRebootChanged();
                }
            }
        } else {
            //-- We're not connected. Record the change and restart.
            _localIPAddr  = localIP_;
            _remoteIPAddr = remoteIP_;
            _netMask      = netMask_;
            _reset();
            res = true;
        }
        if(res) {
            QSettings settings;
            settings.beginGroup(kMICROHARD_GROUP);
            settings.setValue(kLOCAL_IP, localIP_);
            settings.setValue(kREMOTE_IP, remoteIP_);
            settings.setValue(kNET_MASK, netMask_);
            settings.endGroup();
        }
    } else {
        //-- Nothing to change
        res = true;
    }
    return res;
}

//-----------------------------------------------------------------------------
void
MicrohardManager::_setEnabled()
{
    bool enable = _appSettings->enableMicrohard()->rawValue().toBool();
    if(enable) {
        if(!_mhSettings) {
            _mhSettings = new MicrohardSettings(this);
            connect(_mhSettings, &MicrohardSettings::updateSettings, this, &MicrohardManager::_updateSettings);
            connect(_mhSettings, &MicrohardSettings::connected,      this, &MicrohardManager::_connected);
            connect(_mhSettings, &MicrohardSettings::disconnected,   this, &MicrohardManager::_disconnected);
        }
        _reqMask = static_cast<uint32_t>(REQ_ALL);
        _workTimer.start(1000);
    } else {
        //-- Stop everything
        _workTimer.stop();
        _close();
    }
    _enabled = enable;
}

//-----------------------------------------------------------------------------
void
MicrohardManager::_connected()
{
    qCDebug(MicrohardLog) << "Microhard Settings Connected";
    _isConnected = true;
    emit connectedChanged();
    _needReboot = false;
    emit needRebootChanged();
}

//-----------------------------------------------------------------------------
void
MicrohardManager::_disconnected()
{
    qCDebug(MicrohardLog) << "Microhard Settings Disconnected";
    _isConnected = false;
    emit connectedChanged();
    _needReboot = false;
    emit needRebootChanged();
    _linkConnected = false;
    emit linkConnectedChanged();
    _reset();
}

//-----------------------------------------------------------------------------
void
MicrohardManager::_checkMicrohard()
{
    if(_enabled) {
        if(!_isConnected) {
            if(_mhSettings) {
               _mhSettings->start();
            }
        } else {
            //qCDebug(MicrohardVerbose) << bin << _reqMask;
            while(true) {
                if (_reqMask & REQ_LINK_STATUS) {
                    _mhSettings->requestLinkStatus();
                    break;
                }
                //-- Check link status
                if(_timeoutTimer.elapsed() > 3000) {
                    //-- Give up and restart
                    _disconnected();
                    break;
                }
                //-- If it's been too long since we last heard, ping it.
                if(_timeoutTimer.elapsed() > 1000) {
                    _mhSettings->requestLinkStatus();
                    break;
                }
                break;
            }
        }
        _workTimer.start(_isConnected ? 500 : 5000);
    }
}

//-----------------------------------------------------------------------------
void
MicrohardManager::_updateSettings(QByteArray jSonData)
{
    _timeoutTimer.start();
    qCDebug(MicrohardVerbose) << jSonData;
    QJsonParseError jsonParseError;
    QJsonDocument doc = QJsonDocument::fromJson(jSonData, &jsonParseError);
    if (jsonParseError.error != QJsonParseError::NoError) {
        qWarning() <<  "Unable to parse Microhard response:" << jsonParseError.errorString() << jsonParseError.offset;
        return;
    }
    QJsonObject jObj = doc.object();
    //-- Link Status?
    if(jSonData.contains("\"flight\":")) {
        _reqMask &= ~static_cast<uint32_t>(REQ_LINK_STATUS);
        bool tlinkConnected  = jObj["flight"].toString("") == "online";
        if(tlinkConnected != _linkConnected) {
           _linkConnected = tlinkConnected;
           emit linkConnectedChanged();
        }
        int     tdownlinkRSSI   = jObj["radiorssi"].toInt(_downlinkRSSI);
        int     tuplinkRSSI     = jObj["hdrssi"].toInt(_uplinkRSSI);
        if(_downlinkRSSI != tdownlinkRSSI || _uplinkRSSI != tuplinkRSSI) {
            _downlinkRSSI   = tdownlinkRSSI;
            _uplinkRSSI     = tuplinkRSSI;
            emit linkChanged();
        }
    //-- IP Address Settings?
    } else if(jSonData.contains("\"usbEthIp\":")) {
        QString value;
        bool changed = false;
        value = jObj["ipaddr"].toString(_localIPAddr);
        if(value != _localIPAddr) {
            _localIPAddr = value;
            changed = true;
            emit localIPAddrChanged();
        }
        value = jObj["netmask"].toString(_netMask);
        if(value != _netMask) {
            _netMask = value;
            changed = true;
            emit netMaskChanged();
        }
        value = jObj["usbEthIp"].toString(_remoteIPAddr);
        if(value != _remoteIPAddr) {
            _remoteIPAddr = value;
            changed = true;
            emit remoteIPAddrChanged();
        }
        if(changed) {
            QSettings settings;
            settings.beginGroup(kMICROHARD_GROUP);
            settings.setValue(kLOCAL_IP,     _localIPAddr);
            settings.setValue(kREMOTE_IP,    _remoteIPAddr);
            settings.setValue(kNET_MASK,     _netMask);
            settings.setValue(kCFG_PASSWORD, _configPassword);
            settings.endGroup();
        }
    }
}
