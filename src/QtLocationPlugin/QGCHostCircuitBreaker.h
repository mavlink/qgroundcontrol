#pragma once

#include <QtCore/QDateTime>
#include <QtCore/QHash>
#include <QtCore/QMutex>
#include <QtCore/QString>
#include <chrono>

#include "QGCLoggingCategory.h"

Q_DECLARE_LOGGING_CATEGORY(QGeoTiledMapReplyQGCLog)

// ---------------------------------------------------------------------------
// Per-host circuit breaker (R2)
//
// Process-wide and mutex-guarded: QGeoTiledMapReplyQGC instances can be created
// on different threads, so the breaker state is a static singleton rather than
// per-reply. Keyed by URL host. After kFailureThreshold consecutive transient
// failures within kFailureWindow, the breaker OPENs for kOpenDuration and new
// requests to that host fast-fail. After kOpenDuration one HALF-OPEN probe is
// allowed through: success CLOSEs and resets, failure re-OPENs for another
// kOpenDuration. Any 2xx success CLOSEs the breaker and clears the counter.
// ---------------------------------------------------------------------------
class HostCircuitBreaker
{
public:
    static HostCircuitBreaker& instance()
    {
        static HostCircuitBreaker s_instance;
        return s_instance;
    }

    // Returns true if a request to host may proceed (CLOSED, or HALF-OPEN probe
    // slot just claimed). Returns false when the breaker is OPEN and not yet due
    // for a probe.
    bool allowRequest(const QString& host)
    {
        if (host.isEmpty()) {
            return true;
        }
        const qint64 now = QDateTime::currentMSecsSinceEpoch();
        QMutexLocker lock(&_mutex);
        // Read path: never default-insert. A host with no entry is CLOSED, so
        // healthy hosts don't accumulate map entries that recordSuccess must prune.
        auto it = _hosts.find(host);
        if (it == _hosts.end()) {
            return true;  // Closed
        }
        HostState& st = *it;

        if (st.state == State::Open) {
            if (now >= st.openUntilMs) {
                // Cooldown elapsed: allow exactly one half-open probe.
                st.state = State::HalfOpen;
                st.probeInFlight = true;
                qCDebug(QGeoTiledMapReplyQGCLog) << "Circuit breaker HALF-OPEN (probe) for host" << host;
                return true;
            }
            return false;
        }

        if (st.state == State::HalfOpen) {
            // Only one probe at a time while half-open.
            return !st.probeInFlight ? (st.probeInFlight = true) : false;
        }

        return true;  // Closed
    }

    void recordSuccess(const QString& host)
    {
        if (host.isEmpty()) {
            return;
        }
        QMutexLocker lock(&_mutex);
        auto it = _hosts.find(host);
        if (it == _hosts.end()) {
            return;
        }
        if (it->state != State::Closed || it->failures != 0) {
            qCDebug(QGeoTiledMapReplyQGCLog) << "Circuit breaker CLOSED (success) for host" << host;
        }
        _hosts.erase(it);
    }

    void recordFailure(const QString& host)
    {
        if (host.isEmpty()) {
            return;
        }
        const qint64 now = QDateTime::currentMSecsSinceEpoch();
        QMutexLocker lock(&_mutex);
        // operator[] must default-insert to count consecutive failures, so a host
        // that fails once and is never retried would leak an entry — bound the map.
        if (_hosts.size() >= kPruneThreshold) {
            _pruneStale(now);
        }
        HostState& st = _hosts[host];

        if (st.state == State::HalfOpen) {
            // Probe failed: re-open for another cooldown.
            st.state = State::Open;
            st.openUntilMs = now + kOpenDuration.count();
            st.probeInFlight = false;
            qCDebug(QGeoTiledMapReplyQGCLog) << "Circuit breaker re-OPEN (probe failed) for host" << host;
            return;
        }

        // Reset the consecutive counter if the failure window has lapsed.
        if ((st.firstFailureMs == 0) || ((now - st.firstFailureMs) > kFailureWindow.count())) {
            st.firstFailureMs = now;
            st.failures = 0;
        }
        ++st.failures;

        if (st.failures >= kFailureThreshold) {
            st.state = State::Open;
            st.openUntilMs = now + kOpenDuration.count();
            qCDebug(QGeoTiledMapReplyQGCLog)
                << "Circuit breaker OPEN for host" << host << "after" << st.failures << "failures";
        }
    }

private:
    enum class State
    {
        Closed,
        Open,
        HalfOpen
    };

    struct HostState
    {
        State state = State::Closed;
        int failures = 0;
        qint64 firstFailureMs = 0;
        qint64 openUntilMs = 0;
        bool probeInFlight = false;
    };

    // Erase Closed entries whose failure window has lapsed; Open/HalfOpen entries
    // are load-bearing and kept. Caller must hold _mutex.
    void _pruneStale(qint64 now)
    {
        for (auto it = _hosts.begin(); it != _hosts.end();) {
            const bool lapsed = (it->firstFailureMs == 0) || ((now - it->firstFailureMs) > kFailureWindow.count());
            it = (it->state == State::Closed && lapsed) ? _hosts.erase(it) : std::next(it);
        }
    }

    static constexpr int kFailureThreshold = 5;
    static constexpr int kPruneThreshold = 64;
    static constexpr std::chrono::milliseconds kFailureWindow = std::chrono::seconds(30);
    static constexpr std::chrono::milliseconds kOpenDuration = std::chrono::seconds(60);

    QMutex _mutex;
    QHash<QString, HostState> _hosts;
};

// ---------------------------------------------------------------------------
// Per-host concurrency gate (A2)
//
// The live-tile fetcher (QtLocation engine thread) and the bulk downloader
// (QGCCachedTileSet, main thread) own separate QNetworkAccessManagers and so
// cannot share one socket pool safely. Left uncoordinated each runs its own
// budget, letting up to 2N simultaneous requests reach a single provider host
// and doubling the rate that host sees. This process-wide, mutex-guarded
// singleton enforces a SINGLE combined in-flight cap per host across both
// paths: a slot is claimed before a GET is issued and released when it
// finishes. OSM hosts are clamped to kOSMHostLimit so the tile-usage-policy
// guarantee holds no matter which path (or how many) issues the request.
// ---------------------------------------------------------------------------
class HostConcurrencyGate
{
public:
    static HostConcurrencyGate& instance()
    {
        static HostConcurrencyGate s_instance;
        return s_instance;
    }

    // Claim an in-flight slot for host if under its cap. limit is the caller's
    // transport-scaled budget; the effective cap is min(limit, kOSMHostLimit)
    // for OSM so a larger live budget can never exceed the OSM policy. An empty
    // host (unit tests with no real URL) is never gated.
    bool tryAcquire(const QString& host, int limit, bool isOSM)
    {
        if (host.isEmpty()) {
            return true;
        }
        const int cap = isOSM ? qMin(limit, kOSMHostLimit) : limit;
        QMutexLocker lock(&_mutex);
        int& active = _active[host];
        if (active >= cap) {
            return false;
        }
        ++active;
        return true;
    }

    void release(const QString& host)
    {
        if (host.isEmpty()) {
            return;
        }
        QMutexLocker lock(&_mutex);
        const auto it = _active.find(host);
        if (it == _active.end()) {
            return;
        }
        if (--(*it) <= 0) {
            _active.erase(it);
        }
    }

#ifdef QGC_UNITTEST_BUILD
    int activeForTest(const QString& host)
    {
        QMutexLocker lock(&_mutex);
        return _active.value(host, 0);
    }

    void resetForTest()
    {
        QMutexLocker lock(&_mutex);
        _active.clear();
    }
#endif

    static constexpr int kOSMHostLimit = 2;

private:
    QMutex _mutex;
    QHash<QString, int> _active;
};
