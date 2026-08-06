#pragma once

#include <QtCore/qglobal.h>

#if defined(QGC_HAS_ANY_GPU_PATH)

/// Shared prime-retry latch for the GL/Vulkan context bridges. Both wrap a QRhi-derived native handle that may be null
/// until Qt's RHI initializes, so they retry priming a bounded number of times before latching off. Only the
/// boolean/counter bookkeeping is shared here; each bridge keeps its own bail/teardown logic (the GstObject sets they
/// clear differ).
namespace GstBridgePrimeRetry {

struct PrimeRetryState
{
    bool primed = false;
    bool primeAttempted = false;
    int nullCount = 0;
    int maxRetries = 16;
};

enum class Decision
{
    AlreadyPrimed,  ///< prime() already succeeded; caller returns true.
    ShouldRetry,    ///< not yet primed and retries remain; caller proceeds to prime.
    GiveUp          ///< latched off (already attempted, or retry budget exhausted); caller returns false.
};

/// Call at the top of primeLocked(). On ShouldRetry the state is marked attempted so the caller proceeds; the caller
/// is responsible for clearing primeAttempted on a recoverable bail (so a later attempt can retry).
inline Decision primeRetryGuard(PrimeRetryState& s)
{
    if (s.primed) {
        return Decision::AlreadyPrimed;
    }
    if (s.primeAttempted) {
        return Decision::GiveUp;
    }
    s.primeAttempted = true;
    return Decision::ShouldRetry;
}

/// Record a null native-handle attempt. Returns true while retries remain (caller should clear primeAttempted to allow
/// a later retry); false once the budget is exhausted (caller leaves primeAttempted latched). The transition (return
/// false on the first over-budget call) lets callers log a one-shot give-up message.
inline bool rearmRetry(PrimeRetryState& s)
{
    ++s.nullCount;
    if (s.nullCount <= s.maxRetries) {
        s.primeAttempted = false;
        return true;
    }
    return false;
}

/// True exactly once, on the attempt that first exceeds the retry budget — for a single give-up log line.
inline bool justGaveUp(const PrimeRetryState& s)
{
    return s.nullCount == s.maxRetries + 1;
}

/// Clear the latch so the next prime() retries from scratch. Used by reset().
inline void resetRetry(PrimeRetryState& s)
{
    s.primed = false;
    s.primeAttempted = false;
    s.nullCount = 0;
}

/// Pipeline-restart rearm: if priming latched off after exhausting retries, clear the latch for a fresh attempt.
/// Returns true if it cleared an exhausted latch (caller may log).
inline bool rearmAfterExhaustion(PrimeRetryState& s)
{
    if (s.primed) {
        return false;
    }
    if (s.primeAttempted && s.nullCount > s.maxRetries) {
        s.primeAttempted = false;
        s.nullCount = 0;
        return true;
    }
    return false;
}

}  // namespace GstBridgePrimeRetry

#endif  // QGC_HAS_ANY_GPU_PATH
