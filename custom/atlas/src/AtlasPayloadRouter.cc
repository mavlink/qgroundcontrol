#include "AtlasPayloadRouter.h"

#include "AtlasJetsonClient.h"
#include "AtlasSiyiClient.h"
#include "Fact.h"
#include "SettingsManager.h"
#include "VideoSettings.h"
#include "VideoManager.h"

#include <QtCore/QDateTime>
#include <QtCore/QSettings>

namespace {
constexpr const char *kSettingsGroup = "ATLAS/Payload";
constexpr const char *kModeDirectA8 = "DIRECT_A8";
constexpr const char *kModeJetson = "JETSON";
}

AtlasPayloadRouter::AtlasPayloadRouter(AtlasJetsonClient *jetsonClient,
                                       AtlasSiyiClient *siyiClient,
                                       QObject *parent)
    : QObject(parent)
    , _jetson(jetsonClient)
    , _siyi(siyiClient)
{
    Q_ASSERT(_jetson);
    Q_ASSERT(_siyi);

    _loadSettings();

    const auto relayStatus = [this]() { emit statusChanged(); };
    connect(_jetson, &AtlasJetsonClient::connectedChanged, this, relayStatus);
    connect(_jetson, &AtlasJetsonClient::hm30ConnectedChanged, this, relayStatus);
    connect(_jetson, &AtlasJetsonClient::a8ConnectedChanged, this, relayStatus);
    connect(_jetson, &AtlasJetsonClient::statusChanged, this, relayStatus);
    connect(_jetson, &AtlasJetsonClient::eventReceived, this, &AtlasPayloadRouter::eventReceived);

    connect(_siyi, &AtlasSiyiClient::connectedChanged, this, relayStatus);
    connect(_siyi, &AtlasSiyiClient::statusChanged, this, relayStatus);
    connect(_siyi, &AtlasSiyiClient::eventReceived, this, &AtlasPayloadRouter::eventReceived);
}

QString AtlasPayloadRouter::mode() const
{
    return directMode() ? QString::fromLatin1(kModeDirectA8) : QString::fromLatin1(kModeJetson);
}

QString AtlasPayloadRouter::modeDisplayName() const
{
    return directMode() ? QStringLiteral("DIRECT A8") : QStringLiteral("JETSON");
}

bool AtlasPayloadRouter::directMode() const { return _mode == Mode::DirectA8; }
bool AtlasPayloadRouter::jetsonMode() const { return _mode == Mode::Jetson; }

QString AtlasPayloadRouter::activeControlHost() const { return directMode() ? _directA8Host : _jetsonHost; }
QString AtlasPayloadRouter::activeRtspUrl() const { return directMode() ? _directRtspUrl : _jetsonRtspUrl; }

bool AtlasPayloadRouter::connected() const { return directMode() ? _siyi->connected() : _jetson->connected(); }
bool AtlasPayloadRouter::hm30Connected() const { return directMode() ? _siyi->connected() : _jetson->hm30Connected(); }
bool AtlasPayloadRouter::a8Connected() const { return directMode() ? _siyi->connected() : _jetson->a8Connected(); }
bool AtlasPayloadRouter::directA8Connected() const { return _siyi->connected(); }
bool AtlasPayloadRouter::jetsonConnected() const { return _jetson->connected(); }

int AtlasPayloadRouter::rttMs() const { return jetsonMode() ? _jetson->rttMs() : -1; }

double AtlasPayloadRouter::jetsonCpuPercent() const { return jetsonMode() ? _jetson->jetsonCpuPercent() : 0.0; }
double AtlasPayloadRouter::jetsonGpuPercent() const { return jetsonMode() ? _jetson->jetsonGpuPercent() : 0.0; }
double AtlasPayloadRouter::jetsonRamUsedGB() const { return jetsonMode() ? _jetson->jetsonRamUsedGB() : 0.0; }
double AtlasPayloadRouter::jetsonRamTotalGB() const { return jetsonMode() ? _jetson->jetsonRamTotalGB() : 0.0; }
double AtlasPayloadRouter::jetsonTempC() const { return jetsonMode() ? _jetson->jetsonTempC() : 0.0; }
double AtlasPayloadRouter::jetsonPowerW() const { return jetsonMode() ? _jetson->jetsonPowerW() : 0.0; }
double AtlasPayloadRouter::jetsonSwapGB() const { return jetsonMode() ? _jetson->jetsonSwapGB() : 0.0; }
bool AtlasPayloadRouter::jetsonThrottled() const { return jetsonMode() && _jetson->jetsonThrottled(); }

double AtlasPayloadRouter::hm30SignalPercent() const { return jetsonMode() ? _jetson->hm30SignalPercent() : 0.0; }
double AtlasPayloadRouter::hm30RssiDbm() const { return jetsonMode() ? _jetson->hm30RssiDbm() : 0.0; }
double AtlasPayloadRouter::hm30PacketLossPercent() const { return jetsonMode() ? _jetson->hm30PacketLossPercent() : 0.0; }
double AtlasPayloadRouter::hm30LatencyMs() const { return jetsonMode() ? _jetson->hm30LatencyMs() : 0.0; }
double AtlasPayloadRouter::hm30UpMbps() const { return jetsonMode() ? _jetson->hm30UpMbps() : 0.0; }
double AtlasPayloadRouter::hm30DownMbps() const { return jetsonMode() ? _jetson->hm30DownMbps() : 0.0; }

bool AtlasPayloadRouter::recording() const { return directMode() ? _siyi->recording() : _jetson->recording(); }
double AtlasPayloadRouter::zoomLevel() const { return directMode() ? _siyi->zoomLevel() : _jetson->zoomLevel(); }
double AtlasPayloadRouter::gimbalYawDeg() const { return directMode() ? _siyi->gimbalYawDeg() : _jetson->gimbalYawDeg(); }
double AtlasPayloadRouter::gimbalPitchDeg() const { return directMode() ? _siyi->gimbalPitchDeg() : _jetson->gimbalPitchDeg(); }

bool AtlasPayloadRouter::a8MainStreamInfoValid() const { return _siyi->mainStreamInfoValid(); }
QString AtlasPayloadRouter::a8MainStreamCodec() const { return _siyi->mainStreamCodec(); }
int AtlasPayloadRouter::a8MainStreamWidth() const { return _siyi->mainStreamWidth(); }
int AtlasPayloadRouter::a8MainStreamHeight() const { return _siyi->mainStreamHeight(); }
int AtlasPayloadRouter::a8MainStreamBitrateKbps() const { return _siyi->mainStreamBitrateKbps(); }
int AtlasPayloadRouter::a8MainStreamFps() const { return _siyi->mainStreamFps(); }
bool AtlasPayloadRouter::a8RecordStreamInfoValid() const { return _siyi->recordStreamInfoValid(); }
QString AtlasPayloadRouter::a8RecordStreamCodec() const { return _siyi->recordStreamCodec(); }
int AtlasPayloadRouter::a8RecordStreamWidth() const { return _siyi->recordStreamWidth(); }
int AtlasPayloadRouter::a8RecordStreamHeight() const { return _siyi->recordStreamHeight(); }
int AtlasPayloadRouter::a8RecordStreamBitrateKbps() const { return _siyi->recordStreamBitrateKbps(); }
int AtlasPayloadRouter::a8RecordStreamFps() const { return _siyi->recordStreamFps(); }

bool AtlasPayloadRouter::aiEnabled() const { return jetsonMode() && _jetson->aiEnabled(); }
bool AtlasPayloadRouter::sahiEnabled() const { return jetsonMode() && _jetson->sahiEnabled(); }
bool AtlasPayloadRouter::depthEnabled() const { return jetsonMode() && _jetson->depthEnabled(); }
bool AtlasPayloadRouter::tracking() const { return jetsonMode() && _jetson->tracking(); }
bool AtlasPayloadRouter::trackingModeEnabled() const { return jetsonMode() && _jetson->trackingModeEnabled(); }
bool AtlasPayloadRouter::autoGimbal() const { return jetsonMode() && _jetson->autoGimbal(); }

QString AtlasPayloadRouter::aiModel() const { return jetsonMode() ? _jetson->aiModel() : QStringLiteral("N/A"); }
QString AtlasPayloadRouter::performanceProfile() const { return jetsonMode() ? _jetson->performanceProfile() : QStringLiteral("DIRECT"); }
QString AtlasPayloadRouter::missionProfile() const { return jetsonMode() ? _jetson->missionProfile() : QStringLiteral("DIRECT"); }
QString AtlasPayloadRouter::streamMode() const { return jetsonMode() ? _jetson->streamMode() : QStringLiteral("RAW"); }
QString AtlasPayloadRouter::videoProfile() const { return jetsonMode() ? _jetson->videoProfile() : QStringLiteral("DIRECT"); }
QStringList AtlasPayloadRouter::aiClasses() const { return jetsonMode() ? _jetson->aiClasses() : QStringList(); }

double AtlasPayloadRouter::confidence() const { return jetsonMode() ? _jetson->confidence() : 0.0; }
int AtlasPayloadRouter::aiFpsTarget() const { return jetsonMode() ? _jetson->aiFpsTarget() : 0; }
int AtlasPayloadRouter::aiResolution() const { return jetsonMode() ? _jetson->aiResolution() : 0; }
int AtlasPayloadRouter::sahiSlice() const { return jetsonMode() ? _jetson->sahiSlice() : 0; }
double AtlasPayloadRouter::aiFps() const { return jetsonMode() ? _jetson->aiFps() : 0.0; }
double AtlasPayloadRouter::videoFps() const { return jetsonMode() ? _jetson->videoFps() : 0.0; }
double AtlasPayloadRouter::videoBitrateMbps() const { return jetsonMode() ? _jetson->videoBitrateMbps() : 0.0; }

bool AtlasPayloadRouter::targetValid() const { return jetsonMode() && _jetson->targetValid(); }
int AtlasPayloadRouter::targetId() const { return jetsonMode() ? _jetson->targetId() : -1; }
QString AtlasPayloadRouter::targetClass() const { return jetsonMode() ? _jetson->targetClass() : QString(); }
double AtlasPayloadRouter::targetConfidence() const { return jetsonMode() ? _jetson->targetConfidence() : 0.0; }
double AtlasPayloadRouter::targetDistanceM() const { return jetsonMode() ? _jetson->targetDistanceM() : 0.0; }
double AtlasPayloadRouter::targetBearingDeg() const { return jetsonMode() ? _jetson->targetBearingDeg() : 0.0; }
double AtlasPayloadRouter::targetLatitude() const { return jetsonMode() ? _jetson->targetLatitude() : 0.0; }
double AtlasPayloadRouter::targetLongitude() const { return jetsonMode() ? _jetson->targetLongitude() : 0.0; }

void AtlasPayloadRouter::_loadSettings()
{
    QSettings settings;
    settings.beginGroup(QString::fromLatin1(kSettingsGroup));

    const QString savedMode = settings.value(QStringLiteral("mode"), QString::fromLatin1(kModeJetson)).toString().trimmed().toUpper();
    _mode = (savedMode == QString::fromLatin1(kModeDirectA8)) ? Mode::DirectA8 : Mode::Jetson;

    _directA8Host = settings.value(QStringLiteral("directA8Host"), _directA8Host).toString();
    _directA8Port = settings.value(QStringLiteral("directA8Port"), _directA8Port).toInt();
    _directRtspUrl = settings.value(QStringLiteral("directRtspUrl"), _directRtspUrl).toString();

    _jetsonHost = settings.value(QStringLiteral("jetsonHost"), _jetsonHost).toString();
    _jetsonPort = settings.value(QStringLiteral("jetsonPort"), _jetsonPort).toInt();
    _jetsonRtspUrl = settings.value(QStringLiteral("jetsonRtspUrl"), QString()).toString();

    settings.endGroup();
}

void AtlasPayloadRouter::_saveSettings() const
{
    QSettings settings;
    settings.beginGroup(QString::fromLatin1(kSettingsGroup));
    settings.setValue(QStringLiteral("mode"), mode());
    settings.setValue(QStringLiteral("directA8Host"), _directA8Host);
    settings.setValue(QStringLiteral("directA8Port"), _directA8Port);
    settings.setValue(QStringLiteral("directRtspUrl"), _directRtspUrl);
    settings.setValue(QStringLiteral("jetsonHost"), _jetsonHost);
    settings.setValue(QStringLiteral("jetsonPort"), _jetsonPort);
    settings.setValue(QStringLiteral("jetsonRtspUrl"), _jetsonRtspUrl);
    settings.endGroup();
}

void AtlasPayloadRouter::start()
{
    if (_started) {
        return;
    }
    _started = true;

    // v0.9 stored video in QGC's global Video setting. Preserve it as the
    // initial Jetson RTSP profile instead of inventing a Jetson address.
    if (_jetsonRtspUrl.trimmed().isEmpty()) {
        if (auto *videoSettings = SettingsManager::instance()->videoSettings()) {
            const QString currentRtsp = videoSettings->rtspUrl()->rawValue().toString().trimmed();
            if (!currentRtsp.isEmpty() && currentRtsp != _directRtspUrl) {
                _jetsonRtspUrl = currentRtsp;
                _saveSettings();
                emit endpointsChanged();
            }
        }
    }

    _activateMode();
}

void AtlasPayloadRouter::_emitRouteEvent(const QString &message)
{
    emit eventReceived(QStringLiteral("info"), message, QDateTime::currentMSecsSinceEpoch());
}

void AtlasPayloadRouter::setMode(const QString &requestedMode)
{
    const QString normalized = requestedMode.trimmed().toUpper();
    Mode newMode;

    if (normalized == QString::fromLatin1(kModeDirectA8) || normalized == QStringLiteral("DIRECT") || normalized == QStringLiteral("A8")) {
        newMode = Mode::DirectA8;
    } else if (normalized == QString::fromLatin1(kModeJetson)) {
        newMode = Mode::Jetson;
    } else {
        return;
    }

    if (_mode == newMode) {
        return;
    }

    // Manual route only: no autodetection and no automatic fallback.
    gimbalStop();
    _jetson->disconnectFromHost();
    _siyi->disconnectFromHost();

    _mode = newMode;
    _saveSettings();

    emit modeChanged();
    emit routeChanged();
    emit statusChanged();

    if (_started) {
        _activateMode();
    }

    _emitRouteEvent(QStringLiteral("Payload route: %1").arg(modeDisplayName()));
}

void AtlasPayloadRouter::configureDirectA8(const QString &host, int port, const QString &rtspUrl)
{
    const QString cleanHost = host.trimmed();
    const QString cleanRtsp = rtspUrl.trimmed();
    if (cleanHost.isEmpty() || port <= 0 || port > 65535 || cleanRtsp.isEmpty()) {
        emit eventReceived(QStringLiteral("warning"),
                           QStringLiteral("Direct A8 settings not applied: invalid endpoint"),
                           QDateTime::currentMSecsSinceEpoch());
        return;
    }

    _directA8Host = cleanHost;
    _directA8Port = port;
    _directRtspUrl = cleanRtsp;
    _saveSettings();
    emit endpointsChanged();
    emit routeChanged();

    if (_started && directMode()) {
        gimbalStop();
        _siyi->disconnectFromHost();
        _siyi->connectToHost(_directA8Host, _directA8Port);
        _applyVideoUrl(_directRtspUrl);
        emit statusChanged();
    }

    _emitRouteEvent(QStringLiteral("Direct A8 profile updated"));
}

void AtlasPayloadRouter::configureJetson(const QString &host, int port, const QString &rtspUrl)
{
    const QString cleanHost = host.trimmed();
    const QString cleanRtsp = rtspUrl.trimmed();
    if (cleanHost.isEmpty() || port <= 0 || port > 65535) {
        emit eventReceived(QStringLiteral("warning"),
                           QStringLiteral("Jetson settings not applied: invalid control endpoint"),
                           QDateTime::currentMSecsSinceEpoch());
        return;
    }

    _jetsonHost = cleanHost;
    _jetsonPort = port;
    _jetsonRtspUrl = cleanRtsp;
    _saveSettings();
    emit endpointsChanged();
    emit routeChanged();

    if (_started && jetsonMode()) {
        gimbalStop();
        _jetson->disconnectFromHost();
        _jetson->connectToHost(_jetsonHost, _jetsonPort);
        if (_jetsonRtspUrl.isEmpty()) {
            _disableVideoIfUnconfigured();
        } else {
            _applyVideoUrl(_jetsonRtspUrl);
        }
        emit statusChanged();
    }

    _emitRouteEvent(QStringLiteral("Jetson profile updated"));
}

void AtlasPayloadRouter::_activateMode()
{
    if (directMode()) {
        _jetson->disconnectFromHost();
        _siyi->connectToHost(_directA8Host, _directA8Port);
        _applyVideoUrl(_directRtspUrl);
    } else {
        _siyi->disconnectFromHost();
        _jetson->connectToHost(_jetsonHost, _jetsonPort);
        if (_jetsonRtspUrl.trimmed().isEmpty()) {
            _disableVideoIfUnconfigured();
            emit eventReceived(QStringLiteral("warning"),
                               QStringLiteral("Jetson RTSP URL is not configured"),
                               QDateTime::currentMSecsSinceEpoch());
        } else {
            _applyVideoUrl(_jetsonRtspUrl);
        }
    }

    emit routeChanged();
    emit statusChanged();
}

void AtlasPayloadRouter::_applyVideoUrl(const QString &url)
{
    const QString cleanUrl = url.trimmed();
    if (cleanUrl.isEmpty()) {
        return;
    }

    auto *videoSettings = SettingsManager::instance()->videoSettings();
    if (!videoSettings) {
        return;
    }

    // Enable first and change the source last so VideoManager observes at least
    // one configuration signal while hasVideo() is already true. startVideo()
    // is then called explicitly as a final guard for the case where all Facts
    // already held the requested values (Fact does not emit on no-op writes).
    videoSettings->streamEnabled()->setRawValue(true);
    videoSettings->rtspUrl()->setRawValue(cleanUrl);
    videoSettings->videoSource()->setRawValue(videoSettings->rtspVideoSource());
    VideoManager::instance()->startVideo();
}

void AtlasPayloadRouter::_disableVideoIfUnconfigured()
{
    if (auto *videoSettings = SettingsManager::instance()->videoSettings()) {
        // Do not leave the previous route's picture running under the newly
        // selected route. VideoManager listens to videoSource/URL changes, so
        // selecting its explicit disabled source guarantees the old receiver
        // is stopped even though streamEnabled itself is not a restart signal.
        videoSettings->rtspUrl()->setRawValue(QString());
        videoSettings->videoSource()->setRawValue(videoSettings->disabledVideoSource());
        videoSettings->streamEnabled()->setRawValue(false);
        VideoManager::instance()->stopVideo();
    }
}

void AtlasPayloadRouter::gimbalRate(double yaw, double pitch)
{
    if (directMode()) _siyi->gimbalRate(yaw, pitch); else _jetson->gimbalRate(yaw, pitch);
}
void AtlasPayloadRouter::gimbalStop()
{
    if (directMode()) _siyi->gimbalStop(); else _jetson->gimbalStop();
}
void AtlasPayloadRouter::gimbalCenter()
{
    if (directMode()) _siyi->gimbalCenter(); else _jetson->gimbalCenter();
}
void AtlasPayloadRouter::gimbalPreset(const QString &preset)
{
    if (directMode()) _siyi->gimbalPreset(preset); else _jetson->gimbalPreset(preset);
}
void AtlasPayloadRouter::zoom(int value)
{
    if (directMode()) _siyi->zoom(value); else _jetson->zoom(value);
}
void AtlasPayloadRouter::takePhoto()
{
    if (directMode()) _siyi->takePhoto(); else _jetson->takePhoto();
}
void AtlasPayloadRouter::setRecording(bool enabled)
{
    if (directMode()) _siyi->setRecording(enabled); else _jetson->setRecording(enabled);
}

void AtlasPayloadRouter::refreshA8CodecSpecs()
{
    if (directMode()) _siyi->requestCodecSpecs();
}

void AtlasPayloadRouter::configureA8MainStream(const QString &codec, int width, int height, int bitrateKbps)
{
    if (directMode()) _siyi->configureMainStream(codec, width, height, bitrateKbps);
}

void AtlasPayloadRouter::configureA8RecordStream(int width, int height, int bitrateKbps)
{
    if (directMode()) _siyi->configureRecordStream(width, height, bitrateKbps);
}

void AtlasPayloadRouter::setA8GimbalWorkingMode(const QString &mode)
{
    if (directMode()) _siyi->setGimbalWorkingMode(mode);
}

void AtlasPayloadRouter::setA8VideoOutput(const QString &output)
{
    if (directMode()) _siyi->setVideoOutput(output);
}

void AtlasPayloadRouter::setAiEnabled(bool enabled) { if (jetsonMode()) _jetson->setAiEnabled(enabled); }
void AtlasPayloadRouter::setSahiEnabled(bool enabled) { if (jetsonMode()) _jetson->setSahiEnabled(enabled); }
void AtlasPayloadRouter::setDepthEnabled(bool enabled) { if (jetsonMode()) _jetson->setDepthEnabled(enabled); }
void AtlasPayloadRouter::setAiModel(const QString &model) { if (jetsonMode()) _jetson->setAiModel(model); }
void AtlasPayloadRouter::setPerformanceProfile(const QString &profile) { if (jetsonMode()) _jetson->setPerformanceProfile(profile); }
void AtlasPayloadRouter::setMissionProfile(const QString &profile) { if (jetsonMode()) _jetson->setMissionProfile(profile); }
void AtlasPayloadRouter::setStreamMode(const QString &streamMode) { if (jetsonMode()) _jetson->setStreamMode(streamMode); }
void AtlasPayloadRouter::setVideoProfile(const QString &profile) { if (jetsonMode()) _jetson->setVideoProfile(profile); }
void AtlasPayloadRouter::setAiClasses(const QStringList &classes) { if (jetsonMode()) _jetson->setAiClasses(classes); }
void AtlasPayloadRouter::setConfidence(double value) { if (jetsonMode()) _jetson->setConfidence(value); }
void AtlasPayloadRouter::setAiFpsTarget(int value) { if (jetsonMode()) _jetson->setAiFpsTarget(value); }
void AtlasPayloadRouter::setAiResolution(int value) { if (jetsonMode()) _jetson->setAiResolution(value); }
void AtlasPayloadRouter::setSahiSlice(int value) { if (jetsonMode()) _jetson->setSahiSlice(value); }
void AtlasPayloadRouter::setTrackingModeEnabled(bool enabled) { if (jetsonMode()) _jetson->setTrackingModeEnabled(enabled); }
void AtlasPayloadRouter::trackPoint(double xNorm, double yNorm) { if (jetsonMode()) _jetson->trackPoint(xNorm, yNorm); }
void AtlasPayloadRouter::trackRoi(double xNorm, double yNorm, double widthNorm, double heightNorm)
{
    if (jetsonMode()) _jetson->trackRoi(xNorm, yNorm, widthNorm, heightNorm);
}
void AtlasPayloadRouter::setAutoGimbal(bool enabled) { if (jetsonMode()) _jetson->setAutoGimbal(enabled); }
void AtlasPayloadRouter::clearTracking() { if (jetsonMode()) _jetson->clearTracking(); }
