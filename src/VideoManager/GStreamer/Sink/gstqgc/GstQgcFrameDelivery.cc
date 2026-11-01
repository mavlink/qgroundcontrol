#include "GstQgcFrameDelivery.h"

#include <utility>

#include <QtCore/QMetaObject>
#include <QtCore/QMutexLocker>
#include <QtMultimedia/QVideoFrame>
#include <QtMultimedia/QVideoSink>

namespace GstQgc {

struct FrameDelivery::Slot
{
    QMutex mutex;
    QVideoFrame frame;
    /// An invoke is queued to the QVideoSink.
    bool posted = false;

    QVideoFrame take()
    {
        QMutexLocker locker(&mutex);
        return std::exchange(frame, QVideoFrame());
    }
};

FrameDelivery::FrameDelivery()
    : _slot(std::make_shared<Slot>())
{}

FrameDelivery::~FrameDelivery() = default;

QVideoSink* FrameDelivery::sink() const
{
    const QMutexLocker locker(&_mutex);
    return _sink;
}

void FrameDelivery::setSink(QVideoSink* sink)
{
    // Released after the lock: it may hand its decoder buffer back to a pool.
    QVideoFrame undelivered;
    const QMutexLocker locker(&_mutex);
    if (sink != _sink) {
        undelivered = _slot->take();
        _slot = std::make_shared<Slot>();
    }
    _sink = sink;
}

FrameDelivery::Result FrameDelivery::deliver(QVideoFrame&& frame)
{
    QVideoFrame superseded;
    const QMutexLocker locker(&_mutex);
    QVideoSink* const sink = _sink;
    if (!sink) {
        return {};
    }
    const std::shared_ptr<Slot> slot = _slot;
    bool needPost = false;
    {
        QMutexLocker slotLocker(&slot->mutex);
        superseded = std::exchange(slot->frame, std::move(frame));
        needPost = !std::exchange(slot->posted, true);
    }
    if (needPost) {
        QMetaObject::invokeMethod(
            sink,
            [sink, slot]() {
                QVideoFrame latest;
                {
                    QMutexLocker slotLocker(&slot->mutex);
                    latest = std::exchange(slot->frame, QVideoFrame());
                    slot->posted = false;
                }
                if (latest.isValid()) {
                    sink->setVideoFrame(latest);
                }
            },
            Qt::QueuedConnection);
    }
    return {.delivered = true, .superseded = superseded.isValid()};
}

void FrameDelivery::dropPending()
{
    QVideoFrame dropped;
    const QMutexLocker locker(&_mutex);
    dropped = _slot->take();
}

}  // namespace GstQgc
