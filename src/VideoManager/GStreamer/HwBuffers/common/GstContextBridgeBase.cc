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

void GstContextBridgeBase::rememberServedLocked(GstElement* topLevelBin,
                                                std::vector<GStreamer::GstElementPtr>& released)
{
    if (!_served) {
        _served = std::make_unique<WeakRefs>();
    }
    bool known = false;
    std::erase_if(*_served, [&](const auto& ref) {
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
        _served->emplace_back(ref);
    }
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

void GstContextBridgeBase::provideContexts(GstElement* pipeline)
{
    // Released without the mutex.
    std::vector<GStreamer::GstElementPtr> released;
    {
        QMutexLocker lock(&_mutex);
        if (!primeIfNeededLocked()) {
            return;
        }
        rememberServedLocked(pipeline, released);
    }
    for (const char* const contextType : _contextTypes) {
        if (const GStreamer::GstContextPtr context = makeContext(contextType)) {
            gst_element_set_context(pipeline, context.get());
            qCDebug(category()) << "Provided" << _apiName << contextType << "context to" << GST_ELEMENT_NAME(pipeline);
        }
    }
}

bool GstContextBridgeBase::prime()
{
    QMutexLocker lock(&_mutex);
    return primeIfNeededLocked();
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
