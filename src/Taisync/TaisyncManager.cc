/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "TaisyncManager.h"
#include "TaisyncHandler.h"
#include "SettingsManager.h"
#include "QGCApplication.h"
#include "QGCCorePlugin.h"
#include "VideoManager.h"

#include <QSettings>

static const char *kTAISYNC_GROUP       = "Taisync";
static const char *kRADIO_MODE          = "RadioMode";
static const char *kRADIO_CHANNEL       = "RadioChannel";
static const char *kVIDEO_OUTPUT        = "VideoOutput";
static const char *kVIDEO_MODE          = "VideoMode";
static const char *kVIDEO_RATE          = "VideoRate";
static const char *kLOCAL_IP            = "LocalIP";
static const char *kREMOTE_IP           = "RemoteIP";
static const char *kNET_MASK            = "NetMask";
static const char *kRTSP_URI            = "RTSPURI";
static const char *kRTSP_ACCOUNT        = "RTSPAccount";
static const char *kRTSP_PASSWORD       = "RTSPPassword";

//-----------------------------------------------------------------------------
TaisyncManager::TaisyncManager(QGCApplication* app, QGCToolbox* toolbox)
    : QGCTool(app, toolbox)
{
    connect(&_workTimer, &QTimer::timeout, this, &TaisyncManager::_checkTaisync);
    _workTimer.setSingleShot(true);
    QSettings settings;
    settings.beginGroup(kTAISYNC_GROUP);
    _localIPAddr  = settings.value(kLOCAL_IP,       QString("192.168.199.33")).toString();
    _remoteIPAddr = settings.value(kREMOTE_IP,      QString("192.168.199.16")).toString();
    _netMask      = settings.value(kNET_MASK,       QString("255.255.255.0")).toString();
    _rtspURI      = settings.value(kRTSP_URI,       QString("rtsp://192.168.0.2")).toString();
    _rtspAccount  = settings.value(kRTSP_ACCOUNT,   QString("admin")).toString();
    _rtspPassword = settings.value(kRTSP_PASSWORD,  QString("12345678")).toString();
    settings.endGroup();
}

//-----------------------------------------------------------------------------
TaisyncManager::~TaisyncManager()
{
    _close();
}

//-----------------------------------------------------------------------------
void
TaisyncManager::_close()
{
    if(_taiSettings) {
        _taiSettings->close();
        _taiSettings->deleteLater();
        _taiSettings = nullptr;
    }
#if defined(__ios__) || defined(__android__)
    if (_taiTelemetery) {
        _taiTelemetery->close();
        _taiTelemetery->deleteLater();
        _taiTelemetery = nullptr;
    }
    if(_telemetrySocket) {
        _telemetrySocket->close();
        _telemetrySocket->deleteLater();
        _telemetrySocket = nullptr;
    }
    if (_taiVideo) {
        _taiVideo->close();
        _taiVideo->deleteLater();
        _taiVideo = nullptr;
    }
#endif
}

//-----------------------------------------------------------------------------
void
TaisyncManager::_reset()
{
    _close();
    _isConnected = false;
    emit connectedChanged();
    _linkConnected = false;
    emit linkConnectedChanged();
    if(!_appSettings) {
        _appSettings = _toolbox->settingsManager()->appSettings();
        connect(_appSettings->enableTaisync(),      &Fact::rawValueChanged, this, &TaisyncManager::_setEnabled);
        connect(_appSettings->enableTaisyncVideo(), &Fact::rawValueChanged, this, &TaisyncManager::_setVideoEnabled);
    }
    _setEnabled();
}

//-----------------------------------------------------------------------------
FactMetaData*
TaisyncManager::_createMetadata(const char* name, QStringList enums)
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
TaisyncManager::setToolbox(QGCToolbox* toolbox)
{
    QGCTool::setToolbox(toolbox);
    {
        //-- Radio Mode
        QStringList enums;
        enums.append(tr("Auto"));
        enums.append(tr("Manual"));
        FactMetaData* metaData = _createMetadata(kRADIO_MODE, enums);
        _radioMode = new Fact(kTAISYNC_GROUP, metaData, this);
        QQmlEngine::setObjectOwnership(_radioMode, QQmlEngine::CppOwnership);
        _radioModeList.append("auto");
        _radioModeList.append("manual");
        connect(_radioMode, &Fact::_containerRawValueChanged, this, &TaisyncManager::_radioSettingsChanged);
    }
    {
        //-- Radio Channel
        QStringList enums;
        for(int i = 0; i < 14; i++) {
            enums.append(QString("ch%1").arg(i));
        }
        FactMetaData* metaData = _createMetadata(kRADIO_CHANNEL, enums);
        _radioChannel = new Fact(kTAISYNC_GROUP, metaData, this);
        QQmlEngine::setObjectOwnership(_radioChannel, QQmlEngine::CppOwnership);
        connect(_radioChannel, &Fact::_containerRawValueChanged, this, &TaisyncManager::_radioSettingsChanged);
    }
    {
        //-- Video Output
        QStringList enums;
        enums.append(tr("Stream"));
        enums.append(tr("HDMI Port"));
        FactMetaData* metaData = _createMetadata(kVIDEO_OUTPUT, enums);
        _videoOutput = new Fact(kTAISYNC_GROUP, metaData, this);
        QQmlEngine::setObjectOwnership(_videoOutput, QQmlEngine::CppOwnership);
        _videoOutputList.append("phone");
        _videoOutputList.append("hdmi");
        connect(_videoOutput, &Fact::_containerRawValueChanged, this, &TaisyncManager::_videoSettingsChanged);
    }
    {
        //-- Video Mode
        QStringList enums;
        enums.append("H264");
        enums.append("H265");
        FactMetaData* metaData = _createMetadata(kVIDEO_MODE, enums);
        _videoMode = new Fact(kTAISYNC_GROUP, metaData, this);
        QQmlEngine::setObjectOwnership(_videoMode, QQmlEngine::CppOwnership);
        connect(_videoMode, &Fact::_containerRawValueChanged, this, &TaisyncManager::_videoSettingsChanged);
    }
    {
        //-- Video Rate
        QStringList enums;
        enums.append(tr("Low"));
        enums.append(tr("Medium"));
        enums.append(tr("High"));
        FactMetaData* metaData = _createMetadata(kVIDEO_RATE, enums);
        _videoRate = new Fact(kTAISYNC_GROUP, metaData, this);
        QQmlEngine::setObjectOwnership(_videoRate, QQmlEngine::CppOwnership);
        _videoRateList.append("low");
        _videoRateList.append("middle");
        _videoRateList.append("high");
        connect(_videoRate, &Fact::_containerRawValueChanged, this, &TaisyncManager::_videoSettingsChanged);
    }
    //-- Start it all
    _reset();
}

//-----------------------------------------------------------------------------
bool
TaisyncManager::setRTSPSettings(QString uri, QString account, QString password)
{
    if(_taiSettings && _isConnected) {
        if(_taiSettings->setRTSPSettings(uri, account, password)) {
            _rtspURI      = uri;
            _rtspAccount  = account;
            _rtspPassword = password;
            QSettings settings;
            settings.beginGroup(kTAISYNC_GROUP);
            settings.setValue(kRTSP_URI,      _rtspURI);
            settings.setValue(kRTSP_ACCOUNT,  _rtspAccount);
            settings.setValue(kRTSP_PASSWORD, _rtspPassword);
            settings.endGroup();
            emit rtspURIChanged();
            emit rtspAccountChanged();
            emit rtspPasswordChanged();
            _needReboot = true;
            emit needRebootChanged();
            return true;
        }
    }
    return false;
}

//-----------------------------------------------------------------------------
bool
TaisyncManager::setIPSettings(QString localIP_, QString remoteIP_, QString netMask_)
{
    bool res = false;
    if(_localIPAddr != localIP_ || _remoteIPAddr != remoteIP_ || _netMask != netMask_) {
        //-- If we are connected to the Taisync
        if(_isConnected) {
            if(_taiSettings) {
                //-- Change IP settings
                res = _taiSettings->setIPSettings(localIP_, remoteIP_, netMask_);
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
            settings.beginGroup(kTAISYNC_GROUP);
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
TaisyncManager::_radioSettingsChanged(QVariant)
{
    if(_taiSettings && _isConnected) {
        _workTimer.stop();
        _taiSettings->setRadioSettings(
            _radioModeList[_radioMode->rawValue().toInt()],
            _radioChannel->enumStringValue());
        _reqMask |= REQ_RADIO_SETTINGS;
        _workTimer.start(3000);
    }
}

//-----------------------------------------------------------------------------
void
TaisyncManager::_videoSettingsChanged(QVariant)
{
    if(_taiSettings && _isConnected) {
        _workTimer.stop();
        _taiSettings->setVideoSettings(
            _videoOutputList[_videoOutput->rawValue().toInt()],
            _videoMode->enumStringValue(),
            _videoRateList[_videoRate->rawValue().toInt()]);
        _reqMask |= REQ_VIDEO_SETTINGS;
        _workTimer.start(500);
    }
}

//-----------------------------------------------------------------------------
void
TaisyncManager::_setEnabled()
{
    bool enable = _appSettings->enableTaisync()->rawValue().toBool();
    if(enable) {
        if(!_taiSettings) {
            _taiSettings = new TaisyncSettings(this);
            connect(_taiSettings, &TaisyncSettings::updateSettings, this, &TaisyncManager::_updateSettings);
            connect(_taiSettings, &TaisyncSettings::connected,      this, &TaisyncManager::_connected);
            connect(_taiSettings, &TaisyncSettings::disconnected,   this, &TaisyncManager::_disconnected);
        }
#if defined(__ios__) || defined(__android__)
        if(!_taiTelemetery) {
            _taiTelemetery = new TaisyncTelemetry(this);
            QObject::connect(_taiTelemetery, &TaisyncTelemetry::bytesReady, this, &TaisyncManager::_readTelemBytes);
            _telemetrySocket = new QUdpSocket(this);
            _telemetrySocket->setSocketOption(QAbstractSocket::SendBufferSizeSocketOption,    64 * 1024);
            _telemetrySocket->setSocketOption(QAbstractSocket::ReceiveBufferSizeSocketOption, 64 * 1024);
            QObject::connect(_telemetrySocket, &QUdpSocket::readyRead, this, &TaisyncManager::_readUDPBytes);
            _telemetrySocket->bind(QHostAddress::LocalHost, 0, QUdpSocket::ShareAddress);
            _taiTelemetery->start();
        }
#endif
        _reqMask = static_cast<uint32_t>(REQ_ALL);
        _workTimer.start(1000);
    } else {
        //-- Stop everything
        _workTimer.stop();
        _close();
    }
    _enabled = enable;
    //-- Now handle video support
    _setVideoEnabled();
}

//-----------------------------------------------------------------------------
void
TaisyncManager::_restoreVideoSettings(Fact* setting)
{
    auto* pFact = qobject_cast<SettingsFact*>(setting);
    if(pFact) {
        pFact->setVisible(qgcApp()->toolbox()->corePlugin()->adjustSettingMetaData(VideoSettings::settingsGroup, *setting->metaData()));
    }
}

//-----------------------------------------------------------------------------
void
TaisyncManager::_setVideoEnabled()
{
    //-- Check both if video is enabled and Taisync support itself is enabled as well.
    bool enable = _appSettings->enableTaisyncVideo()->rawValue().toBool() && _appSettings->enableTaisync()->rawValue().toBool();
    if(enable) {
        //-- Set it up the way we need it do be.
        VideoSettings* pVSettings = qgcApp()->toolbox()->settingsManager()->videoSettings();
        pVSettings->setVisible(false);
        pVSettings->udpPort()->setRawValue(5600);
        //-- TODO: this AR must come from somewhere
        pVSettings->aspectRatio()->setRawValue(1024.0 / 768.0);
        pVSettings->videoSource()->setRawValue(QString(VideoSettings::videoSourceUDPH264));
#if defined(__ios__) || defined(__android__)
        if(!_taiVideo) {
            //-- iOS and Android receive raw h.264 and need a different pipeline
            qgcApp()->toolbox()->videoManager()->setIsTaisync(true);
            _taiVideo = new TaisyncVideoReceiver(this);
            _taiVideo->start();
        }
#endif
    } else {
        //-- Restore video settings.
#if defined(__ios__) || defined(__android__)
        qgcApp()->toolbox()->videoManager()->setIsTaisync(false);
        if (_taiVideo) {
            _taiVideo->close();
            _taiVideo->deleteLater();
            _taiVideo = nullptr;
        }
#endif
        VideoSettings* pVSettings = qgcApp()->toolbox()->settingsManager()->videoSettings();
        _restoreVideoSettings(pVSettings->videoSource());
        _restoreVideoSettings(pVSettings->aspectRatio());
        _restoreVideoSettings(pVSettings->udpPort());
        pVSettings->setVisible(true);
    }
    _enableVideo = enable;
}

//-----------------------------------------------------------------------------
#if defined(__ios__) || defined(__android__)
void
TaisyncManager::_readTelemBytes(QByteArray bytesIn)
{
    //-- Send telemetry from vehicle to QGC (using normal UDP)
    _telemetrySocket->writeDatagram(bytesIn, QHostAddress::LocalHost, TAISYNC_TELEM_TARGET_PORT);
}
#endif

//-----------------------------------------------------------------------------
#if defined(__ios__) || defined(__android__)
void
TaisyncManager::_readUDPBytes()
{
    if (!_telemetrySocket || !_taiTelemetery) {
        return;
    }
    //-- Read UDP data from QGC
    while (_telemetrySocket->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(static_cast<int>(_telemetrySocket->pendingDatagramSize()));
        _telemetrySocket->readDatagram(datagram.data(), datagram.size());
        //-- Send it to vehicle
        _taiTelemetery->writeBytes(datagram);
    }
}
#endif

//-----------------------------------------------------------------------------
void
TaisyncManager::_connected()
{
    qCDebug(TaisyncLog) << "Taisync Settings Connected";
    _isConnected = true;
    emit connectedChanged();
    _needReboot = false;
    emit needRebootChanged();
}

//-----------------------------------------------------------------------------
void
TaisyncManager::_disconnected()
{
    qCDebug(TaisyncLog) << "Taisync Settings Disconnected";
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
TaisyncManager::_checkTaisync()
{
    if(_enabled) {
        if(!_isConnected) {
            if(_taiSettings) {
                if(!_taiSettings->isServerRunning()) {
                    _taiSettings->start();
                }
            }
        } else {
            //qCDebug(TaisyncVerbose) << bin << _reqMask;
            while(true) {
                if (_reqMask & REQ_LINK_STATUS) {
                    _taiSettings->requestLinkStatus();
                    break;
                }
                if (_reqMask & REQ_DEV_INFO) {
                    _taiSettings->requestDevInfo();
                    break;
                }
                if (_reqMask & REQ_FREQ_SCAN) {
                    _reqMask &= ~static_cast<uint32_t>(REQ_FREQ_SCAN);
                    _taiSettings->requestFreqScan();
                    break;
                }
                if (_reqMask & REQ_VIDEO_SETTINGS) {
                    _taiSettings->requestVideoSettings();
                    break;
                }
                if (_reqMask & REQ_RADIO_SETTINGS) {
                    _taiSettings->requestRadioSettings();
                    break;
                }
                if (_reqMask & REQ_RTSP_SETTINGS) {
                    _reqMask &= ~static_cast<uint32_t>(REQ_RTSP_SETTINGS);
                    _taiSettings->requestRTSPURISettings();
                    break;
                }
                if (_reqMask & REQ_IP_SETTINGS) {
                    _reqMask &= ~static_cast<uint32_t>(REQ_IP_SETTINGS);
                    _taiSettings->requestIPSettings();
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
                    _taiSettings->requestLinkStatus();
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
TaisyncManager::_updateSettings(QByteArray jSonData)
{
    _timeoutTimer.start();
    qCDebug(TaisyncVerbose) << jSonData;
    QJsonParseError jsonParseError;
    QJsonDocument doc = QJsonDocument::fromJson(jSonData, &jsonParseError);
    if (jsonParseError.error != QJsonParseError::NoError) {
        qWarning() <<  "Unable to parse Taisync response:" << jsonParseError.errorString() << jsonParseError.offset;
        return;
    }
    QJsonObject jObj = doc.object();
    //-- Link Status?
    if(jSonData.contains("\"flight\":")) {
        _reqMask &= ~static_cast<uint32_t>(REQ_LINK_STATUS);
        bool tlinkConnected  = jObj["flight"].toString() == "online";
        if(tlinkConnected != _linkConnected) {
           _linkConnected = tlinkConnected;
           emit linkConnectedChanged();
        }
        QString tlinkVidFormat  = jObj["videoformat"].toString(_linkVidFormat);
        int     tdownlinkRSSI   = jObj["radiorssi"].toInt(_downlinkRSSI);
        int     tuplinkRSSI     = jObj["hdrssi"].toInt(_uplinkRSSI);
        if(_linkVidFormat != tlinkVidFormat || _downlinkRSSI != tdownlinkRSSI || _uplinkRSSI != tuplinkRSSI) {
            _linkVidFormat  = tlinkVidFormat;
            _downlinkRSSI   = tdownlinkRSSI;
            _uplinkRSSI     = tuplinkRSSI;
            emit linkChanged();
        }
    //-- Device Info?
    } else if(jSonData.contains("\"firmwareversion\":")) {
        _reqMask &= ~static_cast<uint32_t>(REQ_DEV_INFO);
        QString tfwVersion      = jObj["firmwareversion"].toString(_fwVersion);
        QString tserialNumber   = jObj["sn"].toString(_serialNumber);
        if(tfwVersion != _fwVersion || tserialNumber != _serialNumber) {
            _fwVersion      = tfwVersion;
            _serialNumber   = tserialNumber;
            emit infoChanged();
        }
    //-- Radio Settings?
    } else if(jSonData.contains("\"freq\":")) {
        _reqMask &= ~static_cast<uint32_t>(REQ_RADIO_SETTINGS);
        int idx = _radioModeList.indexOf(jObj["mode"].toString(_radioMode->enumStringValue()));
        if(idx >= 0) _radioMode->_containerSetRawValue(idx);
        idx = _radioChannel->valueIndex(jObj["freq"].toString(_radioChannel->enumStringValue()));
        if(idx < 0) idx = 0;
        _radioChannel->_containerSetRawValue(idx);
    //-- Video Settings?
    } else if(jSonData.contains("\"maxbitrate\":")) {
        _reqMask &= ~static_cast<uint32_t>(REQ_VIDEO_SETTINGS);
        int idx;
        idx = _videoMode->valueIndex(jObj["mode"].toString(_videoMode->enumStringValue()));
        if(idx < 0) idx = 0;
        _videoMode->_containerSetRawValue(idx);
        idx = _videoRateList.indexOf(jObj["maxbitrate"].toString(_videoMode->enumStringValue()));
        if(idx >= 0) _videoRate->_containerSetRawValue(idx);
        idx = _videoOutputList.indexOf(jObj["decode"].toString(_videoOutput->enumStringValue()));
        if(idx >= 0) _videoOutput->_containerSetRawValue(idx);
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
            settings.beginGroup(kTAISYNC_GROUP);
            settings.setValue(kLOCAL_IP,  _localIPAddr);
            settings.setValue(kREMOTE_IP, _remoteIPAddr);
            settings.setValue(kNET_MASK,  _netMask);
            settings.endGroup();
        }
    //-- RTSP URI Settings?
    } else if(jSonData.contains("\"rtspURI\":")) {
        QString value;
        bool changed = false;
        value = jObj["rtspURI"].toString(_rtspURI);
        if(value != _rtspURI) {
            _rtspURI = value;
            changed = true;
            emit rtspURIChanged();
        }
        value = jObj["account"].toString(_rtspAccount);
        if(value != _rtspAccount) {
            _rtspAccount = value;
            changed = true;
            emit rtspAccountChanged();
        }
        value = jObj["passwd"].toString(_rtspPassword);
        if(value != _rtspPassword) {
            _rtspPassword = value;
            changed = true;
            emit rtspPasswordChanged();
        }
        if(changed) {
            QSettings settings;
            settings.beginGroup(kTAISYNC_GROUP);
            settings.setValue(kRTSP_URI,      _rtspURI);
            settings.setValue(kRTSP_ACCOUNT,  _rtspAccount);
            settings.setValue(kRTSP_PASSWORD, _rtspPassword);
            settings.endGroup();
        }
    }
}
