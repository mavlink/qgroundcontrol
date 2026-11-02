#pragma once

#include <functional>
#include <gst/gst.h>

#include <QtCore/QObject>
#include <QtCore/QSemaphore>

#include "GstScoped.h"

class QDeadlineTimer;

/// \brief A pipeline's bus, handled from the thread the pump lives on.
///
/// A sync handler runs on the posting thread for what can't wait; every other message is popped on the pump's thread,
/// which the bus's poll fd wakes up, as Qt's QGstBusObserver does. detach() removes the sync handler, waits until no
/// call into it is in flight and flushes what is still queued: nothing the pipeline posts reaches either handler
/// afterwards.
class GstBusPump : public QObject
{
    Q_OBJECT

public:
    /// Posting threads. Returns GST_BUS_DROP for a message it took (and unreffed), GST_BUS_PASS to queue it.
    using SyncHandler = std::function<GstBusSyncReply(GstMessage* message)>;
    /// The pump's thread.
    using Handler = std::function<void(GstMessage* message)>;

    explicit GstBusPump(QObject* parent = nullptr);
    /// detach()es.
    ~GstBusPump() override;

    /// Watches @p pipeline's bus. The sync handler is installed before anything can post, so it sees every message.
    /// False, logged, when the pipeline has no bus.
    [[nodiscard]] bool attach(GstElement* pipeline, SyncHandler syncHandler, Handler handler);
    /// Idempotent.
    void detach();

    /// Handles the messages queued now, as the poll fd does.
    void drain();
    /// Handles the messages of @p types until @p done() or @p deadline, waiting for them.
    void pumpUntil(GstMessageType types, const std::function<bool()>& done, const QDeadlineTimer& deadline);
    /// Pops the oldest queued message of @p type, unhandled; null when there is none.
    GStreamer::GstMessagePtr pop(GstMessageType type);

private:
    static GstBusSyncReply _onSync(GstBus* bus, GstMessage* message, gpointer data);

    GStreamer::GstBusPtr _bus;
    SyncHandler _syncHandler;
    Handler _handler;
    QObject* _notifier = nullptr;
    bool _syncInstalled = false;
    /// Released by GStreamer once the detached sync handler no longer runs.
    QSemaphore _syncReleased;
};
