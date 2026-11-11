#include "GstContextBridgeBase.h"

#if defined(QGC_HAS_ANY_GPU_PATH)

#include <glib.h>
#include <utility>

#include <QtCore/QLoggingCategory>
#include <QtCore/QMutexLocker>

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
    return _primed;
}

void GstContextBridgeBase::rememberWaitingLocked(GstElement* topLevelBin,
                                                 std::vector<GStreamer::GstElementPtr>& released)
{
    if (!_waiting) {
        _waiting = std::make_unique<WeakRefs>();
    }
    bool known = false;
    std::erase_if(*_waiting, [&](const auto& ref) {
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
        _waiting->emplace_back(ref);
    }
}

std::vector<GStreamer::GstElementPtr> GstContextBridgeBase::takeWaitingLocked()
{
    std::vector<GStreamer::GstElementPtr> bins;
    if (const std::unique_ptr<WeakRefs> waiting = std::move(_waiting)) {
        for (const auto& ref : *waiting) {
            if (GstElement* const bin = GST_ELEMENT(g_weak_ref_get(ref.get()))) {
                bins.push_back(GStreamer::adoptElement(bin));
            }
        }
    }
    return bins;
}

void GstContextBridgeBase::giveContexts(const std::vector<GStreamer::GstElementPtr>& topLevelBins)
{
    for (const GStreamer::GstElementPtr& bin : topLevelBins) {
        for (const char* const contextType : _contextTypes) {
            if (const GStreamer::GstContextPtr context = makeContext(contextType)) {
                gst_element_set_context(bin.get(), context.get());
                qCDebug(category()) << "Provided" << _apiName << contextType << "context to"
                                    << GST_ELEMENT_NAME(bin.get()) << "which asked before the bridge was primed";
            }
        }
    }
}

GStreamer::GstContextPtr GstContextBridgeBase::makeContext(const char* contextType, GstElement* waiting)
{
    GStreamer::GstObjectPtr object;
    // Released, and served, without the mutex.
    std::vector<GStreamer::GstElementPtr> bins;
    {
        QMutexLocker lock(&_mutex);
        const bool wasPrimed = _primed;
        if (!primeIfNeededLocked()) {
            if (waiting) {
                rememberWaitingLocked(waiting, bins);
            }
            return nullptr;
        }
        if (!wasPrimed) {
            bins = takeWaitingLocked();
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
    const GStreamer::GstContextPtr context = makeContext(matched, topLevelBin.get());
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

bool GstContextBridgeBase::answerContextQuery(GstQuery* query)
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

    const GStreamer::GstContextPtr context = makeContext(matched);
    if (!context) {
        return false;
    }
    gst_query_set_context(query, context.get());
    return true;
}

bool GstContextBridgeBase::prime()
{
    std::vector<GStreamer::GstElementPtr> bins;
    {
        QMutexLocker lock(&_mutex);
        const bool wasPrimed = _primed;
        if (!primeIfNeededLocked()) {
            return false;
        }
        if (!wasPrimed) {
            bins = takeWaitingLocked();
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

void GstContextBridgeBase::reset()
{
    QMutexLocker lock(&_mutex);
    clearLocked();
    _primed = false;
    _primeAttempted = false;
    _failedPrimes = 0;
    qCDebug(category()) << _apiName << "bridge reset";
}

void GstContextBridgeBase::rearm()
{
    QMutexLocker lock(&_mutex);
    if (!_primed && _primeAttempted && (_failedPrimes > kMaxPrimeRetries)) {
        _primeAttempted = false;
        _failedPrimes = 0;
        qCDebug(category()) << _apiName << "bridge rearmed after exhausting its prime retries";
    }
}

bool GstContextBridgeBase::retryLater() noexcept
{
    _primeAttempted = false;
    return false;
}

bool GstContextBridgeBase::retryWithinBudget() noexcept
{
    ++_failedPrimes;
    if (_failedPrimes <= kMaxPrimeRetries) {
        _primeAttempted = false;
        return true;
    }
    return false;
}

#endif  // QGC_HAS_ANY_GPU_PATH
