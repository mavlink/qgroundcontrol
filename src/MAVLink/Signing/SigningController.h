#pragma once

#include <QtCore/QMutex>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QTimer>
#include <chrono>
#include <optional>

#include "EdgeTriggeredCounter.h"
#include "MAVLinkMessageType.h"
#include "MAVLinkSigning.h"
#include "SigningChannel.h"
#include "SigningFailure.h"
#include "SigningStatus.h"

/// \brief Owns MAVLink signing state and the deferred-confirmation state machine for one LinkInterface.
///
class SigningController : public QObject
{
    Q_OBJECT
public:
    using FailReason = SigningFailure::Reason;
    using State = SigningStatus::State;

    Q_PROPERTY(State state READ state NOTIFY stateChanged)

    explicit SigningController(mavlink_channel_t channel, QObject* parent = nullptr);
    ~SigningController() override;

    State state() const;
    SigningStatus status() const;

    bool isEnabled() const;
    QString keyName() const;
    QString statusText() const;

    /// Begin pending-enable. Caller must send SETUP_SIGNING only on nullopt; outcome arrives via signingConfirmed/signingFailed.
    [[nodiscard]] std::optional<SigningFailure> tryBeginEnable(uint8_t expectedSysId, const QString& keyName,
                                                               const MAVLinkSigning::SigningKey& keyBytes);

    /// Atomic check-and-commit for disable; same contract as tryBeginEnable.
    [[nodiscard]] std::optional<SigningFailure> tryBeginDisable(uint8_t expectedSysId);

    /// Abort any in-flight operation. Emits signingFailed if pending. No-op if Idle.
    /// `detail` overrides the default ("primary link changed before vehicle confirmation").
    void cancelPending(const QString& detail = {});

    bool clearSigning();

    /// Bypasses the FSM; used by tests and auto-detect. Non-empty keyNameHint seeds the persisted timestamp.
    bool initSigningImmediate(QByteArrayView key, MAVLinkSigning::UnsignedAcceptancePolicy policy,
                              const QString& keyNameHint = {});

    const SigningChannel& channel() const { return _channel; }

    /// Re-sign a cached outgoing message in place with a current, monotonic timestamp. No-op when signing is
    /// disabled or this isn't an outgoing-signed channel. Thread-safe (channel takes its own RW lock).
    bool signOutgoing(mavlink_message_t& message) { return _channel.signOutgoing(message); }

    void recordDetectMiss() { _channel.recordDetectMiss(); }

    void clearDetectCooldown() { _channel.clearDetectCooldown(); }

    /// Per-frame entry point; drives burst alerts, auto-detect, and the FSM. Returns true on auto-detect.
    bool processFrame(bool framingOk, const mavlink_message_t& message);

    void resetBadSigBurst();

    bool wallClockRefreshActiveForTesting() const { return _wallClockRefresh.isActive(); }

    /// Test-only override for the vehicle-confirmation timeout, read at each begin*(); zero restores the default.
    static void setTimeoutForTesting(std::chrono::milliseconds timeout) { _timeoutOverride = timeout; }

signals:
    void stateChanged();
    void keyAutoDetected(const QString& keyName);
    void alertRaised(const QString& detail);

    /// Emitted exactly once per begin*() on success. keyName is the enabled key, or empty for disable.
    void signingConfirmed(const QString& keyName);

    /// Emitted exactly once per begin*() on failure (timeout, init error, cancel, re-entry).
    void signingFailed(SigningFailure failure);

private:
    enum class OpKind : uint8_t
    {
        None,
        Enable,
        Disable
    };

    struct PendingOp
    {
        OpKind kind = OpKind::None;
        uint8_t expectedSysId = 0;
        QString keyName;
        MAVLinkSigning::SigningKey keyBytes{};
        bool unsignedSeen = false;
    };

    bool _isPendingLocked() const { return _op.kind != OpKind::None; }

    /// Caller holds _fsmMutex. Signal emission is marshalled via Qt::AutoConnection so any thread is safe.
    void _handleFsmFrameLocked(const mavlink_message_t& message);
    void _confirmLocked();
    void _failLocked(FailReason reason, const QString& detail, bool cancelled = false);
    void _clearLocked();
    void _setOpLocked(PendingOp next);
    void _completeDisableSuccessLocked();
    void _onTimeout();
    /// Gate the 1Hz timestamp refresh on whether the channel is actually signing; thread-safe (queued to main).
    void _setWallClockRefresh(bool on);

    SigningChannel _channel;
    mavlink_channel_t _mavlinkChannel;
    /// Guards _op/_badSigBurst/_autoDetectGuard against the link-RX thread vs main-thread race; recursive
    /// because Qt::AutoConnection slots fired from the same thread may call state() and re-enter.
    mutable QRecursiveMutex _fsmMutex;
    PendingOp _op;
    std::optional<QGC::AutoSuspendGuard> _autoDetectGuard;
    QTimer _timeout;
    /// 1Hz catch-up of `_signing.timestamp` to wall clock; libmavlink only bumps per-packet so idle
    /// outbound paths otherwise sign with a stale value and get rejected by peers that pin signing
    /// timestamps to wall clock (mavlink/qgroundcontrol#14375).
    QTimer _wallClockRefresh;

    static constexpr uint8_t kBadSignatureAlertThreshold = 3;
    QGC::EdgeTriggeredCounter<uint8_t> _badSigBurst{kBadSignatureAlertThreshold};

    static constexpr auto kTimeout = std::chrono::seconds(5);
    static std::chrono::milliseconds _effectiveTimeout()
    {
        return _timeoutOverride > std::chrono::milliseconds::zero() ? _timeoutOverride
                                                                    : std::chrono::milliseconds(kTimeout);
    }
    static inline std::chrono::milliseconds _timeoutOverride{0};
    /// Refresh interval well under MAVLINK_SIGNING_TIMESTAMP_LIMIT (6s default) on the receiver side.
    static constexpr auto kWallClockRefreshInterval = std::chrono::seconds(1);
};
