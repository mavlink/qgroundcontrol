#include "GstContextBridgeBase.h"

#include <glib.h>
#include <utility>

#include <QtCore/QCoreApplication>
#include <QtCore/QLoggingCategory>
#include <QtCore/QMutexLocker>

#include "HwBuffers.h"

void GstContextBridgeBase::WeakRefDeleter::operator()(GWeakRef* ref) const noexcept
{
    g_weak_ref_clear(ref);
    delete ref;
}

bool GstContextBridgeBase::primeIfNeededLocked()
{
    if (_primed) {
        return true;
    }
    if (_primeAttempted) {
        return false;
    }
    _primeAttempted = true;
    _primed = primeLocked();
    if (_primed) {
        _deviceLost.store(false, std::memory_order_release);
    }
    return _primed;
}

void GstContextBridgeBase::rememberLocked(std::unique_ptr<WeakRefs>& bins, GstElement* topLevelBin,
                                          std::vector<GStreamer::GstElementPtr>& released)
{
    if (!bins) {
        bins = std::make_unique<WeakRefs>();
    }
    bool known = false;
    std::erase_if(*bins, [&](const auto& ref) {
        GStreamer::GstElementPtr bin = GStreamer::adoptElement(GST_ELEMENT(g_weak_ref_get(ref.get())));
        if (!bin) {
            return true;
        }
        known = known || (bin.get() == topLevelBin);
        released.push_back(std::move(bin));
        return false;
    });
    if (!known) {
        GWeakRef* const ref = new GWeakRef;
        g_weak_ref_init(ref, topLevelBin);
        bins->emplace_back(ref);
    }
}

std::vector<GStreamer::GstElementPtr> GstContextBridgeBase::takeWaitingLocked(
    std::vector<GStreamer::GstElementPtr>& released)
{
    std::vector<GStreamer::GstElementPtr> bins;
    if (const std::unique_ptr<WeakRefs> waiting = std::move(_waiting)) {
        for (const auto& ref : *waiting) {
            if (GstElement* const bin = GST_ELEMENT(g_weak_ref_get(ref.get()))) {
                bins.push_back(GStreamer::adoptElement(bin));
            }
        }
    }
    // They are given the contexts next.
    for (const GStreamer::GstElementPtr& bin : bins) {
        rememberLocked(_served, bin.get(), released);
    }
    return bins;
}

std::vector<GStreamer::GstElementPtr> GstContextBridgeBase::takeServedLocked()
{
    std::vector<GStreamer::GstElementPtr> bins;
    if (const std::unique_ptr<WeakRefs> served = std::move(_served)) {
        for (const auto& ref : *served) {
            if (GstElement* const bin = GST_ELEMENT(g_weak_ref_get(ref.get()))) {
                bins.push_back(GStreamer::adoptElement(bin));
            }
        }
    }
    return bins;
}

void GstContextBridgeBase::noteDeviceRemoved()
{
    _deviceLost.store(true, std::memory_order_release);
    // The device may notify with its own locks held, on a thread it joins when it is disposed: nothing is dropped,
    // and no bridge's mutex taken, before the GUI thread gets to it.
    if (QCoreApplication* const app = QCoreApplication::instance()) {
        (void) QMetaObject::invokeMethod(app, [] { HwBuffers::onGraphicsDeviceRemoved(); }, Qt::QueuedConnection);
    }
}

GStreamer::GstContextPtr GstContextBridgeBase::makeContext(const char* contextType)
{
    GStreamer::GstObjectPtr object;
    {
        QMutexLocker lock(&_mutex);
        object = GStreamer::adoptObject(refObject(contextType));
    }
    if (!object) {
        return nullptr;
    }
    // The context holds its own ref on the object.
    return GStreamer::adoptContext(buildContext(contextType, object.get()));
}

bool GstContextBridgeBase::setContexts(GstElement* bin)
{
    bool given = false;
    for (const char* const contextType : _contextTypes) {
        if (const GStreamer::GstContextPtr context = makeContext(contextType)) {
            gst_element_set_context(bin, context.get());
            given = true;
        }
    }
    return given;
}

void GstContextBridgeBase::giveContexts(const std::vector<GStreamer::GstElementPtr>& topLevelBins)
{
    for (const GStreamer::GstElementPtr& bin : topLevelBins) {
        if (!setContexts(bin.get())) {
            continue;
        }
        qCDebug(category()) << "Provided" << _apiName << "contexts to" << GST_ELEMENT_NAME(bin.get())
                            << "which was built before the bridge was primed";
        // An element that has a device ignores the context: only the bin's owner can replace it.
        (void) gst_element_post_message(
            bin.get(), gst_message_new_element(GST_OBJECT(bin.get()),
                                               gst_structure_new_empty(HwBuffers::kGraphicsDeviceServedMessage)));
    }
}

void GstContextBridgeBase::provideContexts(GstElement* pipeline)
{
    // Released, and served, without the mutex.
    std::vector<GStreamer::GstElementPtr> released;
    std::vector<GStreamer::GstElementPtr> waiting;
    {
        QMutexLocker lock(&_mutex);
        const bool wasPrimed = _primed;
        if (!primeIfNeededLocked()) {
            rememberLocked(_waiting, pipeline, released);
            return;
        }
        if (!wasPrimed) {
            waiting = takeWaitingLocked(released);
        }
        rememberLocked(_served, pipeline, released);
    }
    giveContexts(waiting);
    if (setContexts(pipeline)) {
        qCDebug(category()) << "Provided" << _apiName << "contexts to" << GST_ELEMENT_NAME(pipeline);
    }
}

bool GstContextBridgeBase::prime()
{
    std::vector<GStreamer::GstElementPtr> released;
    std::vector<GStreamer::GstElementPtr> bins;
    {
        QMutexLocker lock(&_mutex);
        const bool wasPrimed = _primed;
        if (!primeIfNeededLocked()) {
            return false;
        }
        if (!wasPrimed) {
            bins = takeWaitingLocked(released);
        }
    }
    giveContexts(bins);
    return true;
}

void GstContextBridgeBase::serveWaitingPipelines()
{
    {
        QMutexLocker lock(&_mutex);
        if (!_waiting || _waiting->empty()) {
            return;
        }
    }
    (void) prime();
}

std::vector<GStreamer::GstElementPtr> GstContextBridgeBase::reset()
{
    QMutexLocker lock(&_mutex);
    // Under the lock that drops the objects: a bin served on another thread is either among these, or primes anew.
    // The caller drops the refs, without the mutex.
    std::vector<GStreamer::GstElementPtr> served = takeServedLocked();
    clearLocked();
    _primed = false;
    _primeAttempted = false;
    qCDebug(category()) << _apiName << "bridge reset";
    return served;
}

bool GstContextBridgeBase::retryLater() noexcept
{
    _primeAttempted = false;
    return false;
}
