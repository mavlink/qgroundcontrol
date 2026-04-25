#include "VideoStreamOrchestrator.h"

#include "QGCCorePlugin.h"
#include "QGCLoggingCategory.h"
#include "QtMultimediaReceiver.h"
#include "VideoSettingsBridge.h"
#include "VideoSourceController.h"
#include "VideoStream.h"
#include "VideoStreamAggregateMonitor.h"
#include "VideoStreamModel.h"
#include "VideoUvcController.h"

#include <QtMultimedia/QMediaDevices>
#include <utility>

QGC_LOGGING_CATEGORY(VideoStreamOrchestratorLog, "Video.VideoStreamOrchestrator")

namespace {

VideoReceiver* createDisplayReceiver(const VideoSourceResolver::VideoSource& source,
                                     [[maybe_unused]] bool thermal,
                                     QObject* parent)
{
    Q_UNUSED(source);

    if (auto* core = QGCCorePlugin::instance()) {
        if (auto* override = core->createVideoReceiver(parent))
            return override;
    }

    return new QtMultimediaReceiver(parent);
}

}  // namespace

VideoStreamOrchestrator::VideoStreamOrchestrator(VideoSettings* settings, QObject* parent)
    : QObject(parent),
      _settingsBridge(new VideoSettingsBridge(settings, this)),
      _sourceController(new VideoSourceController(settings, this)),
      _mediaDevices(new QMediaDevices(this)),
      _streamModel(new VideoStreamModel(this)),
      _aggregateMonitor(new VideoStreamAggregateMonitor(this)),
      _uvcController(new VideoUvcController(_streamModel, this)),
      _dynamicFactory(createDisplayReceiver)
{
    _uvcController->setLocalCameraAvailable(QtMultimediaReceiver::localCameraAvailable);

    (void)connect(_settingsBridge, &VideoSettingsBridge::sourceChanged, this,
                  &VideoStreamOrchestrator::_onSourceChanged);
    (void)connect(_sourceController, &VideoSourceController::sourceChanged, this,
                  &VideoStreamOrchestrator::_onSourceChanged);
    (void)connect(_settingsBridge, &VideoSettingsBridge::lowLatencyChanged, this, [this]() {
        forEachStream([this](VideoStream* s) {
            (void)_sourceController->applyLowLatency(s);
            s->restart();
        });
    });
    (void)connect(_aggregateMonitor, &VideoStreamAggregateMonitor::decodingChanged, this,
                  &VideoStreamOrchestrator::decodingChanged);
    (void)connect(_aggregateMonitor, &VideoStreamAggregateMonitor::recordingChanged, this,
                  &VideoStreamOrchestrator::recordingChanged);
    (void)connect(_mediaDevices, &QMediaDevices::videoInputsChanged, this, [this]() {
        if (!_isUvcSource())
            return;

        if (_uvcController->localCameraAvailable())
            _activateUvcStream();
        else
            _deactivateUvcStream();

        emit hasVideoChanged();
        emit isStreamSourceChanged();
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
        {VideoStream::Role::Primary, createDisplayReceiver},
        {VideoStream::Role::Thermal, createDisplayReceiver},
    };

    for (const auto& def : defs) {
        auto* stream = _createStream(def.role, def.factory);
        (void) _applyResolvedSource(stream);

        // Non-empty URI with no receiver means no receiver can serve the source.
        const QString uri = stream->uri();
        if (!uri.isEmpty() && stream->receiver() == nullptr) {
            qCWarning(VideoStreamOrchestratorLog)
                << "No receiver for URI:" << uri;
        }

        if (hasVideo())
            stream->restart();
    }

    if (_isUvcSource())
        _activateUvcStream();

    qCDebug(VideoStreamOrchestratorLog) << "Video streams created:" << _streams.size();

    emit streamsCreated();
}

#ifdef QGC_UNITTEST_BUILD
void VideoStreamOrchestrator::setLocalCameraAvailableForTest(LocalCameraAvailableFn fn)
{
    _uvcController->setLocalCameraAvailable(std::move(fn));
}

void VideoStreamOrchestrator::registerStreamForTest(VideoStream* stream)
{
    _addStream(stream);
}

bool VideoStreamOrchestrator::reconcileDynamicStreamsForTest(
    const QHash<quint8, VideoSourceResolver::StreamInfo>& expected)
{
    return _reconcileDynamicStreams(expected);
}
#endif

void VideoStreamOrchestrator::cleanup()
{
    for (auto* stream : std::as_const(_streams))
        stream->stop();

    // Empty the model before destroying streams so QML delegates don't
    // dereference dangling pointers during teardown.
    for (auto* stream : std::as_const(_streams)) {
        _aggregateMonitor->unwatch(stream);
        _streamModel->removeStream(stream);
    }

    qDeleteAll(_streams);
    _streams.clear();
}

void VideoStreamOrchestrator::setCameraManager(QGCCameraManager* cameraManager)
{
    _sourceController->setCameraManager(cameraManager);
    _onSourceChanged();

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

bool VideoStreamOrchestrator::decoding() const
{
    return _aggregateMonitor->decoding();
}

bool VideoStreamOrchestrator::recording() const
{
    return _aggregateMonitor->recording();
}

bool VideoStreamOrchestrator::hasVideo() const
{
    return _sourceController->hasVideo(primaryStream());
}

bool VideoStreamOrchestrator::autoStreamConfigured() const
{
    return _sourceController->autoStreamConfigured(primaryStream());
}

bool VideoStreamOrchestrator::isAutoStreamConfigured(const std::optional<VideoSourceResolver::StreamInfo>& metadata)
{
    return metadata && !metadata->thermal && !metadata->uri.isEmpty();
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
    return _reconcileDynamicStreams(_sourceController->expectedDynamicStreams());
}

bool VideoStreamOrchestrator::_reconcileDynamicStreams(const QHash<quint8, VideoSourceResolver::StreamInfo>& expected)
{
    // Build a lookup of existing Dynamic streams keyed by MAVLink stream ID.
    QHash<quint8, VideoStream*> existing;
    for (auto* s : _streams) {
        if (!s || s->role() != VideoStream::Role::Dynamic)
            continue;
        const std::optional<quint8> streamId = s->metadataStreamId();
        if (!streamId)
            continue;
        existing.insert(*streamId, s);
    }

    bool changed = false;

    // Remove Dynamic streams whose ID is no longer present.
    for (auto it = existing.begin(); it != existing.end(); ++it) {
        if (!expected.contains(it.key())) {
            VideoStream* s = it.value();
            s->stop();
            _removeStream(s);
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
        VideoSourceResolver::StreamInfo info = it.value();
        info.streamID = id;
        const QString name = QStringLiteral("dynamicVideo.%1").arg(id);

        auto* stream = _createStream(VideoStream::Role::Dynamic, name, _dynamicFactory);
        stream->setSourceMetadata(info);
        (void) _applyResolvedSource(stream);

        changed = true;
        qCDebug(VideoStreamOrchestratorLog) << "Added Dynamic stream for ID" << id << "uri=" << stream->uri();
    }

    return changed;
}

bool VideoStreamOrchestrator::_applyResolvedSource(VideoStream* stream)
{
    return _sourceController->applyResolvedSource(stream);
}

bool VideoStreamOrchestrator::_isUvcSource() const
{
    return _sourceController->isUvcSource();
}

VideoStream* VideoStreamOrchestrator::_ensureUvcStream()
{
    if (auto* existing = uvcStream())
        return existing;

    auto* uvc = _createStream(VideoStream::Role::UVC, createDisplayReceiver);
    qCDebug(VideoStreamOrchestratorLog) << "Created UVC stream for active local camera source";
    return uvc;
}

void VideoStreamOrchestrator::_activateUvcStream()
{
    if (!_uvcController->activate(primaryStream(), [this]() { return _ensureUvcStream(); }))
        return;

    qCDebug(VideoStreamOrchestratorLog) << "Activated UVC stream";
}

void VideoStreamOrchestrator::_deactivateUvcStream()
{
    auto* uvc = uvcStream();
    if (!uvc)
        return;

    if (!_uvcController->deactivate(uvc, [this, uvc]() { _removeStream(uvc); }))
        return;
    uvc->deleteLater();
    qCDebug(VideoStreamOrchestratorLog) << "Deactivated UVC stream";
}

void VideoStreamOrchestrator::_onSourceChanged()
{
    // No re-entrancy guard needed: source writeback uses
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
            if (stream->role() == VideoStream::Role::Dynamic)
                return;  // Dynamic streams: handled by _reconcileDynamicStreams().
            _sourceController->bindCameraMetadata(stream);
            changed |= _applyResolvedSource(stream);
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

        qCDebug(VideoStreamOrchestratorLog) << "Video source changed";
    }
}

void VideoStreamOrchestrator::_wireStreamSignals(VideoStream* stream)
{
    (void)connect(stream, &VideoStream::recordingError, this,
                  &VideoStreamOrchestrator::recordingError);

    (void)connect(stream, &VideoStream::videoSizeChanged, this, [this, stream](QSize size) {
        if (!stream->isThermal())
            _videoSize = size;
    });

    (void)connect(stream, &VideoStream::videoStreamInfoChanged, this, [this, stream]() {
        (void) _applyResolvedSource(stream);
    });

    (void)connect(stream, &VideoStream::frameDeliveryChanged, this, [this, stream]() {
        if (stream->role() == VideoStream::Role::Primary)
            emit primaryFrameDeliveryChanged();
    });
}

VideoStream* VideoStreamOrchestrator::_createStream(VideoStream::Role role,
                                                    VideoStream::ReceiverFactory factory)
{
    auto* stream = new VideoStream(role, std::move(factory), this);
    _addStream(stream);
    return stream;
}

VideoStream* VideoStreamOrchestrator::_createStream(VideoStream::Role role,
                                                    const QString& name,
                                                    VideoStream::ReceiverFactory factory)
{
    auto* stream = new VideoStream(role, name, std::move(factory), this);
    _addStream(stream);
    return stream;
}

void VideoStreamOrchestrator::_addStream(VideoStream* stream)
{
    if (!stream || _streams.contains(stream))
        return;

    _streams.append(stream);
    _wireStreamSignals(stream);
    _aggregateMonitor->watch(stream);
    _streamModel->addStream(stream);
}

void VideoStreamOrchestrator::_removeStream(VideoStream* stream)
{
    if (!stream || !_streams.contains(stream))
        return;

    _streamModel->removeStream(stream);
    _aggregateMonitor->unwatch(stream);
    _streams.removeAll(stream);
    disconnect(stream, nullptr, this, nullptr);
}
