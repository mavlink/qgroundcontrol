#include "SigningChannel.h"

#include <algorithm>
#include <cstring>

#include "MAVLinkSigning.h"
#include "QGCLoggingCategory.h"
#include "SecureMemory.h"

QGC_LOGGING_CATEGORY(SigningChannelLog, "MAVLink.SigningChannel")

bool SigningChannel::init(mavlink_channel_t channel, QByteArrayView key, mavlink_accept_unsigned_t callback,
                          uint64_t persistedTimestamp, const QString& keyName)
{
    if (!key.isEmpty() && !callback) {
        qCWarning(SigningChannelLog) << "callback must be specified when enabling";
        return false;
    }

    mavlink_status_t* const status = mavlink_get_channel_status(channel);
    if (!status) {
        qCWarning(SigningChannelLog) << "Invalid channel:" << channel;
        return false;
    }

    QWriteLocker locker(&_lock);

    if (key.isEmpty()) {
        // Detach C library before wiping our struct so framing won't read freed/zeroed pointers mid-update.
        status->signing = nullptr;
        status->signing_streams = nullptr;
        QGC::secureZero(_signing.secret_key, sizeof(_signing.secret_key));
        _signing.accept_unsigned_callback = nullptr;
        memset(&_streams, 0, sizeof(_streams));
        _keyHint.clear();
        _enabled = false;
        return true;
    }

    if (key.size() < static_cast<qsizetype>(sizeof(_signing.secret_key))) {
        qCWarning(SigningChannelLog) << "Key too short:" << key.size();
        return false;
    }

    // Reset stream table — stale entries from a prior key/session would cause OLD_TIMESTAMP rejections.
    memset(&_streams, 0, sizeof(_streams));

    _signing.link_id = static_cast<uint8_t>(channel);
    _signing.flags = MAVLINK_SIGNING_FLAG_SIGN_OUTGOING;
    _signing.accept_unsigned_callback = callback;
    memcpy(_signing.secret_key, key.constData(), sizeof(_signing.secret_key));
    // Seed with max(wallClock, persisted+bump) so backward clock skew across restarts doesn't cause rejection.
    _signing.timestamp = std::max(MAVLinkSigning::currentSigningTimestampTicks(),
                                  persistedTimestamp + kPersistedTimestampSafetyBumpTicks);
    _keyHint = keyName;

    status->signing = &_signing;
    status->signing_streams = &_streams;
    _enabled = true;
    return true;
}

SigningChannel::TimestampSnapshot SigningChannel::currentTimestampAndName() const
{
    QReadLocker locker(&_lock);
    if (!_enabled) {
        return {0, QString()};
    }
    return {_signing.timestamp, _keyHint};
}

bool SigningChannel::setAcceptUnsignedCallback(mavlink_accept_unsigned_t callback)
{
    QWriteLocker locker(&_lock);
    if (!_enabled) {
        return false;
    }
    _signing.accept_unsigned_callback = callback;
    return true;
}

bool SigningChannel::isEnabled() const
{
    QReadLocker locker(&_lock);
    return _enabled;
}

int SigningChannel::streamCount() const
{
    QReadLocker locker(&_lock);
    return static_cast<int>(_streams.num_signing_streams);
}

QString SigningChannel::keyHint() const
{
    QReadLocker locker(&_lock);
    return _keyHint;
}

void SigningChannel::setKeyHint(const QString& name)
{
    QWriteLocker locker(&_lock);
    _keyHint = name;
}

bool SigningChannel::isAutoDetectSuspended() const
{
    return _autoDetectSuspended.load(std::memory_order_acquire);
}

void SigningChannel::setAutoDetectSuspended(bool suspended)
{
    _autoDetectSuspended.store(suspended, std::memory_order_release);
}

bool SigningChannel::isInDetectCooldown() const
{
    QReadLocker locker(&_lock);
    return !_detectCooldown.hasExpired();
}

void SigningChannel::recordDetectMiss()
{
    QWriteLocker locker(&_lock);
    _detectCooldown.setRemainingTime(kDetectCooldownMs);
}

void SigningChannel::clearDetectCooldown()
{
    QWriteLocker locker(&_lock);
    _detectCooldown.setRemainingTime(0);
}

void SigningChannel::clearKeyHint()
{
    QWriteLocker locker(&_lock);
    _keyHint.clear();
}

MAVLinkSigning::DetectSnapshot SigningChannel::detectSnapshot() const
{
    // _autoDetectSuspended atomic read outside lock; only keyHint/cooldown pair must be coherent.
    MAVLinkSigning::DetectSnapshot snap;
    snap.autoDetectSuspended = _autoDetectSuspended.load(std::memory_order_acquire);
    {
        QReadLocker locker(&_lock);
        snap.keyHint = _keyHint;
        snap.inCooldown = !_detectCooldown.hasExpired();
    }
    return snap;
}

bool SigningChannel::consumeStatusTransition(mavlink_channel_t channel)
{
    QWriteLocker locker(&_lock);
    const mavlink_status_t* const status = mavlink_get_channel_status(channel);
    const mavlink_signing_status_t current =
        (status && status->signing) ? status->signing->last_status : MAVLINK_SIGNING_STATUS_NONE;
    if (current == _lastTransitionStatus) {
        return false;
    }
    _lastTransitionStatus = current;
    return true;
}
