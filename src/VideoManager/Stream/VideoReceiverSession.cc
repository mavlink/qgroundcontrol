#include "VideoReceiverSession.h"

#include <QtCore/QTimer>
#include <QtCore/QThread>

#include "QGCLoggingCategory.h"
#include "VideoFrameDelivery.h"
#include "VideoRecorderSessionController.h"
#include "VideoRecorder.h"
#include "VideoSinkRouter.h"
#include "VideoStream.h"

#ifdef QGC_GST_STREAMING
#include "GstVideoReceiver.h"
#endif

QGC_LOGGING_CATEGORY(VideoReceiverSessionLog, "Video.ReceiverSession")

VideoReceiverSession::VideoReceiverSession(QString name, bool thermal, ReceiverFactory factory, VideoStream* owner)
    : QObject(owner)
    , _name(std::move(name))
    , _thermal(thermal)
    , _factory(std::move(factory))
    , _owner(owner)
    , _sinkRouter(new VideoSinkRouter(_name, this))
    , _recorderController(new VideoRecorderSessionController(owner, this))
{
    connect(_recorderController, &VideoRecorderSessionController::recordingStarted,
            this, &VideoReceiverSession::recordingStarted);
    connect(_recorderController, &VideoRecorderSessionController::recordingChanged,
            this, &VideoReceiverSession::recordingChanged);
    connect(_recorderController, &VideoRecorderSessionController::recordingError,
            this, &VideoReceiverSession::recordingError);
    connect(_recorderController, &VideoRecorderSessionController::recorderChanged,
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

VideoFrameDelivery* VideoReceiverSession::bridge() const
{
    return _sinkRouter ? _sinkRouter->delivery() : nullptr;
}

VideoRecorder* VideoReceiverSession::recorder() const
{
    return _recorderController ? _recorderController->recorder() : nullptr;
}

bool VideoReceiverSession::requiresReceiverRecreate(const VideoSourceResolver::VideoSource& source) const
{
    if (!_receiver)
        return false;
    if (!source.isValid())
        return true;
    return _receiver->kind() != source.backend();
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

    _receiver->setName(_name);
    _receiver->setThermal(_thermal);
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

    _detachReceiverBridge();
    _disconnectReceiver();

    if (_receiver->started()) {
        constexpr int kMaxDrainingReceivers = 3;
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
        safetyTimer->start(3000);
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
        _receiver, _source, sink, [this](const std::function<void()>& fn) { _invokeOnReceiverThread(fn); });
    return {result.bridgeChanged, result.restartRequested};
}

void VideoReceiverSession::createRecorder()
{
    destroyRecorder();

    if (!_receiver)
        return;

    if (_recorderController)
        _recorderController->create(_source, _receiver, bridge());
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

#ifdef QGC_GST_STREAMING
    if (auto* gst = qobject_cast<GstVideoReceiver*>(_receiver)) {
        _receiverConns << connect(gst, &GstVideoReceiver::sinkAttachedToLivePipeline,
                                  this, &VideoReceiverSession::lateSinkRestartRequested);
    }
#endif
}

void VideoReceiverSession::_disconnectReceiver()
{
    for (const auto& connection : std::as_const(_receiverConns))
        disconnect(connection);
    _receiverConns.clear();
}

void VideoReceiverSession::_detachReceiverBridge()
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
