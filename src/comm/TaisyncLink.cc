/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include <QtGlobal>
#include <QDebug>

#include "TaisyncLink.h"
#include "QGC.h"
#include "QGCApplication.h"
#include "SettingsManager.h"
#include "VideoSettings.h"
#include "VideoManager.h"

#include "TaisyncTelemetry.h"
#include "TaisyncSettings.h"
#include "TaisyncVideoReceiver.h"

static const char* kEnableVideo     = "enableVideo";

//-----------------------------------------------------------------------------
TaisyncLink::TaisyncLink(SharedLinkConfigurationPointer& config)
    : LinkInterface(config)
    , _taiConfig(qobject_cast<TaisyncConfiguration*>(config.data()))
{
    if (!_taiConfig) {
        qWarning() << "Internal error";
    }
    moveToThread(this);
}

//-----------------------------------------------------------------------------
TaisyncLink::~TaisyncLink()
{
    _disconnect();
    // Tell the thread to exit
    _running = false;
    quit();
    // Wait for it to exit
    wait();
    this->deleteLater();
}

//-----------------------------------------------------------------------------
void
TaisyncLink::run()
{
    // Thread
    if(_hardwareConnect()) {
        exec();
    }
    _hardwareDisconnect();
}

//-----------------------------------------------------------------------------
void
TaisyncLink::_restartConnection()
{
    if(this->isConnected())
    {
        _disconnect();
        _connect();
    }
}

//-----------------------------------------------------------------------------
QString
TaisyncLink::getName() const
{
    return _taiConfig->name();
}

//-----------------------------------------------------------------------------
void
TaisyncLink::_writeBytes(const QByteArray data)
{
    if (_taiTelemetery) {
        _taiTelemetery->writeBytes(data);
        _logOutputDataRate(static_cast<quint64>(data.size()), QDateTime::currentMSecsSinceEpoch());
    }
}

//-----------------------------------------------------------------------------
void
TaisyncLink::_readBytes(QByteArray bytes)
{
    emit bytesReceived(this, bytes);
    _logInputDataRate(static_cast<quint64>(bytes.size()), QDateTime::currentMSecsSinceEpoch());
}

//-----------------------------------------------------------------------------
void
TaisyncLink::_disconnect()
{
    //-- Stop thread
    _running = false;
    quit();
    wait();
    //-- Kill Taisync handlers
    if (_taiTelemetery) {
        _hardwareDisconnect();
        emit disconnected();
    }
#if defined(__ios__) || defined(__android__)
    qgcApp()->toolbox()->videoManager()->setIsTaisync(false);
#endif
    //-- Restore video settings
    if(!_savedVideoSource.isNull()) {
        VideoSettings* pVSettings = qgcApp()->toolbox()->settingsManager()->videoSettings();
        pVSettings->videoSource()->setRawValue(_savedVideoSource);
        pVSettings->udpPort()->setRawValue(_savedVideoUDP);
        pVSettings->aspectRatio()->setRawValue(_savedAR);
        pVSettings->setVisible(_savedVideoState);
    }
}

//-----------------------------------------------------------------------------
bool
TaisyncLink::_connect(void)
{
    if(this->isRunning() || _running) {
        _running = false;
        quit();
        wait();
    }
    _running = true;
    if(_taiConfig->videoEnabled()) {
        //-- Hide video selection as we will be fixed to Taisync video and set the way we need it.
        VideoSettings* pVSettings = qgcApp()->toolbox()->settingsManager()->videoSettings();
        //-- First save current state
        _savedVideoSource = pVSettings->videoSource()->rawValue();
        _savedVideoUDP    = pVSettings->udpPort()->rawValue();
        _savedAR          = pVSettings->aspectRatio()->rawValue();
        _savedVideoState  = pVSettings->visible();
#if defined(__ios__) || defined(__android__)
        //-- iOS and Android receive raw h.264 and need a different pipeline
        qgcApp()->toolbox()->videoManager()->setIsTaisync(true);
#endif
        //-- Now set it up the way we need it do be
        pVSettings->setVisible(false);
        pVSettings->udpPort()->setRawValue(5600);
        pVSettings->aspectRatio()->setRawValue(1024.0 / 768.0);
        pVSettings->videoSource()->setRawValue(QString(VideoSettings::videoSourceUDP));
    }
    start(NormalPriority);
    return true;
}

//-----------------------------------------------------------------------------
void
TaisyncLink::_hardwareDisconnect()
{
    if (_taiTelemetery) {
        _taiTelemetery->close();
        _taiTelemetery->deleteLater();
        _taiTelemetery = nullptr;
    }
    if (_taiSettings) {
        _taiSettings->close();
        _taiSettings->deleteLater();
        _taiSettings = nullptr;
    }
#if defined(__ios__) || defined(__android__)
    if (_taiVideo) {
        _taiVideo->close();
        _taiVideo->deleteLater();
        _taiVideo = nullptr;
    }
#endif
    _connected = false;
}

//-----------------------------------------------------------------------------
bool
TaisyncLink::_hardwareConnect()
{
    _hardwareDisconnect();
    _taiTelemetery = new TaisyncTelemetry(this);
    QObject::connect(_taiTelemetery, &TaisyncTelemetry::bytesReady, this, &TaisyncLink::_readBytes);
    QObject::connect(_taiTelemetery, &TaisyncTelemetry::connected,  this, &TaisyncLink::_telemetryReady);
    _taiTelemetery->start();
    _taiSettings = new TaisyncSettings(this);
    _taiSettings->start();
    QObject::connect(_taiSettings,   &TaisyncSettings::connected,   this, &TaisyncLink::_settingsReady);
#if defined(__ios__) || defined(__android__)
    if(_taiConfig->videoEnabled()) {
        _taiVideo = new TaisyncVideoReceiver(this);
        _taiVideo->start();
    }
#endif
    return true;
}

//-----------------------------------------------------------------------------
bool
TaisyncLink::isConnected() const
{
    return _connected;
}

//-----------------------------------------------------------------------------
qint64
TaisyncLink::getConnectionSpeed() const
{
    return 57600; // 57.6 Kbit
}

//-----------------------------------------------------------------------------
qint64
TaisyncLink::getCurrentInDataRate() const
{
    return 0;
}

//-----------------------------------------------------------------------------
qint64
TaisyncLink::getCurrentOutDataRate() const
{
    return 0;
}

//-----------------------------------------------------------------------------
void
TaisyncLink::_telemetryReady()
{
    qCDebug(TaisyncLog) << "Taisync telemetry ready";
    if(!_connected) {
        _connected = true;
        emit connected();
    }
}

//-----------------------------------------------------------------------------
void
TaisyncLink::_settingsReady()
{
    qCDebug(TaisyncLog) << "Taisync settings ready";
    _taiSettings->requestSettings();
}

//--------------------------------------------------------------------------
//-- TaisyncConfiguration

//--------------------------------------------------------------------------
TaisyncConfiguration::TaisyncConfiguration(const QString& name) : LinkConfiguration(name)
{
}

//--------------------------------------------------------------------------
TaisyncConfiguration::TaisyncConfiguration(TaisyncConfiguration* source) : LinkConfiguration(source)
{
    _copyFrom(source);
}

//--------------------------------------------------------------------------
TaisyncConfiguration::~TaisyncConfiguration()
{
}

//--------------------------------------------------------------------------
void
TaisyncConfiguration::copyFrom(LinkConfiguration *source)
{
    LinkConfiguration::copyFrom(source);
    _copyFrom(source);
}

//--------------------------------------------------------------------------
void
TaisyncConfiguration::_copyFrom(LinkConfiguration *source)
{
    TaisyncConfiguration* usource = dynamic_cast<TaisyncConfiguration*>(source);
    if (usource) {
        _enableVideo = usource->videoEnabled();
    } else {
        qWarning() << "Internal error";
    }
}

//--------------------------------------------------------------------------
void
TaisyncConfiguration::setVideoEnabled(bool enable)
{
    _enableVideo = enable;
    emit enableVideoChanged();
}

//--------------------------------------------------------------------------
void
TaisyncConfiguration::saveSettings(QSettings& settings, const QString& root)
{
    settings.beginGroup(root);
    settings.setValue(kEnableVideo, _enableVideo);
    settings.endGroup();
}

//--------------------------------------------------------------------------
void
TaisyncConfiguration::loadSettings(QSettings& settings, const QString& root)
{
    settings.beginGroup(root);
    _enableVideo = settings.value(kEnableVideo, true).toBool();
    settings.endGroup();
}

//--------------------------------------------------------------------------
void
TaisyncConfiguration::updateSettings()
{
    if(_link) {
        TaisyncLink* ulink = dynamic_cast<TaisyncLink*>(_link);
        if(ulink) {
            ulink->_restartConnection();
        }
    }
}
