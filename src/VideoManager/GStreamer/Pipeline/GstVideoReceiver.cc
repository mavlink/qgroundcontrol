#include "GstVideoReceiver.h"

#include <chrono>
#include <utility>

#include <QtCore/QCoreApplication>
#include <QtCore/QDeadlineTimer>
#include <QtCore/QThread>
#include <QtCore/QTimer>
#include <QtMultimedia/QVideoFrame>
#include <QtMultimedia/QVideoSink>

#include "GstReceiverWorker.h"
#include "QGCLoggingCategory.h"
#include "QGCQVideoSinkController.h"

QGC_LOGGING_CATEGORY(GstVideoReceiverLog, "Video.GStreamer.GstVideoReceiver")

namespace {

// The destructor's wait for the worker: a recording's finalization plus time for the pipeline to stop.
constexpr std::chrono::seconds kWorkerShutdownTimeout = RecordingBranch::kFinalizeTimeout + std::chrono::seconds{2};
static_assert(GstVideoReceiver::kHeartbeatInterval > kWorkerShutdownTimeout);

// Checks in a row, 60 s, that find the worker has run none of its tasks since the check before, until it counts as
// stuck: a name lookup for a tcp:// or udp:// host, which runs on the worker, can take half a minute to fail. Each
// check comes a full interval after the one before, so one that is late (a suspended machine, a stopped process, a
// stalled GUI thread) can't count twice against a worker that got no time to run a task.
constexpr int kStuckWorkerChecks = 6;

}  // namespace

GstVideoReceiver::GstVideoReceiver(QObject* parent, const GstReceiverTimings& timings)
    : VideoReceiver(parent)
    , _workerThread(new QThread(this))
    , _worker(new GstReceiverWorker(timings))
    , _heartbeatTimer(new QTimer(this))
{
    qCDebug(GstVideoReceiverLog) << this;

    _heartbeatTimer->setSingleShot(true);
    _heartbeatTimer->setInterval(kHeartbeatInterval);
    (void) connect(_heartbeatTimer, &QTimer::timeout, this, &GstVideoReceiver::_checkWorker);

    // Queued, with this receiver as the context: once it is destroyed, the worker's reports go nowhere.
    (void) connect(_worker, &GstReceiverWorker::statusChanged, this, &GstVideoReceiver::_onWorkerStatus);
    (void) connect(_worker, &GstReceiverWorker::taskDone, this, &GstVideoReceiver::_onWorkerTaskDone);
    (void) connect(_worker, &GstReceiverWorker::recordingStarted, this, &GstVideoReceiver::recordingStarted);
    (void) connect(_worker, &GstReceiverWorker::recordingFinished, this, &GstVideoReceiver::recordingFinished);
    (void) connect(_worker, &GstReceiverWorker::errorOccurred, this, &GstVideoReceiver::errorOccurred);
    // The video sink goes with the pipeline: it too holds the device.
    (void) connect(_worker, &GstReceiverWorker::graphicsDeviceLost, this, &GstVideoReceiver::restart);

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

    // Only a streaming thread that doesn't return (a recorder stuck in the kernel on a hung network mount, a decoder
    // deadlocked in its library) holds the worker this long. It reaches this receiver only through its signals, so it
    // can finish on its own and clean up after itself.
    qCWarning(GstVideoReceiverLog) << "Worker still busy after"
                                   << std::chrono::milliseconds(kWorkerShutdownTimeout).count()
                                   << "ms; leaving it to finish on its own";
    _workerThread->setParent(nullptr);
    (void) connect(_workerThread, &QThread::finished, _workerThread, &QObject::deleteLater);
    if (_workerThread->isFinished()) {
        delete _workerThread;
    }
}

void GstVideoReceiver::setHeartbeatInterval(std::chrono::milliseconds interval)
{
    _heartbeatTimer->setInterval(interval);
}

void GstVideoReceiver::onConfigChanged()
{
    _postRequest();
}

void GstVideoReceiver::onActiveChanged()
{
    _postRequest();
}

void GstVideoReceiver::onRestartRequested()
{
    // Construct-only sink settings apply to a new sink only.
    _releaseDisplaySink();
    if (QVideoSink* const sink = videoSink()) {
        _bindVideoSink(sink);
    }
    ++_request.restarts;
    _postRequest();
}

void GstVideoReceiver::onReconnectRequested()
{
    ++_request.reconnects;
    _postRequest();
}

void GstVideoReceiver::onVideoOutputChanged()
{
    (void) disconnect(_videoSizeConnection);

    QVideoSink* const sink = videoSink();
    if (sink) {
        _bindVideoSink(sink);
    } else {
        if (videoOutput()) {
            qCWarning(GstVideoReceiverLog) << "Video output" << videoOutput() << "has no videoSink; not displaying";
        }
        if (_sinkController) {
            _sinkController->setVideoSink(QPointer<QVideoSink>());
            _sinkController->followWindowVisibility(nullptr);
        }
    }
    _postRequest();
}

void GstVideoReceiver::onStartRecording(const QString& file, FileFormat format,
                                        [[maybe_unused]] const RecordingEncoding& encoding)
{
    const quint64 id = (_request.recording ? _request.recording->id : 0) + 1;
    _request.recording = GstRecordingRequest{.file = file, .format = format, .id = id};
    _postRequest();
}

void GstVideoReceiver::onStopRecording()
{
    _request.recording.reset();
    _postRequest();
}

void GstVideoReceiver::_postRequest()
{
    _request.active = active();
    _request.name = name();
    _request.config = config();
    GstElement* const displaySink = videoSink() ? _displaySink.get() : nullptr;
    if (displaySink != _request.videoSink.get()) {
        _request.videoSink = GStreamer::shareObject(displaySink);
    }
    _postToWorker([request = _request](GstReceiverWorker& worker) { worker.apply(request); });
}

void GstVideoReceiver::_checkWorker()
{
    const bool frameShown = std::exchange(_frameShown, false);
    const bool ranTask = _tasksDone != std::exchange(_tasksDoneAtCheck, _tasksDone);
    if (_tasksDone == _tasksPosted) {
        _stalledChecks = 0;
        if (!active()) {
            // Through with all it was posted, and no stream is wanted of it: it has nothing to get stuck in.
            return;
        }
        // The heartbeat of a worker that is posted nothing else: what it does of its own, as replacing a decoder that
        // stalled, can hold it too.
        _postToWorker([](GstReceiverWorker& /*worker*/) {});
    } else if (ranTask) {
        // Slow, or behind with a row of requests, but not stuck.
        _stalledChecks = 0;
    } else if (_workerStuck) {
        _applyStuckStatus(frameShown);
    } else if (++_stalledChecks >= kStuckWorkerChecks) {
        qCWarning(GstVideoReceiverLog) << "The worker of" << name() << "has run none of its tasks for"
                                       << (kStuckWorkerChecks * _heartbeatTimer->intervalAsDuration()).count()
                                       << "ms: a streaming thread it waits for doesn't return";
        _workerStuck = true;
        _applyStuckStatus(frameShown);
        emit errorOccurred(
            tr("The video decoder or recorder stopped responding. Restart %1 to get video working again.")
                .arg(QCoreApplication::applicationName()));
    }
    // A full interval from now, not from when this check was due: one that comes late puts the next ones off.
    _heartbeatTimer->start();
}

void GstVideoReceiver::_onWorkerTaskDone()
{
    ++_tasksDone;
    if (_workerStuck) {
        qCDebug(GstVideoReceiverLog) << "The worker of" << name() << "runs its tasks again";
        _workerStuck = false;
        _applyStatus(_workerStatus);
    }
}

void GstVideoReceiver::_applyStuckStatus(bool frameShown)
{
    // The pipeline runs on without its worker, which a recorder that doesn't return holds as well as a decoder: only
    // the frames tell which. While they reach the output the stream is as the worker reported it last, though nothing
    // measures it. Without them it is as one that gave up, so that no frozen picture passes for the live one.
    GstReceiverStatus status = _workerStatus;
    status.stats = {};
    if (!frameShown) {
        status.state = State::Stopped;
        status.decoding = false;
        status.decoderName.clear();
    }
    _applyStatus(status);
}

void GstVideoReceiver::_onWorkerStatus(const GstReceiverStatus& status)
{
    _workerStatus = status;
    if (!_workerStuck) {
        _applyStatus(status);
    }
}

void GstVideoReceiver::_applyStatus(const GstReceiverStatus& status)
{
    setState(status.state);
    const bool wasDecoding = decoding();
    setDecoding(status.decoding);
    if (wasDecoding && !status.decoding && _sinkController) {
        _sinkController->releaseFrame();
    }
    _decoderName = status.decoderName;
    setRecording(status.recording);
    setStats(status.stats);
}

void GstVideoReceiver::_postToWorker(std::function<void(GstReceiverWorker&)> task)
{
    GstReceiverWorker* const worker = _worker;
    (void) QMetaObject::invokeMethod(
        worker,
        [worker, task = std::move(task)]() {
            task(*worker);
            emit worker->taskDone();
        },
        Qt::QueuedConnection);

    ++_tasksPosted;
    if (!_heartbeatTimer->isActive()) {
        _heartbeatTimer->start();
    }
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
    const DisplayConfig& display = displayConfig();
    GStreamer::GstElementPtr sink = GStreamer::adoptElement(
        GStreamer::createVideoSink(display.conversionElement, display.disablePixelAspectRatio, display.gpuZeroCopy));
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
    if (_displaySink) {
        _sinkController->setVideoSink(QPointer<QVideoSink>(videoSink));
    } else if (!_buildDisplaySink(videoSink)) {
        return;
    }
    _sinkController->followWindowVisibility(videoOutput());
    // Per frame, not on QVideoSink::videoSizeChanged: that follows the frame size, which for a decoder's buffer cropped
    // by crop meta is larger than the picture, and misses a viewport that changes on its own. Frames reach the sink on
    // this thread. The picture's size is the display sink's: a viewport that leaves out the padding of a squeezed
    // texture is smaller.
    const auto sizeOf = [this](const QVideoFrame& frame) {
        return presentedSize(frame, _sinkController ? _sinkController->pictureSize() : QSize());
    };
    _videoSizeConnection =
        connect(videoSink, &QVideoSink::videoFrameChanged, this, [this, sizeOf](const QVideoFrame& frame) {
            _frameShown = _frameShown || frame.isValid();
            setVideoSize(sizeOf(frame));
        });
    if (const QSize size = sizeOf(videoSink->videoFrame()); size.isValid()) {
        setVideoSize(size);
    }
}
