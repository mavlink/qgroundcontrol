#pragma once

#include <QtCore/QByteArrayView>
#include <QtCore/QDeadlineTimer>
#include <QtCore/QReadWriteLock>
#include <QtCore/QString>
#include <atomic>
#include <cstdint>

#include "AutoSuspendGuard.h"
#include "MAVLinkMessageType.h"
#include "MAVLinkSigning.h"

class SigningController;

/// \brief Owns MAVLink signing state for one channel: signing/streams structs, key hint, and RW lock.
///
class SigningChannel
{
public:
    SigningChannel() = default;
    ~SigningChannel() = default;
    SigningChannel(const SigningChannel&) = delete;
    SigningChannel& operator=(const SigningChannel&) = delete;
    SigningChannel(SigningChannel&&) = delete;
    SigningChannel& operator=(SigningChannel&&) = delete;

    /// 60s post-reboot timestamp bump (matches ArduPilot GCS_Signing.cpp); absorbs SIGKILL/suspend/NTP/clock-skew gaps.
    static constexpr uint64_t kPersistedTimestampSafetyBumpTicks = 6'000'000;

    /// Initialize signing for `channel`; empty key disables. `signOutgoing=false` is inbound-verify-only (pending-enable).
    /// Non-empty `keyName` seeds `_signing.timestamp` with max(wallClock, persistedTimestamp+safetyBump) and records keyHint.
    bool init(mavlink_channel_t channel, QByteArrayView key, mavlink_accept_unsigned_t callback,
              uint64_t persistedTimestamp = 0, const QString& keyName = {}, bool signOutgoing = true);

    /// Swap the accept-unsigned callback without resetting the key. Returns false if signing isn't enabled.
    bool setAcceptUnsignedCallback(mavlink_accept_unsigned_t callback);

    bool isEnabled() const;
    int streamCount() const;

    QString keyHint() const;
    void setKeyHint(const QString& name);
    void clearKeyHint();

    struct TimestampSnapshot
    {
        uint64_t timestamp;
        QString keyName;
    };

    /// Returns current timestamp and active key name. Returns {0, ""} when signing is not enabled.
    TimestampSnapshot currentTimestampAndName() const;

    /// While suspended, tryDetectKey is suppressed to block stale-key installs during pending enable.
    bool isAutoDetectSuspended() const;

    /// RAII handle that suspends auto-detect for the guard's lifetime; release is automatic on destruction.
    [[nodiscard]] QGC::AutoSuspendGuard suspendAutoDetect() { return QGC::AutoSuspendGuard(_autoDetectSuspended); }

    /// Throttles detect misses; HMAC per packet per key is expensive. Monotonic timer to avoid wall-clock skew.
    bool isInDetectCooldown() const;
    void recordDetectMiss();
    void clearDetectCooldown();
    static constexpr qint64 kDetectCooldownMs = 2000;

    /// Single-lock snapshot; 3 separate reads have TOCTOU window vs MockLink's thread.
    MAVLinkSigning::DetectSnapshot detectSnapshot() const;

    /// True if last_status changed since previous call; sole transition-detection source.
    bool consumeStatusTransition(mavlink_channel_t channel);

private:
    friend class SigningController;

    /// Toggle SIGN_OUTGOING without resetting the key (pending-enable→confirmed flips it on). Returns false if not enabled.
    bool setSignOutgoing(bool signOutgoing);

    mavlink_signing_t _signing{};
    /// Per-link by design. ArduPilot/PX4 use a single global table; QGC must scope per link because USB+radio failover sees divergent timestamp histories per medium and a shared table causes OLD_TIMESTAMP rejections on the slower link.
    mavlink_signing_streams_t _streams{};
    QString _keyHint;
    bool _enabled = false;
    /// Hot path: lock-free read on every signed packet; co-observed reads use detectSnapshot.
    std::atomic<bool> _autoDetectSuspended{false};
    QDeadlineTimer _detectCooldown;  // default-constructed → expired (forever in the past)
    mavlink_signing_status_t _lastTransitionStatus = MAVLINK_SIGNING_STATUS_NONE;
    mutable QReadWriteLock _lock;
};
