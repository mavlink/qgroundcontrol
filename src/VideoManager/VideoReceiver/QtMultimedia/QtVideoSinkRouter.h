#pragma once

#include <QtCore/QMetaObject>
#include <QtCore/QObject>
#include <QtMultimedia/QVideoSink>
#include <functional>

class VideoFrameDelivery;

/// Shared QtMultimedia sink routing for producers that render to QVideoSink.
///
/// The active producer sink is either the registered QML sink or an internal
/// fallback sink. Frames are observed for stats/recording through
/// VideoFrameDelivery without redisplaying them through the delivery endpoint.
class QtVideoSinkRouter : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(QtVideoSinkRouter)

public:
    using SinkApplier = std::function<void(QVideoSink*)>;

    explicit QtVideoSinkRouter(QObject* parent = nullptr);
    ~QtVideoSinkRouter() override;

    [[nodiscard]] QVideoSink* fallbackSink() const { return _fallbackSink; }
    [[nodiscard]] QVideoSink* activeSink() const { return _activeSink; }
    [[nodiscard]] bool activeSinkIsFallback() const { return _activeSink == _fallbackSink; }
    [[nodiscard]] bool activeSinkHasRhi() const;

    void setFrameDelivery(VideoFrameDelivery* delivery);
    void setSinkApplier(SinkApplier applier);
    void routeTo(QVideoSink* externalSink);
    void detachObserver();

private:
    void _connectObserver(QVideoSink* sink);

    QVideoSink* _fallbackSink = nullptr;
    QVideoSink* _activeSink = nullptr;
    VideoFrameDelivery* _delivery = nullptr;
    SinkApplier _sinkApplier;
    QMetaObject::Connection _frameConn;
};
