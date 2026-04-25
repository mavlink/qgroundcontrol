#include "VideoReceiver.h"

#include "VideoSourceResolver.h"

// ─────────────────────────────────────────────────────────────────────────────
// Frame delivery helpers
//
// The frame watchdog lives on VideoFrameDelivery and is wired at the VideoStream
// level (watchdogTimeout -> VideoStream::_onReceiverTimeout). Receiver
// receivers that want watchdog coverage arm delivery via armWatchdog().
// ─────────────────────────────────────────────────────────────────────────────

void VideoReceiver::resetFrameDeliveryStats()
{
    if (_delivery)
        _delivery->resetStats();
}

bool VideoReceiver::validateFrameDeliveryForDecoding()
{
    resetFrameDeliveryStats();
    if (!_delivery || !_delivery->videoSink())
        return false;
    return true;
}

void VideoReceiver::configureSource(const VideoSourceResolver::VideoSource& source)
{
    Q_UNUSED(source);
}
