#include "GstVideoReceiver.h"

#include <chrono>
#include <utility>

#include <QtCore/QDeadlineTimer>
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

}  // namespace

GstVideoReceiver::GstVideoReceiver(QObject* parent)
    : VideoReceiver(parent)
    , _workerThread(new QThread(this))
    , _worker(new GstReceiverWorker())
{
    qCDebug(GstVideoReceiverLog) << this;

    // Queued, with this receiver as the context: once it is destroyed, the worker's reports go nowhere.
    (void) connect(_worker, &GstReceiverWorker::stateChanged, this, &GstVideoReceiver::setState);
    (void) connect(_worker, &GstReceiverWorker::decodingChanged, this, [this](bool decoding) {
        setDecoding(decoding);
        if (!decoding && _sinkController) {
            _sinkController->releaseFrame();
        }
    });
    (void) connect(_worker, &GstReceiverWorker::decoderNameChanged, this, &GstVideoReceiver::_setDecoderName);
    (void) connect(_worker, &GstReceiverWorker::recordingChanged, this, &GstVideoReceiver::setRecording);
    (void) connect(_worker, &GstReceiverWorker::recordingStarted, this, &GstVideoReceiver::recordingStarted);
    (void) connect(_worker, &GstReceiverWorker::recordingFinished, this, &GstVideoReceiver::recordingFinished);
    (void) connect(_worker, &GstReceiverWorker::errorOccurred, this, &GstVideoReceiver::errorOccurred);
    (void) connect(_worker, &GstReceiverWorker::statsChanged, this, &GstVideoReceiver::setStats);

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
    if (QVideoSink* const sink = videoSink()) {
        _bindVideoSink(sink);
    }
    _postToWorker(
        [sink = GStreamer::shareObject(_displaySink.get())](GstReceiverWorker& worker) { worker.restart(sink); });
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
    GstElement* const displaySink = sink ? _displaySink.get() : nullptr;
    _postToWorker(
        [shared = GStreamer::shareObject(displaySink)](GstReceiverWorker& worker) { worker.setVideoSink(shared); });
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
    const DisplayConfig& display = displayConfig();
    const GStreamer::VideoSinkConfig sinkConfig{
        .conversionElement = display.conversionElement,
        .disablePixelAspectRatio = display.disablePixelAspectRatio,
        .gpuZeroCopy = display.gpuZeroCopy && GStreamer::gpuZeroCopyAllowedForCurrentGraphicsApi(false, false),
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
    GstReceiverWorker* const worker = _worker;
    return QMetaObject::invokeMethod(
        worker, [worker, &task]() { task(worker->pipeline()); }, Qt::BlockingQueuedConnection);
}
