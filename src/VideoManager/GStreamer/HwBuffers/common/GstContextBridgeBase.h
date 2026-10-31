#pragma once

#include <QtCore/qglobal.h>

#if defined(QGC_HAS_ANY_GPU_PATH)

#include <gst/gst.h>
#include <span>

#include <QtCore/QMutex>

#include "GstScoped.h"

QT_FORWARD_DECLARE_CLASS(QLoggingCategory)

/// Process-wide hand-off of Qt's GPU device to GStreamer (GL, Vulkan, D3D11, D3D12). A bridge lazily builds ("primes")
/// GStreamer objects around the device Qt renders with and answers every NEED_CONTEXT / GST_QUERY_CONTEXT for its
/// context types with them, so decoders allocate memory QRhi can sample. One context type maps to one bridge object.
/// The mutex guards priming and the objects but is released before gst_element_set_context()/gst_query_set_context():
/// both re-enter bus handlers through nested context queries and would self-deadlock.
class GstContextBridgeBase
{
public:
    GstContextBridgeBase(const GstContextBridgeBase&) = delete;
    GstContextBridgeBase& operator=(const GstContextBridgeBase&) = delete;

    /// Answers a NEED_CONTEXT for one of this bridge's context types: GST_BUS_DROP when consumed (the message is
    /// unref'd), else GST_BUS_PASS. Thread-safe.
    GstBusSyncReply handleSyncMessage(GstMessage* message);

    /// Answers a GST_QUERY_CONTEXT for one of this bridge's context types; true when consumed. Thread-safe.
    bool answerContextQuery(GstQuery* query);

    /// Primes the bridge unless it already is, or failed for good; true once primed. Thread-safe.
    bool prime();

    /// Drops the shared objects and the retry latch, so the next request primes from scratch.
    void reset();

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

    /// True on the retryWithinBudget() call that ran out of retries, for a single give-up log line.
    bool retriesJustExhausted() const noexcept { return _failedPrimes == kMaxPrimeRetries + 1; }

    /// Native-handle misses so far, counted by retryWithinBudget().
    int failedPrimes() const noexcept { return _failedPrimes; }

    const QLoggingCategory& category() const { return _category(); }

    const char* apiName() const noexcept { return _apiName; }

    QMutex _mutex;

private:
    const char* matchContextType(const char* type) const;
    bool primeIfNeededLocked();
    /// Primes, refs the object for @p contextType under the mutex, then builds its context unlocked; null when the
    /// bridge can't serve it.
    GStreamer::GstContextPtr makeContext(const char* contextType);

    const char* _apiName;
    std::span<const char* const> _contextTypes;
    const QLoggingCategory& (*_category)();
    bool _primed = false;
    bool _primeAttempted = false;
    int _failedPrimes = 0;
};

#endif  // QGC_HAS_ANY_GPU_PATH
