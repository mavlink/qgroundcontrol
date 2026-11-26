#include "GstContextBridgeBase.h"

#include <glib.h>
#include <utility>

#include <QtCore/QCoreApplication>
#include <QtCore/QLoggingCategory>
#include <QtCore/QMutexLocker>

#include "HwBuffers.h"

namespace {

/// The bin at the top of @p element's hierarchy, a pipeline for an element that posts on a bus; @p element itself
/// when it has no parent.
GStreamer::GstElementPtr topLevelBinOf(GstElement* element)
{
    GStreamer::GstObjectPtr top = GStreamer::adoptObject(GST_OBJECT(gst_object_ref(element)));
    while (GStreamer::GstObjectPtr parent = GStreamer::adoptObject(gst_object_get_parent(top.get()))) {
        top = std::move(parent);
    }
    return GStreamer::adoptElement(GST_ELEMENT(top.release()));
}

}  // namespace

void GstContextBridgeBase::WeakRefDeleter::operator()(GWeakRef* ref) const noexcept
{
    g_weak_ref_clear(ref);
    delete ref;
}

const char* GstContextBridgeBase::matchContextType(const char* type) const
{
    if (!type) {
        return nullptr;
    }
    for (const char* const known : _contextTypes) {
        if (g_strcmp0(type, known) == 0) {
            return known;
        }
    }
    return nullptr;
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

void GstContextBridgeBase::giveContexts(const std::vector<GStreamer::GstElementPtr>& topLevelBins)
{
    for (const GStreamer::GstElementPtr& bin : topLevelBins) {
        bool given = false;
        for (const char* const contextType : _contextTypes) {
            if (const GStreamer::GstContextPtr context = makeContext(contextType)) {
                gst_element_set_context(bin.get(), context.get());
                given = true;
                qCDebug(category()) << "Provided" << _apiName << contextType << "context to"
                                    << GST_ELEMENT_NAME(bin.get()) << "which asked before the bridge was primed";
            }
        }
        if (given) {
            // An element that has a device ignores the context: only the bin's owner can replace it.
            (void) gst_element_post_message(
                bin.get(), gst_message_new_element(GST_OBJECT(bin.get()),
                                                   gst_structure_new_empty(HwBuffers::kGraphicsDeviceServedMessage)));
        }
    }
}

GStreamer::GstContextPtr GstContextBridgeBase::makeContext(const char* contextType, GstElement* topLevelBin,
                                                           bool mayWait)
{
    GStreamer::GstObjectPtr object;
    // Released, and served, without the mutex.
    std::vector<GStreamer::GstElementPtr> released;
    std::vector<GStreamer::GstElementPtr> bins;
    {
        QMutexLocker lock(&_mutex);
        const bool wasPrimed = _primed;
        if (!primeIfNeededLocked()) {
            if (topLevelBin && mayWait) {
                rememberLocked(_waiting, topLevelBin, released);
            }
            return nullptr;
        }
        if (!wasPrimed) {
            bins = takeWaitingLocked(released);
        }
        if (topLevelBin) {
            rememberLocked(_served, topLevelBin, released);
        }
        object = GStreamer::adoptObject(refObject(contextType));
    }
    giveContexts(bins);
    if (!object) {
        return nullptr;
    }
    // The context holds its own ref on the object.
    return GStreamer::adoptContext(buildContext(contextType, object.get()));
}

GstBusSyncReply GstContextBridgeBase::handleSyncMessage(GstMessage* message)
{
    if (GST_MESSAGE_TYPE(message) != GST_MESSAGE_NEED_CONTEXT) {
        return GST_BUS_PASS;
    }
    const gchar* contextType = nullptr;
    if (!gst_message_parse_context_type(message, &contextType)) {
        return GST_BUS_PASS;
    }
    const char* matched = matchContextType(contextType);
    if (!matched) {
        return GST_BUS_PASS;
    }
    GstElement* element = GST_ELEMENT(GST_MESSAGE_SRC(message));
    if (!element) {
        return GST_BUS_PASS;
    }

    const GStreamer::GstElementPtr topLevelBin = topLevelBinOf(element);
    const GStreamer::GstContextPtr context = makeContext(matched, topLevelBin.get(), /*mayWait=*/true);
    if (!context) {
        return GST_BUS_PASS;
    }
    // The bin sets it on @p element and on every other element it holds or is given later.
    gst_element_set_context(topLevelBin.get(), context.get());
    qCDebug(category()) << "Provided" << _apiName << matched << "context to" << GST_ELEMENT_NAME(topLevelBin.get())
                        << "for" << GST_ELEMENT_NAME(element);
    // `element` is borrowed from the message, so the message must outlive its last use.
    gst_message_unref(message);
    return GST_BUS_DROP;
}

bool GstContextBridgeBase::answerContextQuery(GstQuery* query, GstElement* asker)
{
    if (!query || GST_QUERY_TYPE(query) != GST_QUERY_CONTEXT) {
        return false;
    }
    const gchar* contextType = nullptr;
    if (!gst_query_parse_context_type(query, &contextType)) {
        return false;
    }
    const char* matched = matchContextType(contextType);
    if (!matched) {
        return false;
    }

    // The element that asks takes the device from the answer, without a NEED_CONTEXT that would name its pipeline.
    const GStreamer::GstElementPtr topLevelBin = asker ? topLevelBinOf(asker) : nullptr;
    const GStreamer::GstContextPtr context = makeContext(matched, topLevelBin.get());
    if (!context) {
        return false;
    }
    gst_query_set_context(query, context.get());
    return true;
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
