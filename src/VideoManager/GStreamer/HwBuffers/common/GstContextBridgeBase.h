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

/// Process-wide hand-off of Qt's GPU device to GStreamer (GL, Vulkan, D3D11, D3D12). A bridge lazily builds ("primes")
/// GStreamer objects around the device Qt renders with and answers every NEED_CONTEXT / GST_QUERY_CONTEXT for its
/// context types with them, so decoders allocate memory QRhi can sample. One context type maps to one bridge object.
/// A context goes to the whole pipeline of the element that asked: an element left without one makes a device of its
/// own and posts it, and its bins then hand that device to every element added later without asking the application.
/// The mutex guards priming and the objects but is released before gst_element_set_context()/gst_query_set_context():
/// both re-enter bus handlers through nested context queries and would self-deadlock.
class GstContextBridgeBase
{
public:
    GstContextBridgeBase(const GstContextBridgeBase&) = delete;
    GstContextBridgeBase& operator=(const GstContextBridgeBase&) = delete;

    /// Answers a NEED_CONTEXT for one of this bridge's context types by setting the context on the top-level bin of
    /// the element that asked: GST_BUS_DROP when consumed (the message is unref'd), else GST_BUS_PASS. A bin the bridge
    /// can't serve yet is remembered for serveWaitingPipelines(). Thread-safe.
    GstBusSyncReply handleSyncMessage(GstMessage* message);

    /// Answers a GST_QUERY_CONTEXT for one of this bridge's context types; true when consumed. @p asker, an element
    /// of the pipeline the query came through, has that pipeline count among those the bridge served. Thread-safe.
    bool answerContextQuery(GstQuery* query, GstElement* asker = nullptr);

    /// Primes the bridge unless it already is, or failed for good; true once primed. Thread-safe.
    bool prime();

    /// Sets this bridge's contexts on the top-level bins that asked before it could prime, priming it now if it can,
    /// so that they replace the device an element made meanwhile for the elements added from now on, and posts
    /// HwBuffers::kGraphicsDeviceServedMessage on each: the elements that have that device keep it. For when Qt's
    /// device becomes known; no-op when no bin waits. Thread-safe.
    void serveWaitingPipelines();

    /// Drops the shared objects and the retry latch, so the next request primes from scratch. Returns the top-level
    /// bins that got this bridge's device and are still alive, which it forgets: every bin served up to the drop,
    /// for HwBuffers::onGraphicsDeviceRemoved() to tell. Thread-safe.
    std::vector<GStreamer::GstElementPtr> reset();

    /// True from noteDeviceRemoved() until the bridge primes again. Thread-safe.
    bool deviceLost() const noexcept { return _deviceLost.load(std::memory_order_acquire); }

    /// Lets a bridge that ran out of prime retries try again (pipeline restart); no-op otherwise.
    void rearm();

protected:
    /// Prime retries for a native handle that isn't available yet (Qt's RHI may initialize after the first pipeline).
    static constexpr int kMaxPrimeRetries = 16;

    constexpr GstContextBridgeBase(const char* api, std::span<const char* const> contextTypes,
                                   const QLoggingCategory& (*logCategory)()) noexcept
        : _apiName(api)
        , _contextTypes(contextTypes)
        , _category(logCategory)
    {}

    ~GstContextBridgeBase() = default;

    /// Builds the shared objects; the mutex is held. A false return latches the bridge off until reset() unless the
    /// subclass calls retryLater() or retryWithinBudget() first.
    virtual bool primeLocked() = 0;

    /// Transfer-full ref to the object behind @p contextType, or null; the mutex is held.
    virtual GstObject* refObject(const char* contextType) = 0;

    /// Wraps @p object in a new GstContext of @p contextType (the context refs it); the mutex is not held.
    virtual GstContext* buildContext(const char* contextType, GstObject* object) = 0;

    /// Drops the shared objects; the mutex is held.
    virtual void clearLocked() = 0;

    /// For primeLocked(): the next request tries again. Returns false so a failure path can `return retryLater();`.
    bool retryLater() noexcept;

    /// For primeLocked() when the native handle is missing: retryLater() for the first kMaxPrimeRetries calls, after
    /// which the bridge stays off until rearm() or reset(). True while a retry was granted.
    bool retryWithinBudget() noexcept;

    /// For a subclass whose device reported its removal, on any thread: deviceLost() from now on, and the GUI thread
    /// calls HwBuffers::onGraphicsDeviceRemoved(), which resets every bridge and tells the pipelines they served.
    void noteDeviceRemoved();

    /// True on the retryWithinBudget() call that ran out of retries, for a single give-up log line.
    bool retriesJustExhausted() const noexcept { return _failedPrimes == kMaxPrimeRetries + 1; }

    /// Native-handle misses so far, counted by retryWithinBudget().
    int failedPrimes() const noexcept { return _failedPrimes; }

    const QLoggingCategory& category() const { return _category(); }

    const char* apiName() const noexcept { return _apiName; }

    QMutex _mutex;

private:
    struct WeakRefDeleter
    {
        void operator()(GWeakRef* ref) const noexcept;
    };

    using WeakRefs = std::vector<std::unique_ptr<GWeakRef, WeakRefDeleter>>;

    const char* matchContextType(const char* type) const;
    bool primeIfNeededLocked();
    /// Primes, refs the object for @p contextType under the mutex, then builds its context unlocked, and
    /// @p topLevelBin (if any) counts as served; null when the bridge can't serve it, and @p topLevelBin then waits
    /// for serveWaitingPipelines() if @p mayWait.
    GStreamer::GstContextPtr makeContext(const char* contextType, GstElement* topLevelBin = nullptr,
                                         bool mayWait = false);
    /// Adds @p topLevelBin to @p bins, the waiting or the served ones; the mutex is held. The refs it took to look
    /// through the list go to @p released, for the caller to drop without the mutex.
    static void rememberLocked(std::unique_ptr<WeakRefs>& bins, GstElement* topLevelBin,
                               std::vector<GStreamer::GstElementPtr>& released);
    /// The waiting bins that are still alive, which wait no longer and count as served; the mutex is held. The refs
    /// taken meanwhile go to @p released, as for rememberLocked().
    std::vector<GStreamer::GstElementPtr> takeWaitingLocked(std::vector<GStreamer::GstElementPtr>& released);
    /// The served bins that are still alive, which the bridge then forgets; the mutex is held.
    std::vector<GStreamer::GstElementPtr> takeServedLocked();
    /// Sets every context of this bridge on @p topLevelBins; the mutex is not held.
    void giveContexts(const std::vector<GStreamer::GstElementPtr>& topLevelBins);

    const char* _apiName;
    std::span<const char* const> _contextTypes;
    const QLoggingCategory& (*_category)();
    bool _primed = false;
    bool _primeAttempted = false;
    int _failedPrimes = 0;
    /// Top-level bins an element of which asked for a context the bridge couldn't give yet; null while there was
    /// none. Behind a pointer because the bridges are constant-initialized, which a container isn't everywhere.
    std::unique_ptr<WeakRefs> _waiting;
    /// Top-level bins that got this bridge's context since it was primed.
    std::unique_ptr<WeakRefs> _served;
    std::atomic<bool> _deviceLost{false};
};

#endif  // QGC_HAS_ANY_GPU_PATH
