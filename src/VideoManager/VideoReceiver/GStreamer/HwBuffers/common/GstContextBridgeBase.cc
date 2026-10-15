#include "GstContextBridgeBase.h"

#if defined(QGC_HAS_ANY_GPU_PATH)

#include <glib.h>

#include <QtCore/QLoggingCategory>
#include <QtCore/QMutexLocker>

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

GStreamer::GstContextPtr GstContextBridgeBase::makeContext(const char* contextType)
{
    GStreamer::GstObjectPtr object;
    {
        QMutexLocker lock(&_mutex);
        if (!primeIfNeededLocked()) {
            return nullptr;
        }
        object = GStreamer::adoptObject(refObject(contextType));
    }
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

    const GStreamer::GstContextPtr context = makeContext(matched);
    if (!context) {
        return GST_BUS_PASS;
    }
    gst_element_set_context(element, context.get());
    qCDebug(category()) << "Provided" << _apiName << matched << "context to" << GST_ELEMENT_NAME(element);
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
    QMutexLocker lock(&_mutex);
    return primeIfNeededLocked();
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
