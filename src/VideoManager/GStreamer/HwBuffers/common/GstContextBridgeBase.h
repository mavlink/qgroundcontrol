#pragma once

#include <QtCore/qglobal.h>

#if defined(QGC_HAS_ANY_GPU_PATH)

#include <atomic>
#include <gst/gst.h>
#include <memory>
#include <span>
#include <vector>

#include <QtCore/QMutex>

#include "GstScoped.h"

QT_FORWARD_DECLARE_CLASS(QLoggingCategory)

/// Process-wide hand-off of Qt's GPU device to GStreamer (GL, D3D11, D3D12). A bridge builds ("primes") GStreamer
/// objects around the device Qt renders with when the first pipeline is built, and sets their contexts on each pipeline
/// before it has elements: GstBin hands them to every element it is given, so decoders allocate memory QRhi can
/// sample. One context type maps to one bridge object. A pipeline built before the bridge can prime waits for Qt's
/// device: an element of it that needs one meanwhile makes a device of its own and posts it, and its bins then hand
/// that device to every element added later. The mutex guards priming and the objects but is released before
/// gst_element_set_context(), which runs the elements' handlers and can post on their bus.
class GstContextBridgeBase
{
public:
    GstContextBridgeBase(const GstContextBridgeBase&) = delete;
    GstContextBridgeBase& operator=(const GstContextBridgeBase&) = delete;

    /// Primes the bridge unless it already is, and sets its contexts on @p pipeline, which then counts among those the
    /// bridge served. A pipeline the bridge can't serve yet is remembered for serveWaitingPipelines(). Thread-safe.
    void provideContexts(GstElement* pipeline);

    /// Primes the bridge unless it already is, or failed for good; true once primed. Thread-safe.
    bool prime();

    /// Sets this bridge's contexts on the top-level bins built before it could prime, priming it now if it can, so
    /// that they replace the device an element made meanwhile for the elements added from now on, and posts
    /// HwBuffers::kGraphicsDeviceServedMessage on each: the elements that have that device keep it. For when Qt's
    /// device becomes known; no-op when no bin waits. Thread-safe.
    void serveWaitingPipelines();

    /// Drops the shared objects and the retry latch, so the next request primes from scratch. Returns the top-level
    /// bins that got this bridge's device and are still alive, which it forgets: every bin served up to the drop,
    /// for HwBuffers::onGraphicsDeviceRemoved() to tell. Thread-safe.
    std::vector<GStreamer::GstElementPtr> reset();

    /// True from noteDeviceRemoved() until the bridge primes again. Thread-safe.
    bool deviceLost() const noexcept { return _deviceLost.load(std::memory_order_acquire); }

protected:
    constexpr GstContextBridgeBase(const char* api, std::span<const char* const> contextTypes,
                                   const QLoggingCategory& (*logCategory)()) noexcept
        : _apiName(api)
        , _contextTypes(contextTypes)
        , _category(logCategory)
    {}

    ~GstContextBridgeBase() = default;

    /// Builds the shared objects; the mutex is held. A false return latches the bridge off until reset() unless the
    /// subclass calls retryLater() first.
    virtual bool primeLocked() = 0;

    /// Transfer-full ref to the object behind @p contextType, or null; the mutex is held.
    virtual GstObject* refObject(const char* contextType) = 0;

    /// Wraps @p object in a new GstContext of @p contextType (the context refs it); the mutex is not held.
    virtual GstContext* buildContext(const char* contextType, GstObject* object) = 0;

    /// Drops the shared objects; the mutex is held.
    virtual void clearLocked() = 0;

    /// For primeLocked(): the next request tries again. Returns false so a failure path can `return retryLater();`.
    bool retryLater() noexcept;

    /// For a subclass whose device reported its removal, on any thread: deviceLost() from now on, and the GUI thread
    /// calls HwBuffers::onGraphicsDeviceRemoved(), which resets every bridge and tells the pipelines they served.
    void noteDeviceRemoved();

    const QLoggingCategory& category() const { return _category(); }

    const char* apiName() const noexcept { return _apiName; }

    QMutex _mutex;

private:
    struct WeakRefDeleter
    {
        void operator()(GWeakRef* ref) const noexcept;
    };

    using WeakRefs = std::vector<std::unique_ptr<GWeakRef, WeakRefDeleter>>;

    bool primeIfNeededLocked();
    /// Refs the object for @p contextType under the mutex, then builds its context unlocked; null when the bridge
    /// shares none.
    GStreamer::GstContextPtr makeContext(const char* contextType);
    /// Sets every context of this bridge on @p bin; the mutex is not held. True when it set one.
    bool setContexts(GstElement* bin);
    /// Adds @p topLevelBin to @p bins, the waiting or the served ones; the mutex is held. The refs it took to look
    /// through the list go to @p released, for the caller to drop without the mutex.
    static void rememberLocked(std::unique_ptr<WeakRefs>& bins, GstElement* topLevelBin,
                               std::vector<GStreamer::GstElementPtr>& released);
    /// The waiting bins that are still alive, which wait no longer and count as served; the mutex is held. The refs
    /// taken meanwhile go to @p released, as for rememberLocked().
    std::vector<GStreamer::GstElementPtr> takeWaitingLocked(std::vector<GStreamer::GstElementPtr>& released);
    /// The served bins that are still alive, which the bridge then forgets; the mutex is held.
    std::vector<GStreamer::GstElementPtr> takeServedLocked();
    /// Sets every context of this bridge on @p topLevelBins, which waited for it, and posts
    /// HwBuffers::kGraphicsDeviceServedMessage on each; the mutex is not held.
    void giveContexts(const std::vector<GStreamer::GstElementPtr>& topLevelBins);

    const char* _apiName;
    std::span<const char* const> _contextTypes;
    const QLoggingCategory& (*_category)();
    bool _primed = false;
    bool _primeAttempted = false;
    /// Top-level bins built before the bridge could prime; null while there was none. Behind a pointer because the
    /// bridges are constant-initialized, which a container isn't everywhere.
    std::unique_ptr<WeakRefs> _waiting;
    /// Top-level bins that got this bridge's context since it was primed.
    std::unique_ptr<WeakRefs> _served;
    std::atomic<bool> _deviceLost{false};
};

#endif  // QGC_HAS_ANY_GPU_PATH
