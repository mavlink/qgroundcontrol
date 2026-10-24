#include "GstVideoReceiver.h"

#include <algorithm>
#include <chrono>
#include <optional>
#include <utility>

#include <QtCore/QCoreApplication>
#include <QtCore/QDeadlineTimer>
#include <QtCore/QMutex>
#include <QtCore/QThread>
#include <QtCore/QTimer>
#include <QtMultimedia/QVideoSink>

#include "GStreamer.h"
#include "GstSourceFactory.h"
#include "PipelineDotDumper.h"
#include "PipelineSession.h"
#include "QGCLoggingCategory.h"
#include "QGCNetworkHelper.h"
#include "QGCQVideoSinkController.h"
#include "StreamWatchdog.h"
#include "gstqgc/gstqgcqvideosink.h"

QGC_LOGGING_CATEGORY(GstVideoReceiverLog, "Video.GStreamer.GstVideoReceiver")

namespace {

constexpr std::chrono::milliseconds kWatchdogInterval{1000};
// The live GPU path counters are logged every this many watchdog ticks.
constexpr int kHwStatsLogIntervalTicks = 10;
// The destructor's wait for the worker: a recording's finalization plus time for the pipeline to stop.
constexpr std::chrono::seconds kWorkerShutdownTimeout = RecordingBranch::kFinalizeTimeout + std::chrono::seconds{2};

/// "<base>_part<part>.<ext>" for @p file "<base>.<ext>".
QString partFile(const QString& file, int part)
{
    const QString suffix = QStringLiteral("_part%1").arg(part);
    const qsizetype dot = file.lastIndexOf(u'.');
    const qsizetype separator = std::max(file.lastIndexOf(u'/'), file.lastIndexOf(u'\\'));
    return (dot > separator) ? (file.left(dot) + suffix + file.mid(dot)) : (file + suffix);
}

}  // namespace

/// The worker's way back to the receiver: queued calls on the GUI thread, which stop once the receiver detaches.
class GstReceiverLink
{
public:
    explicit GstReceiverLink(GstVideoReceiver* receiver)
        : _receiver(receiver)
    {}

    void post(std::function<void(GstVideoReceiver&)> task)
    {
        const QMutexLocker locker(&_mutex);
        if (GstVideoReceiver* const receiver = _receiver) {
            (void) QMetaObject::invokeMethod(
                receiver, [receiver, task = std::move(task)]() { task(*receiver); }, Qt::QueuedConnection);
        }
    }

    void detach()
    {
        const QMutexLocker locker(&_mutex);
        _receiver = nullptr;
    }

private:
    QMutex _mutex;
    GstVideoReceiver* _receiver = nullptr;
};

/// Lives on the receiver's worker thread: holds what the receiver was asked for (active, config, video sink,
/// recording) and keeps a PipelineSession in line with it, reconnecting with backoff after a failure.
class GstReceiverWorker : public QObject, private PipelineSession::Listener
{
public:
    explicit GstReceiverWorker(std::shared_ptr<GstReceiverLink> link);
    ~GstReceiverWorker() override;

    void setActive(bool active, const QString& name, const VideoReceiver::Config& config);
    void setConfig(const VideoReceiver::Config& config);
    void restart(const std::shared_ptr<GstElement>& videoSink);
    void setVideoSink(const std::shared_ptr<GstElement>& videoSink);
    void startRecording(const QString& file, VideoReceiver::FileFormat format);
    void stopRecording();
    void dumpPipelineGraph(const QString& tag) const;
    /// Stops everything and quits the thread.
    void shutdown();

    GstElement* pipeline() const { return _session ? _session->pipeline() : nullptr; }

private:
    /// What startRecording() asked for; after a reconnect it continues in the next part.
    struct Recording
    {
        QString file;
        VideoReceiver::FileFormat format = VideoReceiver::FileFormat::Mkv;
        int part = 1;
        bool opened = false;  ///< The current part has a frame.

        QString currentFile() const { return (part == 1) ? file : partFile(file, part); }
    };

    void _reconcile();
    /// Stops the stream and connects again now.
    void _restartStream();
    void _startSession();
    void _stopSession();
    /// After a failure: reconnects with backoff, or gives up.
    void _recover(const PipelineSession::Failure& failure);
    void _halt(const QString& message, bool permanent);
    void _endRecording();
    std::optional<RecordingBranch::Request> _recordingRequest() const;
    void _onWatchdogTick();
    void _setState(VideoReceiver::State state);

    QString _redactedUri() const { return QGCNetworkHelper::redactedUrlForLogging(_config.uri); }

    void onSessionStreaming() override;
    void onSessionFailed(const PipelineSession::Failure& failure) override;
    void onDecodingChanged(bool decoding) override;
    void onDecoderNameChanged(const QString& name) override;
    void onDecoderFailing() override;
    void onRecordingFileOpened(const QString& file) override;
    void onRecordingFileClosed(const QString& file, const QString& error) override;
    void onRecordingFailed(const QString& file, const QString& error, bool fileOpened) override;

    const std::shared_ptr<GstReceiverLink> _link;
    QTimer* const _watchdogTimer;
    QTimer* const _reconnectTimer;
    StreamWatchdog _watchdog;
    PipelineDotDumper _dotDumper;

    bool _active = false;
    VideoReceiver::Config _config;
    std::shared_ptr<GstElement> _videoSink;
    std::optional<Recording> _recording;

    PipelineSession* _session = nullptr;
    /// Counts sessions, so a failure reported by one that is gone by then is ignored.
    quint64 _sessionSerial = 0;
    /// Given up after a failure until the stream is asked for anew.
    bool _halted = false;
    bool _haltedPermanently = false;
    QString _lastFailure;
    VideoReceiver::State _state = VideoReceiver::State::Stopped;
    int _statsTicks = 0;
};

GstReceiverWorker::GstReceiverWorker(std::shared_ptr<GstReceiverLink> link)
    : _link(std::move(link))
    , _watchdogTimer(new QTimer(this))
    , _reconnectTimer(new QTimer(this))
{
    _watchdogTimer->setInterval(kWatchdogInterval);
    _reconnectTimer->setSingleShot(true);
    // Backoff delays are asserted by tests; a coarse timer may fire up to 5% early.
    _reconnectTimer->setTimerType(Qt::PreciseTimer);
    (void) connect(_watchdogTimer, &QTimer::timeout, this, [this]() { _onWatchdogTick(); });
    (void) connect(_reconnectTimer, &QTimer::timeout, this, [this]() {
        qCDebug(GstVideoReceiverLog) << "Reconnecting (attempt" << _watchdog.reconnectAttempts() << ")"
                                     << _redactedUri();
        _reconcile();
    });
}

GstReceiverWorker::~GstReceiverWorker()
{
    _stopSession();
}

void GstReceiverWorker::setActive(bool active, const QString& name, const VideoReceiver::Config& config)
{
    _active = active;
    _config = config;
    _halted = false;
    _reconnectTimer->stop();
    _watchdog.resetBackoff();
    if (active) {
        _dotDumper.setReceiverName(name);
    } else {
        _endRecording();
    }
    _reconcile();
}

void GstReceiverWorker::setConfig(const VideoReceiver::Config& config)
{
    VideoReceiver::Config sameStream = std::exchange(_config, config);
    sameStream.autoReconnect = config.autoReconnect;
    if (!_active) {
        return;
    }

    if (sameStream != config) {
        _restartStream();
        return;
    }

    // Only autoReconnect changed, which applies at once.
    if (!config.autoReconnect && _reconnectTimer->isActive()) {
        _halt(_lastFailure, false);
    } else if (config.autoReconnect && _halted && !_haltedPermanently) {
        _halted = false;
        _reconcile();
    }
}

void GstReceiverWorker::restart(const std::shared_ptr<GstElement>& videoSink)
{
    _videoSink = videoSink;
    if (_active) {
        _restartStream();
    }
}

void GstReceiverWorker::setVideoSink(const std::shared_ptr<GstElement>& videoSink)
{
    _videoSink = videoSink;
    if (_session) {
        _session->setVideoSink(_videoSink.get());
    }
}

void GstReceiverWorker::startRecording(const QString& file, VideoReceiver::FileFormat format)
{
    if (!_active) {
        return;
    }
    if (_halted) {
        const QString message =
            QCoreApplication::translate("GstVideoReceiver", "Unable to record video: the video stream has stopped.");
        _link->post([message](GstVideoReceiver& receiver) { emit receiver.errorOccurred(message); });
        return;
    }

    _recording = Recording{.file = file, .format = format};
    _link->post([](GstVideoReceiver& receiver) { receiver.setRecording(true); });
    if (_session) {
        _session->setRecording(_recordingRequest());
    }
}

void GstReceiverWorker::stopRecording()
{
    if (!_recording) {
        return;
    }
    _endRecording();
    if (_session) {
        _session->setRecording(std::nullopt);
    }
}

void GstReceiverWorker::dumpPipelineGraph(const QString& tag) const
{
    if (!_session) {
        qCDebug(GstVideoReceiverLog) << "No running pipeline to dump";
        return;
    }
    _session->snapshot(tag);
}

void GstReceiverWorker::shutdown()
{
    _active = false;
    _reconnectTimer->stop();
    _stopSession();
    thread()->quit();
}

void GstReceiverWorker::_reconcile()
{
    if (!_active || _config.uri.isEmpty()) {
        _reconnectTimer->stop();
        _stopSession();
        _setState(VideoReceiver::State::Stopped);
        return;
    }
    if (_session || _halted || _reconnectTimer->isActive()) {
        return;
    }
    _startSession();
}

void GstReceiverWorker::_restartStream()
{
    _reconnectTimer->stop();
    _halted = false;
    _watchdog.resetBackoff();
    _stopSession();
    _reconcile();
}

void GstReceiverWorker::_startSession()
{
    const PipelineSession::Settings settings{
        .uri = _config.uri,
        .redactedUri = _redactedUri(),
        .lowLatency = _config.lowLatency,
        .rtpJitterLatencyMs = _config.rtpJitterLatencyMs,
    };
    _session = new PipelineSession(settings, *this, _watchdog, _dotDumper, this);
    ++_sessionSerial;
    _watchdog.start(_config.timeout, GStreamer::SourceFactory::connectGracePeriod(_config.uri));

    if (!_session->start()) {
        const PipelineSession::Failure failure = _session->failure();
        _stopSession();
        _recover(failure);
        return;
    }

    _setState(VideoReceiver::State::Connecting);
    _session->setVideoSink(_videoSink.get());
    _session->setRecording(_recordingRequest());
    _statsTicks = 0;
    _watchdogTimer->start();
}

void GstReceiverWorker::_stopSession()
{
    if (!_session) {
        return;
    }
    _watchdogTimer->stop();
    PipelineSession* const session = std::exchange(_session, nullptr);
    session->shutdown();
    // Deleted from the event loop: this may run inside one of its calls. Its leftover work is dropped with it.
    session->deleteLater();

    if (_recording && _recording->opened) {
        ++_recording->part;
        _recording->opened = false;
    }
}

void GstReceiverWorker::_recover(const PipelineSession::Failure& failure)
{
    _lastFailure = failure.message;
    if (failure.permanent || !_config.autoReconnect) {
        _halt(failure.message, failure.permanent);
        return;
    }

    const std::chrono::seconds delay = _watchdog.nextReconnectDelay();
    qCDebug(GstVideoReceiverLog) << "Scheduling reconnect #" << _watchdog.reconnectAttempts() << "in" << delay.count()
                                 << "s after" << failure.reason << _redactedUri();
    _reconnectTimer->start(delay);
    _setState(VideoReceiver::State::Reconnecting);
}

void GstReceiverWorker::_halt(const QString& message, bool permanent)
{
    _halted = true;
    _haltedPermanently = permanent;
    _reconnectTimer->stop();
    qCWarning(GstVideoReceiverLog) << "Video stream stopped:" << message;
    _endRecording();
    _setState(VideoReceiver::State::Stopped);
    _link->post([message](GstVideoReceiver& receiver) { emit receiver.errorOccurred(message); });
}

void GstReceiverWorker::_endRecording()
{
    if (!_recording) {
        return;
    }
    _recording.reset();
    _link->post([](GstVideoReceiver& receiver) { receiver.setRecording(false); });
}

std::optional<RecordingBranch::Request> GstReceiverWorker::_recordingRequest() const
{
    if (!_recording) {
        return std::nullopt;
    }
    return RecordingBranch::Request{.file = _recording->currentFile(), .format = _recording->format};
}

void GstReceiverWorker::_onWatchdogTick()
{
    if (!_session) {
        return;
    }

    if (++_statsTicks >= kHwStatsLogIntervalTicks) {
        _statsTicks = 0;
        if (const GstQgc::PathStats hwStats = _session->sinkPathStats(); hwStats.totalDelivered > 0) {
            qCDebug(GstVideoReceiverLog).noquote() << "GPU path live" << _redactedUri() << hwStats.line;
        }
    }

    const StreamWatchdog::Result result = _watchdog.check(StreamWatchdog::now(), _session->displaying());
    switch (result.stall) {
        case StreamWatchdog::Stall::None:
            return;
        case StreamWatchdog::Stall::Decoder:
            qCDebug(GstVideoReceiverLog) << "Video decoder timeout, no frames for" << result.silentMs << "ms"
                                         << _redactedUri();
            _session->snapshot(QStringLiteral("pipeline-decoder-timeout"));
            _session->restartDecoder("decoder watchdog");
            return;
        case StreamWatchdog::Stall::Source:
            qCDebug(GstVideoReceiverLog) << "Stream timeout, no frames for" << result.silentMs << "ms"
                                         << _redactedUri();
            _session->snapshot(QStringLiteral("pipeline-watchdog-timeout"));
            _stopSession();
            _recover({.reason = QStringLiteral("source watchdog"),
                      .message = QCoreApplication::translate("GstVideoReceiver", "No video received for %1 s.")
                                     .arg(result.silentMs / 1000),
                      .permanent = false});
            return;
    }
}

void GstReceiverWorker::_setState(VideoReceiver::State state)
{
    if (state == _state) {
        return;
    }
    _state = state;
    _link->post([state](GstVideoReceiver& receiver) { receiver.setState(state); });
}

void GstReceiverWorker::onSessionStreaming()
{
    _setState(VideoReceiver::State::Streaming);
}

void GstReceiverWorker::onSessionFailed(const PipelineSession::Failure& failure)
{
    qCDebug(GstVideoReceiverLog) << "Stream failed:" << failure.reason << _redactedUri();
    // Later, from the event loop: the session may be in the middle of a call.
    const quint64 serial = _sessionSerial;
    (void) QMetaObject::invokeMethod(
        this,
        [this, serial, failure]() {
            if (_session && (serial == _sessionSerial)) {
                _stopSession();
                _recover(failure);
            }
        },
        Qt::QueuedConnection);
}

void GstReceiverWorker::onDecodingChanged(bool decoding)
{
    _link->post([decoding](GstVideoReceiver& receiver) {
        receiver.setDecoding(decoding);
        if (!decoding) {
            receiver._releaseDisplayedFrame();
        }
    });
}

void GstReceiverWorker::onDecoderNameChanged(const QString& name)
{
    _link->post([name](GstVideoReceiver& receiver) { receiver._setDecoderName(name); });
}

void GstReceiverWorker::onDecoderFailing()
{
    onSessionFailed({.reason = QStringLiteral("decoder failing"),
                     .message = QCoreApplication::translate("GstVideoReceiver", "The video decoder keeps failing."),
                     .permanent = false});
}

void GstReceiverWorker::onRecordingFileOpened(const QString& file)
{
    if (_recording && (file == _recording->currentFile())) {
        _recording->opened = true;
    }
    _link->post([file](GstVideoReceiver& receiver) { emit receiver.recordingStarted(file); });
}

void GstReceiverWorker::onRecordingFileClosed(const QString& file, const QString& error)
{
    _link->post([file, error](GstVideoReceiver& receiver) { emit receiver.recordingFinished(file, error); });
}

void GstReceiverWorker::onRecordingFailed(const QString& file, const QString& error, bool fileOpened)
{
    if (_recording && (file == _recording->currentFile())) {
        _endRecording();
    }
    // A file that opened already reported the error through recordingFinished().
    if (!fileOpened) {
        const QString message =
            QCoreApplication::translate("GstVideoReceiver", "Video recording failed: %1").arg(error);
        _link->post([message](GstVideoReceiver& receiver) { emit receiver.errorOccurred(message); });
    }
}

GstVideoReceiver::GstVideoReceiver(QObject* parent)
    : VideoReceiver(parent)
    , _link(std::make_shared<GstReceiverLink>(this))
    , _workerThread(new QThread(this))
    , _worker(new GstReceiverWorker(_link))
{
    qCDebug(GstVideoReceiverLog) << this;

    _workerThread->setObjectName(QStringLiteral("GstVideoReceiver"));
    _worker->moveToThread(_workerThread);
    (void) connect(_workerThread, &QThread::finished, _worker, &QObject::deleteLater);
    _workerThread->start();
}

GstVideoReceiver::~GstVideoReceiver()
{
    _postToWorker([](GstReceiverWorker& worker) { worker.shutdown(); });
    if (_workerThread->wait(QDeadlineTimer(kWorkerShutdownTimeout))) {
        qCDebug(GstVideoReceiverLog) << this;
        return;
    }

    // Only a recorder stuck in the kernel (a hung network mount) holds the worker this long. It reaches this receiver
    // only through the link, so cutting that lets the worker finish on its own and clean up after itself.
    qCWarning(GstVideoReceiverLog) << "Worker still busy after"
                                   << std::chrono::milliseconds(kWorkerShutdownTimeout).count()
                                   << "ms; leaving it to finish on its own";
    _link->detach();
    _workerThread->setParent(nullptr);
    (void) connect(_workerThread, &QThread::finished, _workerThread, &QObject::deleteLater);
    if (_workerThread->isFinished()) {
        delete _workerThread;
    }
}

void GstVideoReceiver::dumpPipelineGraph(const QString& tag)
{
    _postToWorker([tag](GstReceiverWorker& worker) { worker.dumpPipelineGraph(tag); });
}

void GstVideoReceiver::onConfigChanged(const Config& /*previous*/)
{
    _postToWorker([config = config()](GstReceiverWorker& worker) { worker.setConfig(config); });
}

void GstVideoReceiver::onActiveChanged()
{
    _postToWorker([active = active(), name = name(), config = config()](GstReceiverWorker& worker) {
        worker.setActive(active, name, config);
    });
}

void GstVideoReceiver::onRestartRequested()
{
    // Construct-only sink settings apply to a new sink only.
    _releaseDisplaySink();
    if (QVideoSink* const sink = videoSink(); sink && _buildDisplaySink(sink)) {
        _bindVideoSink(sink);
    }
    _postToWorker(
        [sink = GStreamer::shareObject(_displaySink.get())](GstReceiverWorker& worker) { worker.restart(sink); });
}

void GstVideoReceiver::_scheduleVideoSinkUpdate()
{
    // Coalesced: an output re-attached in one go (setVideoOutput(nullptr), then the item) doesn't touch the pipeline.
    if (_videoSinkUpdatePending) {
        return;
    }
    _videoSinkUpdatePending = true;
    (void) QMetaObject::invokeMethod(
        this,
        [this]() {
            _videoSinkUpdatePending = false;
            GstElement* const sink = videoSink() ? _displaySink.get() : nullptr;
            _postToWorker(
                [shared = GStreamer::shareObject(sink)](GstReceiverWorker& worker) { worker.setVideoSink(shared); });
        },
        Qt::QueuedConnection);
}

void GstVideoReceiver::onVideoOutputChanged()
{
    (void) disconnect(_videoSizeConnection);

    QVideoSink* const sink = videoSink();
    if (!sink) {
        if (videoOutput()) {
            qCWarning(GstVideoReceiverLog) << "Video output" << videoOutput() << "has no videoSink; not displaying";
        }
        if (_sinkController) {
            _sinkController->setVideoSink(QPointer<QVideoSink>());
            _sinkController->followWindowVisibility(nullptr);
        }
        _scheduleVideoSinkUpdate();
        return;
    }

    if (_displaySink || _buildDisplaySink(sink)) {
        _bindVideoSink(sink);
    }
    _scheduleVideoSinkUpdate();
}

void GstVideoReceiver::onStartRecording(const QString& file, FileFormat format)
{
    _postToWorker([file, format](GstReceiverWorker& worker) { worker.startRecording(file, format); });
}

void GstVideoReceiver::onStopRecording()
{
    _postToWorker([](GstReceiverWorker& worker) { worker.stopRecording(); });
}

void GstVideoReceiver::_postToWorker(std::function<void(GstReceiverWorker&)> task)
{
    GstReceiverWorker* const worker = _worker;
    (void) QMetaObject::invokeMethod(
        worker, [worker, task = std::move(task)]() { task(*worker); }, Qt::QueuedConnection);
}

void GstVideoReceiver::_releaseDisplaySink()
{
    (void) disconnect(_videoSizeConnection);
    if (_sinkController) {
        _sinkController->prepareForRelease();
        _sinkController->deleteLater();
        _sinkController = nullptr;
    }
    _displaySink.reset();
}

bool GstVideoReceiver::_buildDisplaySink(QVideoSink* videoSink)
{
    _releaseDisplaySink();

    GStreamer::GstElementPtr sink =
        GStreamer::adoptElement(GStreamer::createVideoSink(GStreamer::videoSinkConfigFromSettings()));
    QGCQVideoSinkController* const controller =
        sink ? GStreamer::setupQVideoSinkElement(sink.get(), videoSink, this) : nullptr;
    if (!controller) {
        qCWarning(GstVideoReceiverLog) << "Unable to create the video sink for" << name();
        emit errorOccurred(tr("Unable to display video: the video sink could not be created."));
        return false;
    }
    _displaySink = std::move(sink);
    _sinkController = controller;
    return true;
}

void GstVideoReceiver::_bindVideoSink(QVideoSink* videoSink)
{
    (void) disconnect(_videoSizeConnection);
    _sinkController->setVideoSink(QPointer<QVideoSink>(videoSink));
    _sinkController->followWindowVisibility(videoOutput());
    // The displayed size: rotation applied, as Qt presents the frame.
    _videoSizeConnection = connect(videoSink, &QVideoSink::videoSizeChanged, this,
                                   [this, videoSink]() { setVideoSize(videoSink->videoSize()); });
    if (videoSink->videoSize().isValid()) {
        setVideoSize(videoSink->videoSize());
    }
}

void GstVideoReceiver::_setDecoderName(const QString& name)
{
    if (name != _decoderName) {
        _decoderName = name;
        emit decoderNameChanged();
    }
}

void GstVideoReceiver::_releaseDisplayedFrame()
{
    if (_sinkController) {
        _sinkController->releaseFrame();
    }
}

bool GstVideoReceiver::_runOnPipeline(const std::function<void(GstElement*)>& task)
{
    GstReceiverWorker* const worker = _worker;
    return QMetaObject::invokeMethod(
        worker, [worker, &task]() { task(worker->pipeline()); }, Qt::BlockingQueuedConnection);
}
