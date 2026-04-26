#include "VideoStream.h"

#include "QGCLoggingCategory.h"
#include "QGCVideoStreamInfo.h"
#include "VideoSourceResolver.h"
#include "VideoStreamSession.h"

#include <utility>

QGC_LOGGING_CATEGORY(VideoStreamLog, "Video.VideoStream")

// ═══════════════════════════════════════════════════════════════════════════
// Construction / destruction
// ═══════════════════════════════════════════════════════════════════════════

QString VideoStream::nameForRole(Role role)
{
    switch (role) {
        case Role::Primary:
            return QStringLiteral("videoContent");
        case Role::Thermal:
            return QStringLiteral("thermalVideo");
        case Role::UVC:
            return QStringLiteral("uvcVideo");
        case Role::Dynamic:
            return QStringLiteral("dynamicVideo");
    }
    Q_UNREACHABLE();
}

std::optional<VideoStream::Role> VideoStream::roleForName(const QString& name)
{
    if (name == QLatin1String("videoContent"))
        return Role::Primary;
    if (name == QLatin1String("thermalVideo"))
        return Role::Thermal;
    if (name == QLatin1String("uvcVideo"))
        return Role::UVC;
    if (name == QLatin1String("dynamicVideo"))
        return Role::Dynamic;
    return std::nullopt;
}

VideoStream::VideoStream(Role role, ReceiverFactory factory, QObject* parent)
    : VideoStream(role, nameForRole(role), std::move(factory), parent)
{
}

VideoStream::VideoStream(Role role, const QString& name, ReceiverFactory factory, QObject* parent)
    : QObject(parent), _role(role), _name(name), _factory(std::move(factory))
{
    _initSession();
    qCDebug(VideoStreamLog) << "Created stream" << _name << (isThermal() ? "(thermal)" : "");
}

void VideoStream::_initSession()
{
    _session = new VideoStreamSession(_name, _role, _factory, this, this);
    _wireSessionSignals();
}

VideoStream::~VideoStream()
{
    _session = nullptr;
}

VideoStream::SessionState VideoStream::sessionState() const
{
    return _session ? _session->sessionState() : SessionState::Stopped;
}

VideoStreamFsm::State VideoStream::fsmState() const
{
    return _session ? _session->fsmState() : VideoStreamFsm::State::Idle;
}

VideoStreamStateMachine* VideoStream::fsm() const
{
    return _session ? _session->fsm() : nullptr;
}

VideoStreamLifecycleController* VideoStream::lifecycle() const
{
    return _session ? _session->lifecycle() : nullptr;
}

// ═══════════════════════════════════════════════════════════════════════════
// Receiver access
// ═══════════════════════════════════════════════════════════════════════════

VideoFrameDelivery* VideoStream::frameDelivery() const
{
    return _session ? _session->frameDelivery() : nullptr;
}

VideoReceiver* VideoStream::receiver() const
{
    return _session ? _session->receiver() : nullptr;
}

VideoRecorder* VideoStream::recorder() const
{
    return _session ? _session->recorder() : nullptr;
}

bool VideoStream::started() const
{
    return _session && _session->started();
}

bool VideoStream::streaming() const
{
    return _session && _session->streaming();
}

bool VideoStream::decoding() const
{
    return _session && _session->decoding();
}

bool VideoStream::firstFrameReady() const
{
    return _session && _session->firstFrameReady();
}

bool VideoStream::recording() const
{
    return _session && _session->recording();
}

bool VideoStream::latencySupported() const
{
    return _session && _session->latencySupported();
}

QString VideoStream::uri() const
{
    return _session ? _session->uri() : QString();
}

const VideoSourceResolver::VideoSource& VideoStream::sourceDescriptor() const
{
    static const VideoSourceResolver::VideoSource emptySource;
    return _session ? _session->sourceDescriptor() : emptySource;
}

QSize VideoStream::videoSize() const
{
    return _session ? _session->videoSize() : QSize();
}

QString VideoStream::lastError() const
{
    return _session ? _session->lastError() : QString();
}

VideoReceiver::ErrorCategory VideoStream::lastErrorCategory() const
{
    return _session ? _session->lastErrorCategory() : VideoReceiver::ErrorCategory::Transient;
}

QVideoSink* VideoStream::videoSink() const
{
    return _session ? _session->videoSink() : nullptr;
}

// ═══════════════════════════════════════════════════════════════════════════
// Stream info
// ═══════════════════════════════════════════════════════════════════════════

QGCVideoStreamInfo* VideoStream::videoStreamInfo() const
{
    return _videoStreamInfo.data();
}

void VideoStream::setVideoStreamInfo(QGCVideoStreamInfo* info)
{
    if (_videoStreamInfo == info)
        return;

    if (_videoStreamInfoConnection) {
        disconnect(_videoStreamInfoConnection);
        _videoStreamInfoConnection = {};
    }

    _videoStreamInfo = info;
    _sourceMetadata = VideoSourceResolver::streamInfoFrom(info);
    if (_sourceMetadata && _sourceMetadata->streamID)
        _metadataStreamId = _sourceMetadata->streamID;

    if (_videoStreamInfo)
        _videoStreamInfoConnection = connect(_videoStreamInfo.data(), &QGCVideoStreamInfo::infoChanged, this, [this]() {
            _sourceMetadata = VideoSourceResolver::streamInfoFrom(_videoStreamInfo.data());
            emit streamInfoUpdated();
        });

    emit videoStreamInfoChanged();
}

qreal VideoStream::sourceAspectRatio() const
{
    if (!_sourceMetadata || _sourceMetadata->resolution.height() <= 0)
        return 0.0;
    return static_cast<qreal>(_sourceMetadata->resolution.width())
           / static_cast<qreal>(_sourceMetadata->resolution.height());
}

bool VideoStream::sourceIsThermal() const
{
    return _sourceMetadata && _sourceMetadata->thermal;
}

quint16 VideoStream::sourceHfov() const
{
    return _sourceMetadata ? _sourceMetadata->hfov : 0;
}

void VideoStream::setMetadataStreamId(std::optional<quint8> streamId)
{
    _metadataStreamId = streamId;
    if (_sourceMetadata)
        _sourceMetadata->streamID = streamId;
}

void VideoStream::setSourceMetadata(const VideoSourceResolver::StreamInfo& metadata)
{
    if (_videoStreamInfoConnection) {
        disconnect(_videoStreamInfoConnection);
        _videoStreamInfoConnection = {};
    }

    _videoStreamInfo = nullptr;
    _sourceMetadata = metadata;
    _metadataStreamId = metadata.streamID;
    emit videoStreamInfoChanged();
    emit streamInfoUpdated();
}

void VideoStream::clearSourceMetadata()
{
    if (_videoStreamInfoConnection) {
        disconnect(_videoStreamInfoConnection);
        _videoStreamInfoConnection = {};
    }

    _videoStreamInfo = nullptr;
    _sourceMetadata.reset();
    _metadataStreamId.reset();
    emit videoStreamInfoChanged();
    emit streamInfoUpdated();
}

// ═══════════════════════════════════════════════════════════════════════════
// Sink management
// ═══════════════════════════════════════════════════════════════════════════

void VideoStream::registerVideoSink(QVideoSink* sink)
{
    if (_session)
        _session->registerVideoSink(sink);
}

// ═══════════════════════════════════════════════════════════════════════════
// URI resolution
// ═══════════════════════════════════════════════════════════════════════════

bool VideoStream::setUri(const QString& uri)
{
    return setSourceDescriptor(VideoSourceResolver::describeUri(uri));
}

bool VideoStream::setSourceDescriptor(const VideoSourceResolver::VideoSource& source)
{
    return _session && _session->setSourceDescriptor(source);
}

bool VideoStream::setLowLatency(bool lowLatency)
{
    return _session && _session->setLowLatency(lowLatency);
}

// ═══════════════════════════════════════════════════════════════════════════
// Lifecycle
// ═══════════════════════════════════════════════════════════════════════════

void VideoStream::start(uint32_t timeout)
{
    if (_session)
        _session->start(timeout);
}

void VideoStream::stop()
{
    if (_session)
        _session->stop();
}

void VideoStream::restart()
{
    if (_session)
        _session->restart();
}

std::unique_ptr<VideoRecorder> VideoStream::releaseRecorder()
{
    return _session ? _session->releaseRecorder() : std::unique_ptr<VideoRecorder>();
}

void VideoStream::setRecorderForTest(VideoRecorder* recorder)
{
    if (!_session)
        return;

    _session->setRecorderForTest(recorder);
}

void VideoStream::setRecorderFactoryForTest(RecorderFactory factory)
{
    if (_session)
        _session->setRecorderFactoryForTest(std::move(factory));
}

void VideoStream::setPlaybackInputResolverForTest(PlaybackInputResolver resolver)
{
    if (_session)
        _session->setPlaybackInputResolverForTest(std::move(resolver));
}

void VideoStream::_wireSessionSignals()
{
    if (!_session)
        return;

    connect(_session, &VideoStreamSession::sessionStateChanged, this, &VideoStream::sessionStateChanged);
    connect(_session, &VideoStreamSession::fsmStateChanged, this, &VideoStream::fsmStateChanged);
    connect(_session, &VideoStreamSession::streamingChanged, this, &VideoStream::streamingChanged);
    connect(_session, &VideoStreamSession::decodingChanged, this, &VideoStream::decodingChanged);
    connect(_session, &VideoStreamSession::firstFrameReadyChanged, this, &VideoStream::firstFrameReadyChanged);
    connect(_session, &VideoStreamSession::recordingChanged, this, &VideoStream::recordingChanged);
    connect(_session, &VideoStreamSession::recordingStarted, this, &VideoStream::recordingStarted);
    connect(_session, &VideoStreamSession::recordingError, this, &VideoStream::recordingError);
    connect(_session, &VideoStreamSession::videoSizeChanged, this, &VideoStream::videoSizeChanged);
    connect(_session, &VideoStreamSession::uriChanged, this, &VideoStream::uriChanged);
    connect(_session, &VideoStreamSession::frameDeliveryChanged, this, &VideoStream::frameDeliveryChanged);
    connect(_session, &VideoStreamSession::recorderChanged, this, &VideoStream::recorderChanged);
    connect(_session, &VideoStreamSession::receiverChanged, this, &VideoStream::receiverChanged);
    connect(_session, &VideoStreamSession::lastErrorChanged, this, &VideoStream::lastErrorChanged);
    connect(_session, &VideoStreamSession::statsChanged, this, &VideoStream::statsChanged);
    connect(_session, &VideoStreamSession::hwDecodingChanged, this, &VideoStream::hwDecodingChanged);
    connect(_session, &VideoStreamSession::activeDecoderNameChanged, this, &VideoStream::activeDecoderNameChanged);
}

bool VideoStream::hwDecoding() const
{
    return _session && _session->hwDecoding();
}

QString VideoStream::activeDecoderName() const
{
    return _session ? _session->activeDecoderName() : QString();
}

VideoStreamStats* VideoStream::stats() const
{
    return _session ? _session->stats() : nullptr;
}

VideoDiagnostics* VideoStream::diagnostics() const
{
    return _session ? _session->diagnostics() : nullptr;
}

QObject* VideoStream::frameDeliveryAsObject() const
{
    return _session ? _session->frameDeliveryAsObject() : nullptr;
}
