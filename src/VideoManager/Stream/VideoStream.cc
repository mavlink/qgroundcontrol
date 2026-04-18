#include "VideoStream.h"

#include "QGCLoggingCategory.h"
#include "QGCVideoStreamInfo.h"
#include "VideoFrameDelivery.h"
#include "VideoReceiver.h"
#include "VideoRecorder.h"
#include "VideoReceiverSession.h"
#include "VideoSettings.h"
#include "VideoSourceResolver.h"
#include "VideoStreamStateMachine.h"
#include "VideoStreamStats.h"

#include <array>

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
    : QObject(parent), _role(role), _name(nameForRole(role)), _factory(std::move(factory))
{
    _diagnostics = new VideoDiagnostics(this);
    connect(_diagnostics, &VideoDiagnostics::lastErrorChanged, this, &VideoStream::lastErrorChanged);
    _session = new VideoReceiverSession(_name, isThermal(), _factory, this);
    connect(_session->bridge(), &VideoFrameDelivery::videoSizeChanged, this, [this](QSize s) {
        _videoSize = s;
        emit videoSizeChanged(s);
    });
    connect(_session->bridge(), &VideoFrameDelivery::firstFrameReadyChanged,
            this, &VideoStream::firstFrameReadyChanged);
    // Bridge-centralized frame watchdog — fires when an armed backend stops
    // delivering frames. GStreamer uses StreamHealthMonitor internally and
    // does not arm the bridge watchdog; QtMultimedia/UVC arm it per-start.
    connect(_session->bridge(), &VideoFrameDelivery::watchdogTimeout, this, &VideoStream::_onReceiverTimeout);
    _wireSessionSignals();
    qCDebug(VideoStreamLog) << "Created stream" << _name << (isThermal() ? "(thermal)" : "");
}

VideoStream::VideoStream(Role role, const QString& name, ReceiverFactory factory, QObject* parent)
    : QObject(parent), _role(role), _name(name), _factory(std::move(factory))
{
    _diagnostics = new VideoDiagnostics(this);
    connect(_diagnostics, &VideoDiagnostics::lastErrorChanged, this, &VideoStream::lastErrorChanged);
    _session = new VideoReceiverSession(_name, isThermal(), _factory, this);
    connect(_session->bridge(), &VideoFrameDelivery::videoSizeChanged, this, [this](QSize s) {
        _videoSize = s;
        emit videoSizeChanged(s);
    });
    connect(_session->bridge(), &VideoFrameDelivery::firstFrameReadyChanged,
            this, &VideoStream::firstFrameReadyChanged);
    // Bridge-centralized frame watchdog — fires when an armed backend stops
    // delivering frames. GStreamer uses StreamHealthMonitor internally and
    // does not arm the bridge watchdog; QtMultimedia/UVC arm it per-start.
    connect(_session->bridge(), &VideoFrameDelivery::watchdogTimeout, this, &VideoStream::_onReceiverTimeout);
    _wireSessionSignals();
    qCDebug(VideoStreamLog) << "Created dynamic stream" << _name;
}

VideoStream::~VideoStream()
{
    _destroyReceiver();
    _session = nullptr;
}

// ═══════════════════════════════════════════════════════════════════════════
// FSM state mapping
// ═══════════════════════════════════════════════════════════════════════════

/*static*/ VideoStream::SessionState VideoStream::_mapFsmState(VideoStreamFsm::State fsmState)
{
    using FsmState = VideoStreamFsm::State;
    switch (fsmState) {
        case FsmState::Idle:
        case FsmState::Failed:
            return SessionState::Stopped;
        case FsmState::Starting:
            return SessionState::Starting;
        case FsmState::Connected:
        case FsmState::Streaming:
        case FsmState::Paused:
        case FsmState::Reconnecting:
            return SessionState::Running;
        case FsmState::Stopping:
            return SessionState::Stopping;
    }
    return SessionState::Stopped;
}

VideoStream::SessionState VideoStream::sessionState() const
{
    if (!_fsm)
        return SessionState::Stopped;
    return _mapFsmState(_fsm->state());
}

VideoStreamFsm::State VideoStream::fsmState() const
{
    return _fsm ? _fsm->state() : VideoStreamFsm::State::Idle;
}

static VideoStreamStateMachine::Policy _fsmPolicyForRoleImpl(VideoStream::Role role)
{
    struct RolePolicy
    {
        bool allowSoftReconnect = true;
        int circuitFailureThreshold = 3;
        int circuitResetTimeoutMs = 8000;
    };
    static constexpr std::array<RolePolicy, VideoStream::RoleCount> kPolicies = {{
        {true, 3, 8000},    // Primary
        {true, 3, 8000},    // Thermal
        {false, 1, 0},      // UVC: local capture reconnect is driven by QMediaDevices.
        {false, 1, 30000},  // Dynamic: fail bad MAVLink URIs quickly.
    }};
    const RolePolicy rolePolicy = kPolicies.at(static_cast<size_t>(role));
    VideoStreamStateMachine::Policy policy;
    policy.allowSoftReconnect = rolePolicy.allowSoftReconnect;
    policy.circuitFailureThreshold = rolePolicy.circuitFailureThreshold;
    policy.circuitResetTimeoutMs = rolePolicy.circuitResetTimeoutMs;
    return policy;
}

// ═══════════════════════════════════════════════════════════════════════════
// Receiver access
// ═══════════════════════════════════════════════════════════════════════════

VideoFrameDelivery* VideoStream::bridge() const
{
    return _session ? _session->bridge() : nullptr;
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
    return receiver() && receiver()->started();
}

bool VideoStream::streaming() const
{
    return receiver() && receiver()->isStreaming();
}

bool VideoStream::decoding() const
{
    return receiver() && receiver()->isDecoding();
}

bool VideoStream::firstFrameReady() const
{
    return bridge() && bridge()->firstFrameReady();
}

bool VideoStream::recording() const
{
    return recorder() && recorder()->isRecording();
}

bool VideoStream::latencySupported() const
{
    return receiver() ? receiver()->latencySupported() : false;
}

QString VideoStream::lastError() const
{
    return _diagnostics ? _diagnostics->lastError() : QString();
}

VideoReceiver::ErrorCategory VideoStream::lastErrorCategory() const
{
    return _diagnostics ? _diagnostics->lastErrorCategory() : VideoReceiver::ErrorCategory::Transient;
}

QVideoSink* VideoStream::pendingSink() const
{
    return _session ? _session->videoSink() : nullptr;
}

// ═══════════════════════════════════════════════════════════════════════════
// Stream info
// ═══════════════════════════════════════════════════════════════════════════

void VideoStream::setVideoStreamInfo(QGCVideoStreamInfo* info)
{
    if (_streamInfo == info)
        return;

    // Disconnect the old info before swapping (#2: stale infoChanged connection).
    if (_streamInfo)
        disconnect(_streamInfo, &QGCVideoStreamInfo::infoChanged, this, &VideoStream::streamInfoUpdated);

    _streamInfo = info;

    if (_streamInfo)
        (void)connect(_streamInfo, &QGCVideoStreamInfo::infoChanged, this, &VideoStream::streamInfoUpdated);

    // Dynamic streams derive their URI solely from MAVLink VIDEO_STREAM_INFORMATION
    // (Primary/Thermal still go through updateFromSettings so the settings-writeback
    // path can run). Apply the auto-stream URI here so the orchestrator doesn't need
    // a separate invocation after setVideoStreamInfo().
    if (_role == Role::Dynamic && _streamInfo)
        _updateAutoStream();

    emit videoStreamInfoChanged();
}

// ═══════════════════════════════════════════════════════════════════════════
// Sink management
// ═══════════════════════════════════════════════════════════════════════════

void VideoStream::registerVideoSink(QVideoSink* sink)
{
    if (!_session)
        return;

    const VideoReceiverSession::SinkResult result = _session->registerVideoSink(sink);
    if (result.restartRequested) {
        qCDebug(VideoStreamLog) << _name << "late sink attach on live GStreamer pipeline — restarting";
        restart();
    }

    // Wire per-stream stats to the bridge (idempotent — _connectStats no-ops if already wired).
    _connectStats(_session->bridge());
    // bridgeChanged only fires when the bridge has a live receiver attached —
    // registering a sink before the receiver exists is a deferred flush, not a
    // bridge change. `_ensureReceiver()` emits bridgeChanged when the pending
    // sink goes live on the new receiver.
    if (result.bridgeChanged)
        emit bridgeChanged();
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
    if (source.uri == _uri && !_uri.isNull() && source.preferredBackend == _source.preferredBackend)
        return false;

    _source = source;
    _uri = _source.uri;
    _restartPolicy.noteSourceChanged();

    if (_session && _session->requiresReceiverRecreate(_source)) {
        qCDebug(VideoStreamLog) << _name << "backend switch required:"
                                << static_cast<int>(receiver()->kind()) << "->"
                                << static_cast<int>(_source.preferredBackend);

        // The delivery endpoint and its registered sink survive the backend switch.
        _destroyReceiver();
    }

    // Create receiver on demand if we have a URI
    if (_source.isValid()) {
        _ensureReceiver();
        if (receiver())
            receiver()->setUri(_uri);
    }

    emit uriChanged(_uri);
    return true;
}

bool VideoStream::_updateAutoStream(QString* outSource)
{
    if (!_streamInfo)
        return false;

    qCDebug(VideoStreamLog) << QString("Configure auto-stream (%1):").arg(_name) << _streamInfo->uri();

    auto resolved = VideoSourceResolver::fromStreamInfo(_streamInfo);
    if (outSource)
        *outSource = resolved.sourceName;

    return setSourceDescriptor(resolved);
}

bool VideoStream::updateFromSettings(VideoSettings* settings)
{
    bool changed = false;

    const bool lowLatency = settings->lowLatencyMode()->rawValue().toBool();
    if (receiver() && lowLatency != receiver()->lowLatency()) {
        receiver()->setLowLatency(lowLatency);
        changed = true;
    }

    // Thermal streams only get low-latency; URI comes from auto-stream only
    if (isThermal())
        return changed | _updateAutoStream();

    // Settings writeback uses QSignalBlocker to break the
    // rawValueChanged → _videoSourceChanged → updateFromSettings re-entrance loop.
    QString resolvedSource;
    const bool autoChanged = _updateAutoStream(&resolvedSource);
    if (autoChanged && !resolvedSource.isEmpty()) {
        { QSignalBlocker b(settings->videoSource()); settings->videoSource()->setRawValue(resolvedSource); }
        if (resolvedSource == QLatin1String(VideoSettings::videoSourceRTSP)) {
            QSignalBlocker b(settings->rtspUrl()); settings->rtspUrl()->setRawValue(_uri);
        }
    }
    changed |= autoChanged;

    // Settings-driven source resolution (table + special cases)
    auto resolved = VideoSourceResolver::fromSettings(settings);
    changed |= setSourceDescriptor(resolved);

    return changed;
}

// ═══════════════════════════════════════════════════════════════════════════
// Lifecycle
// ═══════════════════════════════════════════════════════════════════════════

uint32_t VideoStream::_timeoutForUri() const
{
    return static_cast<uint32_t>(_source.startupTimeoutS);
}

void VideoStream::start(uint32_t timeout)
{
    if (!receiver() || _uri.isEmpty())
        return;

    if (sessionState() != SessionState::Stopped) {
        qCDebug(VideoStreamLog) << "Cannot start" << _name << "— state:" << static_cast<int>(sessionState());
        return;
    }

    qCDebug(VideoStreamLog) << "Starting" << _name << "uri:" << _uri;
    _restartPolicy.noteStartRequested();

    // Fresh start attempt — clear any error surfaced by the previous session.
    if (_diagnostics)
        _diagnostics->clearError();

    // The FSM's Starting onEntry will call _receiver->start(timeout).
    _fsm->setStartTimeoutS(static_cast<int>(timeout));
    _fsm->requestStart();
}

void VideoStream::stop()
{
    // Public stop is an explicit user/caller request — clears auto-restart
    // intent so the receiverFullyStopped handler does NOT re-launch.
    _restartPolicy.noteStopRequested();
    _stopReceiverIfStarted();
}

void VideoStream::_stopReceiverIfStarted()
{
    if (!_fsm)
        return;

    // Stopping and Stopped (Idle/Failed) are already quiescent — no-op.
    const auto st = sessionState();
    if (st == SessionState::Stopping || st == SessionState::Stopped)
        return;

    qCDebug(VideoStreamLog) << "Stopping" << _name;
    // The FSM's Stopping onEntry will call _receiver->stop().
    _fsm->requestStop();
}

void VideoStream::restart()
{
    if (_uri.isEmpty()) {
        if (sessionState() != SessionState::Stopped)
            stop();
        return;
    }

    qCDebug(VideoStreamLog) << "Restarting" << _name;
    _restartPolicy.requestRestart(sessionState(), !_uri.isEmpty(),
                                  [this]() { start(_timeoutForUri()); },
                                  [this]() { _stopReceiverIfStarted(); });
}

void VideoStream::_onReceiverTimeout()
{
    if (!receiver())
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

std::unique_ptr<VideoRecorder> VideoStream::releaseRecorder()
{
    if (!_session)
        return {};

    std::unique_ptr<VideoRecorder> out = _session->releaseRecorder();

    // Rebuild a fresh recorder on this stream so `recorder()` stays valid.
    // The new instance is wired via _createRecorder's connect() block.
    _createRecorder();
    return out;
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

// ═══════════════════════════════════════════════════════════════════════════
// Receiver management — lazy factory
// ═══════════════════════════════════════════════════════════════════════════

void VideoStream::_ensureReceiver()
{
    if (!_session || receiver())
        return;

    const VideoReceiverSession::EnsureResult ensureResult = _session->ensureReceiver(_source);
    if (!ensureResult.created || !receiver())
        return;

    // Build the authoritative FSM and bind it to the new receiver.
    _fsm = std::make_unique<VideoStreamStateMachine>(_name + QStringLiteral("/fsm"), _fsmPolicyForRoleImpl(_role), this);

    // Re-emit FSM state changes as both fsmStateChanged and sessionStateChanged.
    // sessionStateChanged only fires when the mapped coarse state actually changes
    // (Connected → Streaming both map to Running, so no duplicate emission).
    connect(_fsm.get(), &VideoStreamStateMachine::stateChanged, this,
            [this](VideoStreamStateMachine::State newFsmState) {
                emit fsmStateChanged(newFsmState);
                const SessionState mapped = _mapFsmState(newFsmState);
                if (mapped != _lastMappedState) {
                    _lastMappedState = mapped;
                    emit sessionStateChanged(mapped);
                }
            });

    // Hook the FSM's high-level lifecycle signals.

    // receiverFullyStarted: pipeline up + setStarted(true) + startDecoding() done.
    connect(_fsm.get(), &VideoStreamStateMachine::receiverFullyStarted, this, [this]() {
        _restartPolicy.noteReceiverFullyStarted();
    });

    // receiverFullyStopped: receiver quiescent after a stop cycle.
    connect(_fsm.get(), &VideoStreamStateMachine::receiverFullyStopped, this, [this]() {
        _restartPolicy.handleReceiverFullyStopped(_name, _uri, !_uri.isEmpty(),
                                                  [this]() { start(_timeoutForUri()); });
    });

    // reconnectRequested: FSM entered Reconnecting — schedule a retry.
    // The retry must transition the FSM Reconnecting → Starting directly
    // (via requestStart) rather than going through VideoStream::start(),
    // which short-circuits unless sessionState() is Stopped (and Reconnecting
    // maps to Running). The Reconnecting→Starting transition re-fires the
    // Starting onEntry which calls _receiver->start().
    connect(_fsm.get(), &VideoStreamStateMachine::reconnectRequested, this, [this]() {
        _restartPolicy.handleReconnectRequested(_name, !_uri.isEmpty(), [this]() {
            if (!_fsm || _uri.isEmpty())
                return;
            _fsm->setStartTimeoutS(static_cast<int>(_timeoutForUri()));
            _fsm->requestStart();
        });
    });

    (void)_fsm->bind(receiver());
    _fsm->start();

    // Reset the mapped-state tracker to Stopped for the fresh FSM.
    _lastMappedState = SessionState::Stopped;

    emit receiverChanged();  // latencySupported may have changed
    // A deferred sink (set before any receiver existed) is now live on the
    // new receiver — QML bindings on `bridge` must refresh.
    if (ensureResult.hadPendingSink)
        emit bridgeChanged();
    _createRecorder();
}

void VideoStream::_createRecorder()
{
    if (_session)
        _session->createRecorder();
}

void VideoStream::_destroyRecorder()
{
    if (_session)
        _session->destroyRecorder();
}

void VideoStream::_destroyReceiver()
{
    if (!_session || !receiver())
        return;

    (void) _session->destroyReceiver();
    emit receiverChanged();  // latencySupported is now false (no receiver)
    // Bridge remains valid and sink-connected across backend switches.
    // Emit bridgeChanged so QML re-evaluates bindings on the (still-live) bridge object.
    emit bridgeChanged();

    // Tear down the FSM alongside the receiver.
    if (_fsm) {
        const SessionState preDestroy = _mapFsmState(_fsm->state());
        if (_fsm->isRunning())
            _fsm->stop();
        _fsm.reset();
        // Emit synthetic fsmStateChanged(Idle) so QML bindings on `fsmState` reset.
        emit fsmStateChanged(VideoStreamStateMachine::State::Idle);
        // Only emit sessionStateChanged if the mapped state was not already Stopped.
        if (preDestroy != SessionState::Stopped) {
            _lastMappedState = SessionState::Stopped;
            emit sessionStateChanged(SessionState::Stopped);
        }
    }

    _restartPolicy.noteStopRequested();
}

void VideoStream::_wireSessionSignals()
{
    if (!_session)
        return;

    connect(_session, &VideoReceiverSession::streamingChanged, this, &VideoStream::streamingChanged);
    connect(_session, &VideoReceiverSession::decodingChanged, this, [this](bool active) {
        emit decodingChanged(active);
        emit hwDecodingChanged();
        emit activeDecoderNameChanged();
    });
    connect(_session, &VideoReceiverSession::timeout, this, &VideoStream::_onReceiverTimeout);
    connect(_session, &VideoReceiverSession::lateSinkRestartRequested, this, [this]() {
        qCDebug(VideoStreamLog) << _name << "late sink attach — triggering restart";
        restart();
    });
    connect(_session, &VideoReceiverSession::recordingStarted, this, &VideoStream::recordingStarted);
    connect(_session, &VideoReceiverSession::recordingChanged, this, &VideoStream::recordingChanged);
    connect(_session, &VideoReceiverSession::recordingError, this, &VideoStream::recordingError);
    connect(_session, &VideoReceiverSession::recorderChanged, this, &VideoStream::recorderChanged);

    connect(_session, &VideoReceiverSession::receiverError, this,
            [this](VideoReceiver::ErrorCategory category, const QString& message) {
                if (_diagnostics)
                    _diagnostics->setError(category, message);
            });
}

// ═══════════════════════════════════════════════════════════════════════════
// Per-stream stats
// ═══════════════════════════════════════════════════════════════════════════

void VideoStream::_connectStats(VideoFrameDelivery* delivery)
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

        connect(_stats, &VideoStreamStats::fpsChanged, this, &VideoStream::fpsChanged);
        connect(_stats, &VideoStreamStats::streamHealthChanged, this,
                [this](VideoStreamStats::Health health) { emit streamHealthChanged(static_cast<int>(health)); });
        connect(_stats, &VideoStreamStats::latencyChanged, this, &VideoStream::latencyMsChanged);
        connect(_stats, &VideoStreamStats::fpsChanged, this, [this]() {
            if (VideoFrameDelivery* currentBridge = this->bridge())
                emit droppedFramesChanged(currentBridge->droppedFrames());
        });
    }

    _stats->setFrameDelivery(delivery);
}

float VideoStream::fps() const
{
    return _stats ? _stats->fps() : 0.0F;
}

int VideoStream::streamHealth() const
{
    return _stats ? static_cast<int>(_stats->streamHealth()) : 0;
}

float VideoStream::latencyMs() const
{
    return bridge() ? bridge()->latencyMs() : -1.0F;
}

quint64 VideoStream::droppedFrames() const
{
    return bridge() ? bridge()->droppedFrames() : 0;
}

bool VideoStream::hwDecoding() const
{
    return receiver() && receiver()->hwDecoding();
}

QString VideoStream::activeDecoderName() const
{
    return receiver() ? receiver()->activeDecoderName() : QString();
}

QObject* VideoStream::bridgeAsObject() const
{
    return bridge();
}

QObject* VideoStream::frameDeliveryAsObject() const
{
    return frameDelivery();
}
