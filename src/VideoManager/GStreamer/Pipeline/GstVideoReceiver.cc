#include "GstVideoReceiver.h"

#include <chrono>
#include <memory>
#include <mutex>
#include <utility>

#include <QtCore/QDeadlineTimer>
#include <QtCore/QSemaphore>
#include <QtCore/QThread>
#include <QtMultimedia/QVideoFrame>
#include <QtMultimedia/QVideoSink>

#include "GStreamer.h"
#include "GstReceiverWorker.h"
#include "QGCLoggingCategory.h"
#include "QGCQVideoSinkController.h"

QGC_LOGGING_CATEGORY(GstVideoReceiverLog, "Video.GStreamer.GstVideoReceiver")

namespace {

// The destructor's wait for the worker: a recording's finalization plus time for the pipeline to stop.
constexpr std::chrono::seconds kWorkerShutdownTimeout = RecordingBranch::kFinalizeTimeout + std::chrono::seconds{2};
// _runOnPipeline()'s wait for the worker to run a task.
constexpr std::chrono::seconds kRunOnPipelineTimeout{10};

}  // namespace

GstVideoReceiver::GstVideoReceiver(QObject* parent)
    : VideoReceiver(parent)
    , _workerThread(new QThread(this))
    , _worker(new GstReceiverWorker())
{
    qCDebug(GstVideoReceiverLog) << this;

    // Queued, with this receiver as the context: once it is destroyed, the worker's reports go nowhere.
    (void) connect(_worker, &GstReceiverWorker::statusChanged, this, &GstVideoReceiver::_applyStatus);
    (void) connect(_worker, &GstReceiverWorker::recordingStarted, this, &GstVideoReceiver::recordingStarted);
    (void) connect(_worker, &GstReceiverWorker::recordingFinished, this, &GstVideoReceiver::recordingFinished);
    (void) connect(_worker, &GstReceiverWorker::errorOccurred, this, &GstVideoReceiver::errorOccurred);

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
    // only through its signals, so it can finish on its own and clean up after itself.
    qCWarning(GstVideoReceiverLog) << "Worker still busy after"
                                   << std::chrono::milliseconds(kWorkerShutdownTimeout).count()
                                   << "ms; leaving it to finish on its own";
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

void GstVideoReceiver::onStartRecording(const QString& file, FileFormat format)
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

void GstVideoReceiver::_applyStatus(const GstReceiverStatus& status)
{
    setState(status.state);
    const bool wasDecoding = decoding();
    setDecoding(status.decoding);
    if (wasDecoding && !status.decoding && _sinkController) {
        _sinkController->releaseFrame();
    }
    _setDecoderName(status.decoderName);
    setRecording(status.recording);
    setStats(status.stats);
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
    const DisplayConfig& display = displayConfig();
    const GStreamer::VideoSinkConfig sinkConfig{
        .conversionElement = display.conversionElement,
        .disablePixelAspectRatio = display.disablePixelAspectRatio,
        .gpuZeroCopy = display.gpuZeroCopy,
    };
    GStreamer::GstElementPtr sink = GStreamer::adoptElement(GStreamer::createVideoSink(sinkConfig));
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
    // this thread.
    _videoSizeConnection = connect(videoSink, &QVideoSink::videoFrameChanged, this,
                                   [this](const QVideoFrame& frame) { setVideoSize(presentedSize(frame)); });
    if (const QSize size = presentedSize(videoSink->videoFrame()); size.isValid()) {
        setVideoSize(size);
    }
}

void GstVideoReceiver::_setDecoderName(const QString& name)
{
    if (name != _decoderName) {
        _decoderName = name;
        emit decoderNameChanged();
    }
}

bool GstVideoReceiver::_runOnPipeline(const std::function<void(GstElement*)>& task)
{
    // Not a BlockingQueuedConnection, which would wait for good on a worker that never gets to the task. The task
    // runs under the mutex, so once the wait is given up it is either done or never runs.
    struct Call
    {
        std::mutex mutex;
        bool abandoned = false;
        QSemaphore done;
    };

    const auto call = std::make_shared<Call>();
    GstReceiverWorker* const worker = _worker;
    const bool posted = QMetaObject::invokeMethod(
        worker,
        [worker, call, &task]() {
            const std::lock_guard lock(call->mutex);
            if (!call->abandoned) {
                task(worker->pipeline());
                call->done.release();
            }
        },
        Qt::QueuedConnection);
    if (!posted) {
        return false;
    }
    if (call->done.tryAcquire(1, QDeadlineTimer(kRunOnPipelineTimeout))) {
        return true;
    }
    const std::lock_guard lock(call->mutex);
    call->abandoned = true;
    return call->done.tryAcquire();
}
