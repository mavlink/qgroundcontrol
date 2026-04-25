#include "VideoReceiverSession.h"

#include <QtCore/QTimer>
#include <QtCore/QThread>

#include "QGCLoggingCategory.h"
#include "VideoFrameDelivery.h"
#include "VideoRecordingService.h"
#include "VideoRecorder.h"
#include "VideoSinkRouter.h"
#include "VideoStream.h"

QGC_LOGGING_CATEGORY(VideoReceiverSessionLog, "Video.ReceiverSession")

namespace {

static constexpr int kMaxDrainingReceivers = 3;
static constexpr int kDrainSafetyTimeoutMs = 3000;

}  // namespace

VideoReceiverSession::VideoReceiverSession(QString name,
                                           bool thermal,
                                           ReceiverFactory factory,
                                           VideoStream* owner,
                                           QObject* parent)
    : QObject(parent ? parent : owner)
    , _name(std::move(name))
    , _thermal(thermal)
    , _factory(std::move(factory))
    , _owner(owner)
    , _sinkRouter(new VideoSinkRouter(_name, this))
    , _recorderController(new VideoRecordingService(owner, this))
{
    connect(_recorderController, &VideoRecordingService::recordingStarted,
            this, &VideoReceiverSession::recordingStarted);
    connect(_recorderController, &VideoRecordingService::recordingChanged,
            this, &VideoReceiverSession::recordingChanged);
    connect(_recorderController, &VideoRecordingService::recordingError,
            this, &VideoReceiverSession::recordingError);
    connect(_recorderController, &VideoRecordingService::recorderChanged,
            this, &VideoReceiverSession::recorderChanged);
}

VideoReceiverSession::~VideoReceiverSession()
{
    (void) destroyReceiver();
}

QVideoSink* VideoReceiverSession::videoSink() const
{
    return _sinkRouter ? _sinkRouter->videoSink() : nullptr;
}

VideoFrameDelivery* VideoReceiverSession::frameDelivery() const
{
    return _sinkRouter ? _sinkRouter->frameDelivery() : nullptr;
}

VideoRecorder* VideoReceiverSession::recorder() const
{
    return _recorderController ? _recorderController->recorder() : nullptr;
}

bool VideoReceiverSession::requiresReceiverRecreate(const VideoSourceResolver::VideoSource& source) const
{
    if (!_receiver)
        return false;
    return !source.isValid();
}

VideoReceiverSession::EnsureResult VideoReceiverSession::ensureReceiver(const VideoSourceResolver::VideoSource& source)
{
    EnsureResult result;
    if (_receiver)
        return result;

    _source = source;
    _receiver = _factory(source, _thermal, this);
    if (!_receiver) {
        qCWarning(VideoReceiverSessionLog) << "Factory returned null receiver for" << _name;
        return result;
    }

    result.created = true;

    _receiver->configureSource(_source);
    result.hadPendingSink = _sinkRouter->attachReceiver(_receiver,
        [this](const std::function<void()>& fn) { _invokeOnReceiverThread(fn); });

    _wireReceiverSignals();

    qCDebug(VideoReceiverSessionLog) << "Created receiver for" << _name
                                     << "caps:" << static_cast<int>(_receiver->capabilities());
    return result;
}

bool VideoReceiverSession::destroyReceiver()
{
    if (!_receiver)
        return false;

    _detachReceiverFrameDelivery();
    _disconnectReceiver();

    if (_receiver->started()) {
        while (_drainingReceivers.size() >= kMaxDrainingReceivers) {
            VideoReceiver* oldest = _drainingReceivers.takeFirst();
            qCWarning(VideoReceiverSessionLog) << _name
                                               << "draining-receiver cap reached; force-dropping oldest";
            oldest->deleteLater();
        }

        VideoReceiver* draining = _receiver;
        _drainingReceivers.append(draining);

        auto* safetyTimer = new QTimer(this);
        safetyTimer->setSingleShot(true);

        auto cleanup = [this, draining, safetyTimer]() {
            safetyTimer->stop();
            safetyTimer->deleteLater();
            _drainingReceivers.removeOne(draining);
            draining->deleteLater();
        };

        (void)connect(draining, &VideoReceiver::receiverStopped, this, cleanup, Qt::SingleShotConnection);
        connect(safetyTimer, &QTimer::timeout, this, [this, cleanup]() {
            qCWarning(VideoReceiverSessionLog) << _name << "safety timeout: force-removing draining receiver";
            cleanup();
        });
        safetyTimer->start(kDrainSafetyTimeoutMs);
        draining->stop();
    } else {
        _receiver->deleteLater();
    }

    _receiver = nullptr;
    destroyRecorder();
    return true;
}

VideoReceiverSession::SinkResult VideoReceiverSession::registerVideoSink(QVideoSink* sink)
{
    if (!_sinkRouter)
        return {};

    const VideoSinkRouter::SinkResult result = _sinkRouter->registerVideoSink(
        _receiver, sink, [this](const std::function<void()>& fn) { _invokeOnReceiverThread(fn); });
    return {result.frameDeliveryChanged, result.restartRequested};
}

void VideoReceiverSession::createRecorder(std::unique_ptr<VideoRecorder> preferredRecorder)
{
    destroyRecorder();

    if (!_receiver)
        return;

    if (_recorderController)
        _recorderController->create(_source, _receiver, frameDelivery(), std::move(preferredRecorder));
}

void VideoReceiverSession::destroyRecorder()
{
    if (_recorderController)
        _recorderController->destroy();
}

std::unique_ptr<VideoRecorder> VideoReceiverSession::releaseRecorder()
{
    return _recorderController ? _recorderController->release() : std::unique_ptr<VideoRecorder>();
}

void VideoReceiverSession::setRecorderForTest(VideoRecorder* recorder)
{
    if (_recorderController)
        _recorderController->setForTest(recorder);
}

void VideoReceiverSession::setRecorderFactoryForTest(RecorderFactory factory)
{
    if (_recorderController)
        _recorderController->setFactoryForTest(std::move(factory));
}

void VideoReceiverSession::_wireReceiverSignals()
{
    if (!_receiver)
        return;

    _receiverConns << connect(_receiver, &VideoReceiver::streamingChanged,
                              this, &VideoReceiverSession::streamingChanged);
    _receiverConns << connect(_receiver, &VideoReceiver::decodingChanged,
                              this, &VideoReceiverSession::decodingChanged);
    _receiverConns << connect(_receiver, &VideoReceiver::receiverError,
                              this, &VideoReceiverSession::receiverError);
    _receiverConns << connect(_receiver, &VideoReceiver::timeout,
                              this, &VideoReceiverSession::timeout);
}

void VideoReceiverSession::_disconnectReceiver()
{
    for (const auto& connection : std::as_const(_receiverConns))
        disconnect(connection);
    _receiverConns.clear();
}

void VideoReceiverSession::_detachReceiverFrameDelivery()
{
    if (!_receiver)
        return;

    _sinkRouter->detachReceiver(_receiver, [this](const std::function<void()>& fn) { _invokeOnReceiverThread(fn); });
}

void VideoReceiverSession::_invokeOnReceiverThread(const std::function<void()>& fn)
{
    if (!_receiver)
        return;

    if (_receiver->thread() == QThread::currentThread()) {
        fn();
        return;
    }

    QMetaObject::invokeMethod(_receiver, fn, Qt::BlockingQueuedConnection);
}
