#pragma once

#include <memory>

#include <QtCore/QMutex>

class QVideoFrame;
class QVideoSink;

namespace GstQgc {

/// \brief qgcqvideosink's latest-frame-only hand-off to a QVideoSink, on the QVideoSink's thread.
///
/// At most one invoke is queued to the QVideoSink, and it takes whatever frame is newest when it runs, so a stalled GUI
/// thread never piles up frames, or the decoder buffers they pin. Thread-safe.
class FrameDelivery
{
public:
    struct Result
    {
        bool delivered = false;   ///< False when no QVideoSink is bound.
        bool superseded = false;  ///< It replaced a frame the QVideoSink hadn't taken yet.
    };

    FrameDelivery();
    ~FrameDelivery();

    FrameDelivery(const FrameDelivery&) = delete;
    FrameDelivery& operator=(const FrameDelivery&) = delete;

    /// The QVideoSink the frames go to: caller-owned, never dereferenced here but to queue the hand-off.
    QVideoSink* sink() const;
    /// Delivers to @p sink from now on; nullptr stops delivering. A frame bound for the previous sink is dropped, and
    /// an invoke still queued to it finds nothing. The controller clears the sink here from the QVideoSink's destroyed
    /// handler, which can't interleave with a deliver(): an invoke deliver() queued meanwhile is purged by ~QObject.
    void setSink(QVideoSink* sink);

    /// Hands @p frame over as the newest.
    Result deliver(QVideoFrame&& frame);
    /// Drops an undelivered frame, releasing the decoder buffer it pins.
    void dropPending();

private:
    struct Slot;

    mutable QMutex _mutex;
    QVideoSink* _sink = nullptr;
    /// Replaced whenever the sink changes: an invoke queued to a destroyed QVideoSink is purged without running, which
    /// would otherwise leave the slot's posted flag latched and starve the next sink.
    std::shared_ptr<Slot> _slot;
};

}  // namespace GstQgc
