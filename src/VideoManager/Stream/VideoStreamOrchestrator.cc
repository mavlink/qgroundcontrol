#include "VideoStreamOrchestrator.h"

#include "MavlinkCameraControlInterface.h"
#include "QGCApplication.h"
#include "QGCCameraManager.h"
#include "QGCLoggingCategory.h"
#include "QGCVideoStreamInfo.h"
#include "QmlObjectListModel.h"
#include "UVCReceiver.h"
#include "VideoBackendRegistry.h"
#include "VideoFrameDelivery.h"
#include "VideoReceiverFactory.h"
#include "VideoSettings.h"
#include "VideoSettingsBridge.h"
#include "VideoStream.h"
#include "VideoStreamModel.h"

QGC_LOGGING_CATEGORY(VideoStreamOrchestratorLog, "Video.VideoStreamOrchestrator")

VideoStreamOrchestrator::VideoStreamOrchestrator(VideoSettings* settings, QObject* parent)
    : QObject(parent),
      _settings(settings),
      _settingsBridge(new VideoSettingsBridge(settings, this)),
      _streamModel(new VideoStreamModel(this)),
      _dynamicFactory(VideoReceiverFactory::gstOrQtMultimedia)
{
    // Bridge signals wire now; nothing fires until bindToSettings() → subscribe().
    (void)connect(_settingsBridge, &VideoSettingsBridge::sourceChanged, this,
                  &VideoStreamOrchestrator::_onSourceChanged);
    (void)connect(_settingsBridge, &VideoSettingsBridge::lowLatencyChanged, this, [this]() {
        forEachStream([](VideoStream* s) { s->restart(); });
    });
}

VideoStreamOrchestrator::~VideoStreamOrchestrator() = default;

void VideoStreamOrchestrator::refreshFromSettings()
{
    _onSourceChanged();
}

void VideoStreamOrchestrator::bindToSettings()
{
    _settingsBridge->subscribe();
}

void VideoStreamOrchestrator::createStreams()
{
    if (!_streams.isEmpty())
        return;  // already created (idempotency guard)

    qCDebug(VideoStreamOrchestratorLog) << "Creating video streams";

    if (_createStreamsForTest) {
        _createStreamsForTest();
        emit streamsCreated();
        return;
    }

    struct StreamDef
    {
        VideoStream::Role role;
        VideoStream::ReceiverFactory factory;
    };

    const QList<StreamDef> defs = {
        {VideoStream::Role::Primary, VideoReceiverFactory::gstOrQtMultimedia},
        {VideoStream::Role::Thermal, VideoReceiverFactory::gstOrQtMultimedia},
    };

    for (const auto& def : defs) {
        auto* stream = new VideoStream(def.role, def.factory, this);
        _streams.append(stream);

        _wireStreamSignals(stream);
        stream->updateFromSettings(_settings);

        // Non-empty URI with no receiver ⇒ no backend can serve the scheme.
        const QString uri = stream->uri();
        if (!uri.isEmpty() && stream->receiver() == nullptr) {
            qCWarning(VideoStreamOrchestratorLog)
                << "No receiver for URI:" << uri << "— backend may be unavailable";
        }

        // Stable row per stream — survives backend switches.
        _streamModel->addStream(stream);

        if (hasVideo())
            stream->restart();
    }

    // Local camera stream (UVC on desktop, built-in camera on Android/iOS)
    if (VideoBackendRegistry::instance().isAvailable(VideoReceiver::BackendKind::UVC)) {
        auto* uvcStream = new VideoStream(VideoStream::Role::UVC, VideoReceiverFactory::uvc, this);
        _streams.append(uvcStream);

        _wireStreamSignals(uvcStream);
        _streamModel->addStream(uvcStream);

        qCDebug(VideoStreamOrchestratorLog) << "UVC stream created";
    }

    if (_isUvcSource())
        _activateUvcStream();

    qCDebug(VideoStreamOrchestratorLog) << "Video streams created:" << _streams.size();

    emit streamsCreated();
}

void VideoStreamOrchestrator::cleanup()
{
    for (auto* stream : std::as_const(_streams))
        stream->stop();

    // Empty the model before destroying streams so QML delegates don't
    // dereference dangling pointers during teardown.
    for (auto* stream : std::as_const(_streams))
        _streamModel->removeStream(stream);

    qDeleteAll(_streams);
    _streams.clear();
}

void VideoStreamOrchestrator::setCameraManager(QGCCameraManager* cameraManager)
{
    if (_cameraManager) {
        disconnect(_cameraManagerConn);
        _cameraManagerConn = {};
        forEachNetworkStream([](VideoStream* s) { s->setVideoStreamInfo(nullptr); });
    }

    _cameraManager = cameraManager;

    if (_cameraManager) {
        _cameraManagerConn = connect(_cameraManager, &QGCCameraManager::streamChanged, this,
                                     &VideoStreamOrchestrator::_onSourceChanged);

        forEachNetworkStream([this](VideoStream* stream) {
            if (stream->role() == VideoStream::Role::Primary)
                stream->setVideoStreamInfo(_cameraManager->currentStreamInstance());
            else if (stream->role() == VideoStream::Role::Thermal)
                stream->setVideoStreamInfo(_cameraManager->thermalStreamInstance());
            // Dynamic streams: reconciled below.
        });

        _reconcileDynamicStreams();
    } else {
        forEachNetworkStream([](VideoStream* s) { s->setVideoStreamInfo(nullptr); });
        // No camera manager ⇒ no MAVLink stream list ⇒ tear down all Dynamic streams.
        _reconcileDynamicStreams();
    }

    // hasThermal no longer aggregated at the facade — QML binds directly
    // to `streamModel.activeStreamForRole(Thermal).videoStreamInfo.isThermal`.
}

void VideoStreamOrchestrator::startVideo()
{
    qCDebug(VideoStreamOrchestratorLog) << "startVideo";
    if (!hasVideo()) {
        qCDebug(VideoStreamOrchestratorLog) << "Stream not enabled/configured";
        return;
    }

    if (_isUvcSource())
        _activateUvcStream();
    else
        forEachNetworkStream([](VideoStream* s) { s->restart(); });
}

void VideoStreamOrchestrator::stopVideo()
{
    forEachStream([](VideoStream* s) { s->stop(); });
}

bool VideoStreamOrchestrator::streaming() const
{
    for (const auto* s : _streams) {
        if (!s->isThermal() && s->streaming())
            return true;
    }
    return false;
}

bool VideoStreamOrchestrator::decoding() const
{
    for (const auto* s : _streams) {
        if (!s->isThermal() && s->decoding())
            return true;
    }
    return false;
}

bool VideoStreamOrchestrator::recording() const
{
    for (const auto* s : _streams) {
        if (!s->isThermal() && s->recording())
            return true;
    }
    return false;
}

bool VideoStreamOrchestrator::hasVideo() const
{
    return _settings->streamEnabled()->rawValue().toBool() && _settings->streamConfigured();
}

VideoStream* VideoStreamOrchestrator::streamByRole(VideoStream::Role role) const
{
    for (auto* s : _streams) {
        if (s && s->role() == role)
            return s;
    }
    return nullptr;
}

QList<VideoStream*> VideoStreamOrchestrator::dynamicStreams() const
{
    QList<VideoStream*> out;
    for (auto* s : _streams) {
        if (s && s->role() == VideoStream::Role::Dynamic)
            out.append(s);
    }
    return out;
}

bool VideoStreamOrchestrator::_reconcileDynamicStreams()
{
    // Build the expected {streamID → info} map from the camera's reported list,
    // excluding streams already bound to Primary/Thermal slots.
    QHash<quint8, QGCVideoStreamInfo*> expected;
    if (_cameraManager) {
        auto* currentCamera = _cameraManager->currentCameraInstance();
        auto* currentInfo = _cameraManager->currentStreamInstance();
        auto* thermalInfo = _cameraManager->thermalStreamInstance();
        if (currentCamera && currentCamera->streams()) {
            auto* list = currentCamera->streams();
            for (int i = 0; i < list->count(); ++i) {
                auto* info = qobject_cast<QGCVideoStreamInfo*>(list->get(i));
                if (!info)
                    continue;
                // Skip the streams already consumed by Primary/Thermal — comparison
                // by QGCVideoStreamInfo pointer identity (stable across infoChanged).
                if (info == currentInfo || info == thermalInfo)
                    continue;
                expected.insert(info->streamID(), info);
            }
        }
    }
    return _reconcileDynamicStreams(expected);
}

bool VideoStreamOrchestrator::_reconcileDynamicStreams(const QHash<quint8, QGCVideoStreamInfo*>& expected)
{
    // Build a lookup of existing Dynamic streams keyed by the stream ID embedded
    // in the stream's name (`dynamicVideo.<id>`).
    QHash<quint8, VideoStream*> existing;
    for (auto* s : _streams) {
        if (!s || s->role() != VideoStream::Role::Dynamic)
            continue;
        const QString name = s->name();
        const int dot = name.lastIndexOf(QLatin1Char('.'));
        if (dot < 0)
            continue;
        bool ok = false;
        const uint id = name.mid(dot + 1).toUInt(&ok);
        if (ok && id <= 0xFF)
            existing.insert(static_cast<quint8>(id), s);
    }

    bool changed = false;

    // Remove Dynamic streams whose ID is no longer present.
    for (auto it = existing.begin(); it != existing.end(); ++it) {
        if (!expected.contains(it.key())) {
            VideoStream* s = it.value();
            s->stop();
            _streamModel->removeStream(s);
            _streams.removeAll(s);
            s->deleteLater();
            changed = true;
            qCDebug(VideoStreamOrchestratorLog) << "Removed Dynamic stream for vanished ID" << it.key();
        }
    }

    // Create Dynamic streams for new IDs.
    for (auto it = expected.begin(); it != expected.end(); ++it) {
        if (existing.contains(it.key()))
            continue;
        const quint8 id = it.key();
        QGCVideoStreamInfo* info = it.value();
        const QString name = QStringLiteral("dynamicVideo.%1").arg(id);

        // Dynamic streams route MAVLink URIs through the GStreamer/QtMM table —
        // same factory as Primary. UVC is hardware-only and never dynamic.
        // Factory pulled from `_dynamicFactory` so tests can inject FakeVideoReceiver.
        auto* stream = new VideoStream(VideoStream::Role::Dynamic, name,
                                        _dynamicFactory, this);
        _streams.append(stream);
        _wireStreamSignals(stream);
        _streamModel->addStream(stream);

        // setVideoStreamInfo triggers _updateAutoStream() for Role::Dynamic,
        // so the URI is resolved immediately — no separate updateFromSettings.
        stream->setVideoStreamInfo(info);

        changed = true;
        qCDebug(VideoStreamOrchestratorLog) << "Added Dynamic stream for ID" << id << "uri=" << stream->uri();
    }

    return changed;
}

bool VideoStreamOrchestrator::_isUvcSource() const
{
    return _settings->currentSourceType() == VideoSettings::SourceType::UVC;
}

void VideoStreamOrchestrator::_activateUvcStream()
{
    auto* uvc = uvcStream();
    if (!uvc)
        return;

    // UVC takes over the video display — stop the network primary first.
    if (auto* primary = primaryStream())
        primary->stop();

    uvc->setUri(QStringLiteral("uvc://local"));
    uvc->restart();

    // activeStreamChanged(Primary) lets QML re-bind sinks declaratively.
    _streamModel->setUvcActive(true);
    qCDebug(VideoStreamOrchestratorLog) << "Activated UVC stream";
}

void VideoStreamOrchestrator::_deactivateUvcStream()
{
    auto* uvc = uvcStream();
    if (!uvc)
        return;

    uvc->stop();
    uvc->setUri(QString());

    _streamModel->setUvcActive(false);
    qCDebug(VideoStreamOrchestratorLog) << "Deactivated UVC stream";
}

void VideoStreamOrchestrator::_onSourceChanged()
{
    // No re-entrancy guard needed: VideoStream::updateFromSettings uses
    // QSignalBlocker to suppress the rawValueChanged re-fire during writeback.
    const bool uvcSelected = _isUvcSource();
    const bool wasUvc = uvcStream() && uvcStream()->started();
    bool changed = false;

    if (uvcSelected) {
        if (!wasUvc) {
            _activateUvcStream();
            changed = true;
        }
    } else {
        if (wasUvc) {
            _deactivateUvcStream();
            changed = true;
        }

        forEachNetworkStream([this, &changed](VideoStream* stream) {
            QGCVideoStreamInfo* info = nullptr;
            if (stream->role() == VideoStream::Role::Primary)
                info = _cameraManager ? _cameraManager->currentStreamInstance() : nullptr;
            else if (stream->role() == VideoStream::Role::Thermal)
                info = _cameraManager ? _cameraManager->thermalStreamInstance() : nullptr;
            else
                return;  // Dynamic streams: handled by _reconcileDynamicStreams().
            stream->setVideoStreamInfo(info);
            changed |= stream->updateFromSettings(_settings);
        });

        // Primary/Thermal rebind finished — now reconcile the Dynamic set
        // against whatever else the active camera is advertising.
        changed |= _reconcileDynamicStreams();
    }

    if (changed) {
        emit hasVideoChanged();
        emit isStreamSourceChanged();

        if (!uvcSelected) {
            if (hasVideo())
                forEachNetworkStream([](VideoStream* s) { s->restart(); });
            else
                stopVideo();
        }

        qCDebug(VideoStreamOrchestratorLog)
            << "New Video Source:" << _settings->videoSource()->rawValue().toString();
    }
}

void VideoStreamOrchestrator::_recomputeAggregate()
{
    const bool nowStreaming = streaming();
    const bool nowDecoding = decoding();
    const bool nowRecording = recording();

    if (nowStreaming != _agg.streaming) {
        _agg.streaming = nowStreaming;
        emit streamingChanged();
    }
    if (nowDecoding != _agg.decoding) {
        _agg.decoding = nowDecoding;
        emit decodingChanged();
    }
    if (nowRecording != _agg.recording) {
        _agg.recording = nowRecording;
        emit recordingChanged(nowRecording);
    }
}

void VideoStreamOrchestrator::_wireStreamSignals(VideoStream* stream)
{
    // Funnel per-stream changes into _recomputeAggregate so NOTIFY signals
    // fire only on true aggregate transitions, not per-stream churn.
    (void)connect(stream, &VideoStream::streamingChanged, this,
                  &VideoStreamOrchestrator::_recomputeAggregate);
    (void)connect(stream, &VideoStream::decodingChanged, this,
                  &VideoStreamOrchestrator::_recomputeAggregate);
    (void)connect(stream, &VideoStream::recordingChanged, this,
                  &VideoStreamOrchestrator::_recomputeAggregate);

    (void)connect(stream, &VideoStream::recordingError, this,
                  &VideoStreamOrchestrator::recordingError);

    // Primary stream's resolution still tracked here; `videoSize()` is read
    // from `RecordingCoordinator::startRecording` for subtitle sizing.
    (void)connect(stream, &VideoStream::videoSizeChanged, this, [this, stream](QSize size) {
        if (!stream->isThermal())
            _videoSize = size;
    });

    // Settings re-sync on stream-info changes — URI/lowLatency may need to
    // pick up auto-stream values from the new info.
    (void)connect(stream, &VideoStream::videoStreamInfoChanged, this, [this, stream]() {
        stream->updateFromSettings(_settings);
    });

    // Primary bridge changes — facade re-binds subtitle overlay sink.
    (void)connect(stream, &VideoStream::bridgeChanged, this, [this, stream]() {
        if (stream->role() == VideoStream::Role::Primary)
            emit primaryBridgeChanged();
    });
}
