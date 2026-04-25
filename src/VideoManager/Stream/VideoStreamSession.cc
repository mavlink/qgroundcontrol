#include "VideoStreamSession.h"

#include "QGCLoggingCategory.h"
#include "VideoFrameDelivery.h"
#include "VideoPlaybackRuntime.h"
#include "VideoRecorder.h"
#include "VideoStreamLifecycleController.h"
#include "VideoStreamLifecyclePolicy.h"

#include <utility>

QGC_LOGGING_CATEGORY(VideoStreamSessionLog, "Video.VideoStreamSession")

VideoStreamSession::VideoStreamSession(QString name,
                                       VideoStream::Role role,
                                       ReceiverFactory factory,
                                       VideoStream* owner,
                                       QObject* parent)
    : QObject(parent)
    , _name(std::move(name))
    , _role(role)
    , _factory(std::move(factory))
    , _receiverResources(new VideoReceiverSession(_name, _role == VideoStream::Role::Thermal, _factory, owner, this))
    , _diagnostics(new VideoDiagnostics(this))
    , _playbackRuntime(new VideoPlaybackRuntime(_name, this))
    , _lifecycle(new VideoStreamLifecycleController(_name, VideoStreamLifecyclePolicy::policyForRole(_role), this))
{
    _wireSignals();
}

VideoStreamSession::~VideoStreamSession()
{
    _stopIngestSession();
    _destroyReceiver();
}

void VideoStreamSession::_wireSignals()
{
    connect(_diagnostics, &VideoDiagnostics::lastErrorChanged, this, &VideoStreamSession::lastErrorChanged);
    connect(_lifecycle, &VideoStreamLifecycleController::fsmStateChanged, this, &VideoStreamSession::fsmStateChanged);
    connect(_lifecycle, &VideoStreamLifecycleController::sessionStateChanged,
            this, &VideoStreamSession::sessionStateChanged);
    connect(_lifecycle, &VideoStreamLifecycleController::receiverFullyStarted, this, [this]() {
        _restartPolicy.noteReceiverFullyStarted();
    });
    connect(_lifecycle, &VideoStreamLifecycleController::receiverFullyStopped, this, [this]() {
        _restartPolicy.handleReceiverFullyStopped(_name, _uri, _hasPlaybackInput(),
                                                  [this]() { start(_timeoutForUri()); });
    });
    connect(_lifecycle, &VideoStreamLifecycleController::reconnectRequested, this, [this]() {
        _restartPolicy.handleReconnectRequested(_name, _hasPlaybackInput(), [this]() {
            if (!_lifecycle || !_hasPlaybackInput())
                return;
            if (!_refreshPlaybackInputForStart())
                return;
            _lifecycle->requestStart(_timeoutForUri());
        });
    });

    connect(_playbackRuntime, &VideoPlaybackRuntime::errorOccurred, this,
            [this](VideoReceiver::ErrorCategory category, const QString& message) {
                if (_diagnostics)
                    _diagnostics->setError(category, message);
                if (_lifecycle)
                    _lifecycle->handleReceiverError(category, message);
            });
    connect(_playbackRuntime, &VideoPlaybackRuntime::endOfStream, this, [this]() {
        qCDebug(VideoStreamSessionLog) << _name << "GStreamer ingest session reached EOS";
        if (!_isStoppingOrStopped())
            restart();
    });

    connect(_receiverResources, &VideoReceiverSession::streamingChanged, this, &VideoStreamSession::streamingChanged);
    connect(_receiverResources, &VideoReceiverSession::decodingChanged, this, [this](bool active) {
        emit decodingChanged(active);
        emit hwDecodingChanged();
        emit activeDecoderNameChanged();
    });
    connect(_receiverResources, &VideoReceiverSession::timeout, this, &VideoStreamSession::_onReceiverTimeout);
    connect(_receiverResources, &VideoReceiverSession::recordingStarted, this, &VideoStreamSession::recordingStarted);
    connect(_receiverResources, &VideoReceiverSession::recordingChanged, this, &VideoStreamSession::recordingChanged);
    connect(_receiverResources, &VideoReceiverSession::recordingError, this, &VideoStreamSession::recordingError);
    connect(_receiverResources, &VideoReceiverSession::recorderChanged, this, &VideoStreamSession::recorderChanged);
    connect(_receiverResources, &VideoReceiverSession::receiverError, this,
            [this](VideoReceiver::ErrorCategory category, const QString& message) {
                if (_diagnostics)
                    _diagnostics->setError(category, message);
                if (category == VideoReceiver::ErrorCategory::Fatal && _source.needsIngestSession())
                    _playbackRuntime->markRefreshNeeded();
            });

    connect(frameDelivery(), &VideoFrameDelivery::videoSizeChanged, this, [this](QSize size) {
        _videoSize = size;
        emit videoSizeChanged(size);
    });
    connect(frameDelivery(), &VideoFrameDelivery::firstFrameReadyChanged,
            this, &VideoStreamSession::firstFrameReadyChanged);
    connect(frameDelivery(), &VideoFrameDelivery::watchdogTimeout, this, &VideoStreamSession::_onReceiverTimeout);
}

VideoFrameDelivery* VideoStreamSession::frameDelivery() const
{
    return _receiverResources ? _receiverResources->frameDelivery() : nullptr;
}

QObject* VideoStreamSession::frameDeliveryAsObject() const
{
    return frameDelivery();
}

VideoReceiver* VideoStreamSession::receiver() const
{
    return _receiverResources ? _receiverResources->receiver() : nullptr;
}

VideoRecorder* VideoStreamSession::recorder() const
{
    return _receiverResources ? _receiverResources->recorder() : nullptr;
}

QVideoSink* VideoStreamSession::videoSink() const
{
    return _receiverResources ? _receiverResources->videoSink() : nullptr;
}

VideoStreamSession::SessionState VideoStreamSession::sessionState() const
{
    return _lifecycle ? _lifecycle->sessionState() : SessionState::Stopped;
}

VideoStreamFsm::State VideoStreamSession::fsmState() const
{
    return _lifecycle ? _lifecycle->fsmState() : VideoStreamFsm::State::Idle;
}

VideoStreamStateMachine* VideoStreamSession::fsm() const
{
    return _lifecycle ? _lifecycle->fsm() : nullptr;
}

bool VideoStreamSession::started() const
{
    const VideoReceiver* activeReceiver = receiver();
    return activeReceiver && activeReceiver->started();
}

bool VideoStreamSession::streaming() const
{
    const VideoReceiver* activeReceiver = receiver();
    return activeReceiver && activeReceiver->isStreaming();
}

bool VideoStreamSession::decoding() const
{
    const VideoReceiver* activeReceiver = receiver();
    return activeReceiver && activeReceiver->isDecoding();
}

bool VideoStreamSession::firstFrameReady() const
{
    const VideoFrameDelivery* delivery = frameDelivery();
    return delivery && delivery->firstFrameReady();
}

bool VideoStreamSession::recording() const
{
    const VideoRecorder* activeRecorder = recorder();
    return activeRecorder && activeRecorder->isRecording();
}

bool VideoStreamSession::latencySupported() const
{
    const VideoReceiver* activeReceiver = receiver();
    return activeReceiver ? activeReceiver->latencySupported() : false;
}

bool VideoStreamSession::hwDecoding() const
{
    const VideoReceiver* activeReceiver = receiver();
    return activeReceiver && activeReceiver->hwDecoding();
}

QString VideoStreamSession::activeDecoderName() const
{
    const VideoReceiver* activeReceiver = receiver();
    return activeReceiver ? activeReceiver->activeDecoderName() : QString();
}

QString VideoStreamSession::lastError() const
{
    return _diagnostics ? _diagnostics->lastError() : QString();
}

VideoReceiver::ErrorCategory VideoStreamSession::lastErrorCategory() const
{
    return _diagnostics ? _diagnostics->lastErrorCategory() : VideoReceiver::ErrorCategory::Transient;
}

bool VideoStreamSession::setUri(const QString& uri)
{
    return setSourceDescriptor(VideoSourceResolver::describeUri(uri));
}

bool VideoStreamSession::setSourceDescriptor(const VideoSourceResolver::VideoSource& source)
{
    if (source.uri == _uri && !_uri.isNull())
        return false;

    _stopIngestSession();
    _source = source;
    _uri = _source.uri;
    _playbackRuntime->setSource(_source);
    _restartPolicy.noteSourceChanged();

    if (_receiverResources && _receiverResources->requiresReceiverRecreate(_source)) {
        qCDebug(VideoStreamSessionLog) << _name << "source cleared; destroying receiver";
        _destroyReceiver();
    }

    if (_hasSource()) {
        _ensureReceiver();
        if (receiver())
            receiver()->configureSource(_source);
        _applyPlaybackInputToReceiver();
    }

    emit uriChanged(_uri);
    return true;
}

bool VideoStreamSession::setLowLatency(bool lowLatency)
{
    VideoReceiver* activeReceiver = receiver();
    if (!activeReceiver || lowLatency == activeReceiver->lowLatency())
        return false;

    activeReceiver->setLowLatency(lowLatency);
    return true;
}

void VideoStreamSession::registerVideoSink(QVideoSink* sink)
{
    if (!_receiverResources)
        return;

    const VideoReceiverSession::SinkResult result = _receiverResources->registerVideoSink(sink);
    if (result.restartRequested) {
        qCDebug(VideoStreamSessionLog) << _name << "receiver requested restart after sink change";
        restart();
    }

    _connectStats(_receiverResources->frameDelivery());
    if (result.frameDeliveryChanged)
        emit frameDeliveryChanged();
}

uint32_t VideoStreamSession::_timeoutForUri() const
{
    return _source.startupTimeout();
}

void VideoStreamSession::start(uint32_t timeout)
{
    if (!_hasReceiver())
        return;

    if (!_isStopped()) {
        qCDebug(VideoStreamSessionLog) << "Cannot start" << _name << "state:" << static_cast<int>(sessionState());
        return;
    }

    if (!_refreshPlaybackInputForStart())
        return;

    qCDebug(VideoStreamSessionLog) << "Starting" << _name << "uri:" << _uri
                                   << "input:" << _playbackRuntime->input().uri;
    _restartPolicy.noteStartRequested();

    if (_diagnostics)
        _diagnostics->clearError();

    _lifecycle->requestStart(timeout);
}

void VideoStreamSession::stop()
{
    _restartPolicy.noteStopRequested();
    _stopReceiverIfStarted();
    _stopIngestSession();
}

void VideoStreamSession::_stopReceiverIfStarted()
{
    if (!_lifecycle)
        return;

    if (_isStoppingOrStopped())
        return;

    qCDebug(VideoStreamSessionLog) << "Stopping" << _name;
    _lifecycle->requestStop();
}

void VideoStreamSession::restart()
{
    if (!_hasPlaybackInput()) {
        if (!_isStopped())
            stop();
        return;
    }

    qCDebug(VideoStreamSessionLog) << "Restarting" << _name;
    _restartPolicy.requestRestart(sessionState(), _hasPlaybackInput(),
                                  [this]() { start(_timeoutForUri()); },
                                  [this]() { _stopReceiverIfStarted(); });
}

void VideoStreamSession::_onReceiverTimeout()
{
    if (!_hasReceiver())
        return;

    _restartPolicy.handleReceiverTimeout(_name, _source,
                                         [this]() {
                                             if (receiver())
                                                 receiver()->pause();
                                         },
                                         [this]() {
                                             if (receiver())
                                                 receiver()->resume();
                                         },
                                         [this]() { restart(); });
}

std::unique_ptr<VideoRecorder> VideoStreamSession::releaseRecorder()
{
    if (!_receiverResources)
        return {};

    std::unique_ptr<VideoRecorder> out = _receiverResources->releaseRecorder();
    _createRecorder();
    return out;
}

void VideoStreamSession::setRecorderForTest(VideoRecorder* recorder)
{
    if (_receiverResources)
        _receiverResources->setRecorderForTest(recorder);
}

void VideoStreamSession::setRecorderFactoryForTest(RecorderFactory factory)
{
    if (_receiverResources)
        _receiverResources->setRecorderFactoryForTest(std::move(factory));
}

void VideoStreamSession::setPlaybackInputResolverForTest(PlaybackInputResolver resolver)
{
    _playbackRuntime->setResolverForTest(std::move(resolver));
}

void VideoStreamSession::_ensureReceiver()
{
    if (!_receiverResources || _hasReceiver())
        return;

    const VideoReceiverSession::EnsureResult ensureResult = _receiverResources->ensureReceiver(_source);
    if (!ensureResult.created || !_hasReceiver())
        return;

    (void)_lifecycle->bind(receiver());

    emit receiverChanged();
    if (ensureResult.hadPendingSink)
        emit frameDeliveryChanged();
    _createRecorder();
}

void VideoStreamSession::_createRecorder()
{
    if (_receiverResources) {
        _receiverResources->createRecorder(_playbackRuntime
                                               ? _playbackRuntime->createIngestRecorder(_receiverResources)
                                               : std::unique_ptr<VideoRecorder>());
    }
}

void VideoStreamSession::_destroyReceiver()
{
    if (!_receiverResources || !_hasReceiver())
        return;

    (void)_receiverResources->destroyReceiver();
    emit receiverChanged();
    emit frameDeliveryChanged();

    if (_lifecycle)
        _lifecycle->destroy();

    _restartPolicy.noteStopRequested();
}

bool VideoStreamSession::_refreshPlaybackInputForStart()
{
    if (!_playbackRuntime->refreshForStart())
        return false;

    _applyPlaybackInputToReceiver();
    return _hasPlaybackInput();
}

bool VideoStreamSession::_hasPlaybackInput() const
{
    return _playbackRuntime && _playbackRuntime->hasInput();
}

void VideoStreamSession::_applyPlaybackInputToReceiver()
{
    if (_playbackRuntime)
        _playbackRuntime->applyToReceiver(receiver());
}

void VideoStreamSession::_stopIngestSession()
{
    if (_playbackRuntime)
        _playbackRuntime->stop();
}

bool VideoStreamSession::_isStoppingOrStopped() const
{
    const auto state = sessionState();
    return state == SessionState::Stopping || state == SessionState::Stopped;
}

void VideoStreamSession::_connectStats(VideoFrameDelivery* delivery)
{
    if (!delivery) {
        if (_stats) {
            _stats->setFrameDelivery(nullptr);
            _stats->reset();
        }
        return;
    }

    if (!_stats) {
        _stats = new VideoStreamStats(this);
        emit statsChanged();
    }

    _stats->setFrameDelivery(delivery);
}
