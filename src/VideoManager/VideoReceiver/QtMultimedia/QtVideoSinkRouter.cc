#include "QtVideoSinkRouter.h"

#include <QtMultimedia/QVideoFrame>

#include "QGCLoggingCategory.h"
#include "VideoFrameDelivery.h"

QGC_LOGGING_CATEGORY(QtVideoSinkRouterLog, "Video.QtVideoSinkRouter")

QtVideoSinkRouter::QtVideoSinkRouter(QObject* parent)
    : QObject(parent),
      _fallbackSink(new QVideoSink(this)),
      _activeSink(_fallbackSink)
{
}

QtVideoSinkRouter::~QtVideoSinkRouter()
{
    detachObserver();
}

void QtVideoSinkRouter::setFrameDelivery(VideoFrameDelivery* delivery)
{
    _delivery = delivery;
    _connectObserver(_activeSink);
}

void QtVideoSinkRouter::setSinkApplier(SinkApplier applier)
{
    _sinkApplier = std::move(applier);
}

void QtVideoSinkRouter::routeTo(QVideoSink* externalSink)
{
    QVideoSink* newSink = externalSink ? externalSink : _fallbackSink;
    _activeSink = newSink;

    if (_sinkApplier)
        _sinkApplier(_activeSink);

    _connectObserver(_activeSink);
    qCDebug(QtVideoSinkRouterLog) << "Active Qt video sink:" << _activeSink
                                  << "fallback:" << activeSinkIsFallback()
                                  << "rhi:" << activeSinkHasRhi();
}

void QtVideoSinkRouter::detachObserver()
{
    disconnect(_frameConn);
}

bool QtVideoSinkRouter::activeSinkHasRhi() const
{
    return _activeSink && _activeSink->rhi();
}

void QtVideoSinkRouter::_connectObserver(QVideoSink* sink)
{
    disconnect(_frameConn);

    if (!_delivery || !sink)
        return;

    _frameConn = connect(sink, &QVideoSink::videoFrameChanged, this,
                         [this](const QVideoFrame& frame) {
                             if (frame.isValid() && _delivery)
                                 _delivery->observeSinkFrame(frame);
                         });
}
