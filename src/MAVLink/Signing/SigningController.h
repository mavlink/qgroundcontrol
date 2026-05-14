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
    int streamCount() const;
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

    void recordDetectMiss() { _channel.recordDetectMiss(); }

    void clearDetectCooldown() { _channel.clearDetectCooldown(); }

    /// Per-frame entry point; drives burst alerts, auto-detect, and the FSM. Returns true on auto-detect.
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

    SigningChannel _channel;
    mavlink_channel_t _mavlinkChannel;
    /// Guards _op/_badSigBurst/_autoDetectGuard against the link-RX thread vs main-thread race; recursive
    /// because Qt::AutoConnection slots fired from the same thread may call state() and re-enter.
    mutable QRecursiveMutex _fsmMutex;
    PendingOp _op;
    std::optional<QGC::AutoSuspendGuard> _autoDetectGuard;
    QTimer _timeout;

    static constexpr uint8_t kBadSignatureAlertThreshold = 3;
    QGC::EdgeTriggeredCounter<uint8_t> _badSigBurst{kBadSignatureAlertThreshold};

    static constexpr auto kTimeout = std::chrono::seconds(5);
};
