#pragma once

#include <QtCore/QByteArrayView>
#include <QtCore/QDeadlineTimer>
#include <QtCore/QReadWriteLock>
#include <QtCore/QString>
#include <atomic>
#include <cstdint>

#include "MAVLinkLib.h"
#include "MAVLinkSigning.h"

/// Owns MAVLink signing state for one channel: signing/streams structs, key hint, and RW lock.
class SigningChannel
{
public:
    SigningChannel() = default;
    ~SigningChannel() = default;
    SigningChannel(const SigningChannel&) = delete;
    SigningChannel& operator=(const SigningChannel&) = delete;
    SigningChannel(SigningChannel&&) = delete;
    SigningChannel& operator=(SigningChannel&&) = delete;

    static constexpr uint64_t kPersistedTimestampSafetyBumpTicks = 100'000;  // 1 second in 10µs ticks

    /// Initialize signing for `channel`. Empty key disables. Wipes any prior key material.
    /// When `keyName` is non-empty, seeds `_signing.timestamp` with max(wallClock, persistedTimestamp +
    /// safetyBump) and records `keyName` as the keyHint for later periodic timestamp flushes.
    bool init(mavlink_channel_t channel, QByteArrayView key, mavlink_accept_unsigned_t callback,
              uint64_t persistedTimestamp = 0, const QString& keyName = {});

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
    void setAutoDetectSuspended(bool suspended);

    /// Throttles detect misses: HMAC per packet per key is expensive. QDeadlineTimer (monotonic) avoids wall-clock skew desync.
    bool isInDetectCooldown() const;
    void recordDetectMiss();
    void clearDetectCooldown();
    static constexpr qint64 kDetectCooldownMs = 2000;

    /// Single-lock snapshot; 3 separate reads have TOCTOU window vs MockLink's thread.
    MAVLinkSigning::DetectSnapshot detectSnapshot() const;

    /// Returns true if the channel's last_status changed since the previous call. Single source of
    /// truth for signing-state transition detection.
    bool consumeStatusTransition(mavlink_channel_t channel);

private:
    mavlink_signing_t _signing{};
    mavlink_signing_streams_t _streams{};
    QString _keyHint;
    bool _enabled = false;
    /// Hot path: lock-free read on every signed packet; co-observed reads use detectSnapshot.
    std::atomic<bool> _autoDetectSuspended{false};
    QDeadlineTimer _detectCooldown;  // default-constructed → expired (forever in the past)
    mavlink_signing_status_t _lastTransitionStatus = MAVLINK_SIGNING_STATUS_NONE;
    mutable QReadWriteLock _lock;
};
