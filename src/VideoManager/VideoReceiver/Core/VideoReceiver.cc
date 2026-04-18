#include "VideoReceiver.h"

#include "BridgeRecorder.h"

// ─────────────────────────────────────────────────────────────────────────────
// Frame delivery helpers
//
// The frame watchdog lives on VideoFrameDelivery and is wired at the VideoStream
// level (watchdogTimeout -> VideoStream::_onReceiverTimeout). Receiver
// backends that want watchdog coverage arm delivery via armWatchdog().
// ─────────────────────────────────────────────────────────────────────────────

void VideoReceiver::resetBridgeStats()
{
    if (_delivery)
        _delivery->resetStats();
}

bool VideoReceiver::validateBridgeForDecoding()
{
    resetBridgeStats();
    if (!_delivery || !_delivery->videoSink())
        return false;
    return true;
}

VideoRecorder* VideoReceiver::createRecorder(VideoFrameDelivery* delivery, QObject* parent)
{
    if (!delivery)
        return nullptr;

    return new BridgeRecorder(delivery, parent);
}
