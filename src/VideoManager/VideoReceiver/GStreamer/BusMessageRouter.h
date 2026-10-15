#pragma once

#include <gst/gstbus.h>
#include <gst/gstelement.h>

#include <QtCore/QSemaphore>

class GstVideoReceiver;

/// \brief The receiver pipeline's only bus consumer: a sync handler that drops every message once it is handled.
///
/// Lets the HwBuffers context bridges answer NEED_CONTEXT, hands recorder and display traffic to the receiver's
/// branches, does other cheap work inline, and queues the rest on the receiver's worker under the pipeline
/// generation. Nothing pops the bus, so a queued message would stay for the pipeline's lifetime.
///
/// Threading: attach() and detach() on the receiver's worker; the handler on whichever thread posts.
class BusMessageRouter
{
public:
    explicit BusMessageRouter(GstVideoReceiver& receiver);

    /// Before anything can post: the context bridges must see NEED_CONTEXT, which upstream queries during
    /// PAUSED->PLAYING.
    void attach(GstElement* pipeline);
    /// Returns once no thread is inside the handler any more, so no bus callback can reach the receiver.
    void detach();

private:
    static GstBusSyncReply _onSyncMessage(GstBus* bus, GstMessage* message, gpointer user_data);
    void _onError(GstMessage* message);
    void _onElement(GstMessage* message);
    void _onStateChanged(GstMessage* message);
    void _onLatency();

    GstVideoReceiver& _receiver;
    GstBus* _bus = nullptr;       ///< Held while the handler is attached.
    QSemaphore _handlerReleased;  ///< Released by GStreamer once the detached handler is no longer running.
};
