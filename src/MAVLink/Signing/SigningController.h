#pragma once

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QTimer>
#include <chrono>

#include "MAVLinkLib.h"
#include "MAVLinkSigning.h"
#include "SigningChannel.h"
#include "SigningFailure.h"
#include "SigningStatus.h"

/// Owns MAVLink signing state and the deferred-confirmation state machine for one LinkInterface.
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
    int streamCount() const;
    QString statusText() const;

    /// Begin enable confirmation. On success: emits signingConfirmed(keyName).
    /// On failure (timeout / init error / re-entry): emits signingFailed.
    void beginEnable(uint8_t expectedSysId, const QString& keyName, const MAVLinkSigning::SigningKey& keyBytes);

    /// Begin disable confirmation. On success: emits signingConfirmed("").
    void beginDisable(uint8_t expectedSysId);

    /// Abort any in-flight operation. Emits signingFailed if pending. No-op if Idle.
    void cancelPending();

    bool clearSigning();

    /// Install signing immediately, bypassing the deferred-confirmation FSM. Used by tests and
    /// auto-detect. Non-empty keyNameHint seeds the channel with the persisted timestamp.
    bool initSigningImmediate(QByteArrayView key, mavlink_accept_unsigned_t callback, const QString& keyNameHint = {});

    MAVLinkSigning::DetectSnapshot detectSnapshot() const { return _channel.detectSnapshot(); }
    bool isAutoDetectSuspended() const { return _channel.isAutoDetectSuspended(); }
    bool isInDetectCooldown() const { return _channel.isInDetectCooldown(); }
    void recordDetectMiss() { _channel.recordDetectMiss(); }
    void clearDetectCooldown() { _channel.clearDetectCooldown(); }
    SigningChannel::TimestampSnapshot currentTimestampAndKey() const { return _channel.currentTimestampAndName(); }

    /// Per-frame entry point. `framingOk` distinguishes MAVLINK_FRAMING_OK from BAD_SIGNATURE.
    /// Drives status transitions, burst alerts, auto-detect, and the FSM. Returns true on auto-detect.
    bool processFrame(bool framingOk, const mavlink_message_t& message);

    void resetBadSigBurst();

signals:
    void stateChanged();
    void keyAutoDetected(const QString& keyName);
    void alertRaised(const QString& detail);

    /// Emitted exactly once per begin*() on success. keyName is the enabled key, or empty for disable.
    void signingConfirmed(const QString& keyName);

    /// Emitted exactly once per begin*() on failure (timeout, init error, cancel, re-entry).
    void signingFailed(SigningFailure failure);

private:
    enum class OpKind : uint8_t { None, Enable, Disable };

    /// keyBytes is meaningful only for Enable; unsignedSeen only for Disable.
    struct PendingOp
    {
        OpKind kind = OpKind::None;
        QString keyName;
        MAVLinkSigning::SigningKey keyBytes{};
        bool unsignedSeen = false;
    };

    bool _isPending() const { return _op.kind != OpKind::None; }
    void _emitFailedDeferred(FailReason reason, const QString& detail);
    void _handleFsmFrame(const mavlink_message_t& message);
    void _confirm();
    void _fail(FailReason reason, const QString& detail, bool cancelled = false);
    void _clear();
    void _setOp(PendingOp next);
    void _completeDisableSuccess();
    void _onTimeout();

    SigningChannel _channel;
    mavlink_channel_t _mavlinkChannel;
    uint8_t _expectedSysId = 0;
    PendingOp _op;
    QTimer _timeout;

    uint8_t _consecutiveBadSig = 0;
    bool _badSigAlertRaised = false;

    static constexpr auto kTimeout = std::chrono::seconds(5);
    static constexpr uint8_t kBadSignatureAlertThreshold = 3;
};
