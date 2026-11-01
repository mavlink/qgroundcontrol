#include "GstBusPump.h"

#include <utility>

#include <QtCore/QDeadlineTimer>
#ifdef Q_OS_WIN
#include <QtCore/QWinEventNotifier>
#else
#include <QtCore/QSocketNotifier>
#endif

#include "GstVideoReceiver.h"

GstBusPump::GstBusPump(QObject* parent)
    : QObject(parent)
{}

GstBusPump::~GstBusPump()
{
    detach();
}

bool GstBusPump::attach(GstElement* pipeline, SyncHandler syncHandler, Handler handler)
{
    detach();
    _bus = GStreamer::adoptBus(gst_pipeline_get_bus(GST_PIPELINE(pipeline)));
    if (!_bus) {
        qCCritical(GstVideoReceiverLog) << "gst_pipeline_get_bus() failed";
        return false;
    }
    _syncHandler = std::move(syncHandler);
    _handler = std::move(handler);

    gst_bus_set_sync_handler(_bus.get(), _onSync, this,
                             [](gpointer pump) { static_cast<GstBusPump*>(pump)->_syncReleased.release(); });
    _syncInstalled = true;

    // Readable while messages are queued, like Qt's QGstBusObserver.
    GPollFD pollFd{};
    gst_bus_get_pollfd(_bus.get(), &pollFd);
#ifdef Q_OS_WIN
    auto* const notifier = new QWinEventNotifier(reinterpret_cast<Qt::HANDLE>(pollFd.fd), this);
    (void) connect(notifier, &QWinEventNotifier::activated, this, &GstBusPump::drain);
#else
    auto* const notifier = new QSocketNotifier(pollFd.fd, QSocketNotifier::Read, this);
    (void) connect(notifier, &QSocketNotifier::activated, this, &GstBusPump::drain);
#endif
    _notifier = notifier;
    return true;
}

void GstBusPump::detach()
{
    // Before the bus, which owns the fd it watches.
    delete _notifier;
    _notifier = nullptr;

    if (!_bus) {
        return;
    }
    if (_syncInstalled) {
        gst_bus_set_sync_handler(_bus.get(), nullptr, nullptr, nullptr);
        // GStreamer refcounts the handler per post and releases it once the last in-flight call has returned.
        _syncReleased.acquire();
        _syncInstalled = false;
    }
    // Drops what is still queued.
    gst_bus_set_flushing(_bus.get(), TRUE);
    _bus.reset();
}

void GstBusPump::drain()
{
    while (_bus) {
        const GStreamer::GstMessagePtr message = GStreamer::adoptMessage(gst_bus_pop(_bus.get()));
        if (!message) {
            return;
        }
        _handler(message.get());
    }
}

void GstBusPump::pumpUntil(GstMessageType types, const std::function<bool()>& done, const QDeadlineTimer& deadline)
{
    while (_bus && !done() && !deadline.hasExpired()) {
        const GStreamer::GstMessagePtr message = GStreamer::adoptMessage(
            gst_bus_timed_pop_filtered(_bus.get(), static_cast<GstClockTime>(deadline.remainingTimeNSecs()), types));
        if (!message) {
            return;
        }
        _handler(message.get());
    }
}

GStreamer::GstMessagePtr GstBusPump::pop(GstMessageType type)
{
    return GStreamer::adoptMessage(_bus ? gst_bus_pop_filtered(_bus.get(), type) : nullptr);
}

GstBusSyncReply GstBusPump::_onSync(GstBus* /*bus*/, GstMessage* message, gpointer data)
{
    return static_cast<GstBusPump*>(data)->_syncHandler(message);
}
