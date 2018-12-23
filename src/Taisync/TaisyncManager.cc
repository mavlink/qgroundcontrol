/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "TaisyncManager.h"
#include "TaisyncHandler.h"
#include "SettingsManager.h"
#include "QGCApplication.h"
#include "VideoManager.h"

static const char *kTAISYNC_GROUP       = "Taisync";
static const char *kRADIO_MODE          = "RadioMode";
static const char *kRADIO_CHANNEL       = "RadioChannel";
static const char *kVIDEO_OUTPUT        = "VideoOutput";
static const char *kVIDEO_MODE          = "VideoMode";
static const char *kVIDEO_RATE          = "VideoRate";

//-----------------------------------------------------------------------------
TaisyncManager::TaisyncManager(QGCApplication* app, QGCToolbox* toolbox)
    : QGCTool(app, toolbox)
{
    connect(&_workTimer, &QTimer::timeout, this, &TaisyncManager::_checkTaisync);
    _workTimer.setSingleShot(true);
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
    _taiSettings = new TaisyncSettings(this);
    connect(_taiSettings, &TaisyncSettings::updateSettings, this, &TaisyncManager::_updateSettings);
    connect(_taiSettings, &TaisyncSettings::connected,      this, &TaisyncManager::_connected);
    connect(_taiSettings, &TaisyncSettings::disconnected,   this, &TaisyncManager::_disconnected);
    if(!_appSettings) {
        _appSettings = _toolbox->settingsManager()->appSettings();
        connect(_appSettings->enableTaisync(),      &Fact::rawValueChanged, this, &TaisyncManager::_setEnabled);
        connect(_appSettings->enableTaisyncVideo(), &Fact::rawValueChanged, this, &TaisyncManager::_setVideoEnabled);
    }
    _setEnabled();
    if(_enabled) {
        _setVideoEnabled();
    }
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
        for(int i = 0; i < 13; i++) {
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
    _reset();
}

//-----------------------------------------------------------------------------
void
TaisyncManager::_radioSettingsChanged(QVariant)
{
    if(_taiSettings) {
        _taiSettings->setRadioSettings(
            _radioModeList[_radioMode->rawValue().toInt()],
            _radioChannel->enumStringValue());
    }
}

//-----------------------------------------------------------------------------
void
TaisyncManager::_videoSettingsChanged(QVariant)
{
    if(_taiSettings) {
        _taiSettings->setVideoSettings(
            _videoOutputList[_videoOutput->rawValue().toInt()],
            _videoMode->enumStringValue(),
            _videoRateList[_videoRate->rawValue().toInt()]);
    }
}

//-----------------------------------------------------------------------------
void
TaisyncManager::_setEnabled()
{
    bool enable = _appSettings->enableTaisync()->rawValue().toBool();
    if(enable) {
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
TaisyncManager::_setVideoEnabled()
{
    bool enable = _appSettings->enableTaisyncVideo()->rawValue().toBool();
    if(enable) {
        if(!_savedVideoSource.isValid()) {
            //-- Hide video selection as we will be fixed to Taisync video and set the way we need it.
            VideoSettings* pVSettings = qgcApp()->toolbox()->settingsManager()->videoSettings();
            //-- First save current state
            _savedVideoSource = pVSettings->videoSource()->rawValue();
            _savedVideoUDP    = pVSettings->udpPort()->rawValue();
            _savedAR          = pVSettings->aspectRatio()->rawValue();
            _savedVideoState  = pVSettings->visible();
            //-- Now set it up the way we need it do be
            pVSettings->setVisible(false);
            pVSettings->udpPort()->setRawValue(5600);
            pVSettings->aspectRatio()->setRawValue(1024.0 / 768.0);
            pVSettings->videoSource()->setRawValue(QString(VideoSettings::videoSourceUDP));
        }
#if defined(__ios__) || defined(__android__)
        if(!_taiVideo) {
            //-- iOS and Android receive raw h.264 and need a different pipeline
            qgcApp()->toolbox()->videoManager()->setIsTaisync(true);
            _taiVideo = new TaisyncVideoReceiver(this);
            _taiVideo->start();
        }
#endif
    } else {
        //-- Restore video settings
#if defined(__ios__) || defined(__android__)
        qgcApp()->toolbox()->videoManager()->setIsTaisync(false);
        if (_taiVideo) {
            _taiVideo->close();
            _taiVideo->deleteLater();
            _taiVideo = nullptr;
        }
#endif
        if(!_savedVideoSource.isValid()) {
            VideoSettings* pVSettings = qgcApp()->toolbox()->settingsManager()->videoSettings();
            pVSettings->videoSource()->setRawValue(_savedVideoSource);
            pVSettings->udpPort()->setRawValue(_savedVideoUDP);
            pVSettings->aspectRatio()->setRawValue(_savedAR);
            pVSettings->setVisible(_savedVideoState);
            _savedVideoSource.clear();
        }
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
}

//-----------------------------------------------------------------------------
void
TaisyncManager::_disconnected()
{
    qCDebug(TaisyncLog) << "Taisync Settings Disconnected";
    _isConnected = false;
    emit connectedChanged();
    _reset();
}

//-----------------------------------------------------------------------------
void
TaisyncManager::_checkTaisync()
{
    if(_enabled) {
        if(!_isConnected) {
            if(!_taiSettings->isServerRunning()) {
                _taiSettings->start();
            }
        } else {
            if(++_currReq >= REQ_LAST) {
                _currReq = REQ_LINK_STATUS;
            }
            switch(_currReq) {
            case REQ_LINK_STATUS:
                _taiSettings->requestLinkStatus();
                break;
            case REQ_DEV_INFO:
                _taiSettings->requestDevInfo();
                break;
            case REQ_FREQ_SCAN:
                _taiSettings->requestFreqScan();
                break;
            case REQ_VIDEO_SETTINGS:
                _taiSettings->requestVideoSettings();
                break;
            case REQ_RADIO_SETTINGS:
                _taiSettings->requestRadioSettings();
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
        bool tlinkConnected  = jObj["flight"].toString("") == "online";
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
        QString tfwVersion      = jObj["firmwareversion"].toString(_fwVersion);
        QString tserialNumber   = jObj["sn"].toString(_serialNumber);
        if(tfwVersion != _fwVersion || tserialNumber != _serialNumber) {
            _fwVersion      = tfwVersion;
            _serialNumber   = tserialNumber;
            emit infoChanged();
        }
    //-- Radio Settings?
    } else if(jSonData.contains("\"freq\":")) {
        int idx = _radioModeList.indexOf(jObj["mode"].toString(_radioMode->enumStringValue()));
        if(idx >= 0) _radioMode->_containerSetRawValue(idx);
        idx = _radioChannel->valueIndex(jObj["freq"].toString(_radioChannel->enumStringValue()));
        if(idx < 0) idx = 0;
        _radioChannel->_containerSetRawValue(idx);
    //-- Video Settings?
    } else if(jSonData.contains("\"maxbitrate\":")) {
        int idx;
        idx = _videoMode->valueIndex(jObj["mode"].toString(_videoMode->enumStringValue()));
        if(idx < 0) idx = 0;
        _videoMode->_containerSetRawValue(idx);
        idx = _videoRateList.indexOf(jObj["maxbitrate"].toString(_videoMode->enumStringValue()));
        if(idx >= 0) _videoRate->_containerSetRawValue(idx);
        idx = _videoOutputList.indexOf(jObj["decode"].toString(_videoOutput->enumStringValue()));
        if(idx >= 0) _videoOutput->_containerSetRawValue(idx);
    }
}
