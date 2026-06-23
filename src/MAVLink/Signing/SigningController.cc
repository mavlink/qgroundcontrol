#include "SigningController.h"

#include <QtCore/QMetaEnum>
#include <QtCore/QMetaObject>

#include "MAVLinkLib.h"
#include "MAVLinkSigning.h"
#include "MAVLinkSigningKeys.h"
#include "QGCLoggingCategory.h"
#include "SecureMemory.h"

QGC_LOGGING_CATEGORY(SigningControllerLog, "MAVLink.SigningController")

namespace {

const char* failReasonName(SigningController::FailReason r)
{
    return QMetaEnum::fromType<SigningFailure::Reason>().valueToKey(static_cast<int>(r));
}

}  // namespace

SigningController::SigningController(mavlink_channel_t channel, QObject* parent)
    : QObject(parent), _mavlinkChannel(channel)
{
    _timeout.setSingleShot(true);
    connect(&_timeout, &QTimer::timeout, this, &SigningController::_onTimeout);
    _wallClockRefresh.setInterval(kWallClockRefreshInterval);
    connect(&_wallClockRefresh, &QTimer::timeout, this, [this]() { _channel.refreshOutgoingTimestamp(); });
    qCDebug(SigningControllerLog) << "SigningController ctor — channel" << _mavlinkChannel;
}

SigningController::~SigningController()
{
    qCDebug(SigningControllerLog) << "SigningController dtor — channel" << _mavlinkChannel;
    _timeout.stop();
    _wallClockRefresh.stop();
    {
        QMutexLocker<QRecursiveMutex> locker(&_fsmMutex);
        _autoDetectGuard.reset();
        if (_op.kind == OpKind::Enable) {
            QGC::secureZero(_op.keyBytes);
        }
        _op = PendingOp{};
    }

    // Detach status->signing before _channel dies — otherwise the next parser call dangles.
    _channel.init(_mavlinkChannel, QByteArrayView(), nullptr);
}

void SigningController::_setOpLocked(PendingOp next)
{
    _op = std::move(next);
    QMetaObject::invokeMethod(this, [this]() { emit stateChanged(); }, Qt::AutoConnection);
}

void SigningController::_setWallClockRefresh(bool on)
{
    // QTimer start/stop must run on the timer's owning (main) thread; auto-detect reaches here on the link-RX thread.
    QMetaObject::invokeMethod(&_wallClockRefresh, on ? "start" : "stop", Qt::AutoConnection);
}

SigningController::State SigningController::state() const
{
    OpKind kind;
    {
        QMutexLocker<QRecursiveMutex> locker(&_fsmMutex);
        kind = _op.kind;
    }
    switch (kind) {
        case OpKind::Enable:
            return State::Enabling;
        case OpKind::Disable:
            return State::Disabling;
        case OpKind::None:
            break;
    }
    return _channel.isEnabled() ? State::On : State::Off;
}

SigningStatus SigningController::status() const
{
    SigningStatus s;
    s.state = state();
    // Enabling installs the channel struct but isn't surfaced as enabled; Disabling stays enabled until vehicle
    // confirms.
    s.enabled = (s.state == State::On) || (s.state == State::Disabling);
    s.keyName = _channel.keyHint();
    s.statusText = statusText();
    s.streamCount = _channel.streamCount();
    return s;
}

bool SigningController::isEnabled() const
{
    // Excludes Enabling: channel struct installed for verify but vehicle hasn't confirmed yet.
    const State s = state();
    return (s == State::On) || (s == State::Disabling);
}

QString SigningController::keyName() const
{
    return _channel.keyHint();
}

QString SigningController::statusText() const
{
    const State s = state();
    if (s == State::Enabling) {
        return tr("Configuring…");
    }
    if (s == State::Disabling) {
        return tr("Disabling…");
    }
    if (!_channel.isEnabled()) {
        return tr("Off");
    }
    // Last wire-status detail (empty when NONE); fall back to "On" for a healthy-but-idle link.
    const QString detail = MAVLinkSigning::signingStatusString(_mavlinkChannel);
    return detail.isEmpty() ? tr("On") : detail;
}

bool SigningController::clearSigning()
{
    if (_channel.isEnabled()) {
        const auto snap = _channel.currentTimestampAndName();
        if (!snap.keyName.isEmpty() && snap.timestamp > 0) {
            MAVLinkSigningKeys::instance()->recordTimestamp(snap.keyName, snap.timestamp);
        }
    }
    const bool ok = _channel.init(_mavlinkChannel, QByteArrayView(), nullptr);
    _channel.clearDetectCooldown();
    _setWallClockRefresh(false);
    return ok;
}

bool SigningController::initSigningImmediate(QByteArrayView key, MAVLinkSigning::UnsignedAcceptancePolicy policy,
                                             const QString& keyNameHint)
{
    const uint64_t persisted = keyNameHint.isEmpty() ? 0 : MAVLinkSigningKeys::instance()->lastTimestamp(keyNameHint);
    const bool ok = _channel.init(_mavlinkChannel, key, MAVLinkSigning::callbackForPolicy(policy), persisted, keyNameHint);
    _setWallClockRefresh(_channel.isEnabled());
    return ok;
}

std::optional<SigningFailure> SigningController::tryBeginEnable(uint8_t expectedSysId, const QString& kName,
                                                                const MAVLinkSigning::SigningKey& keyBytes)
{
    {
        QMutexLocker<QRecursiveMutex> locker(&_fsmMutex);
        if (_isPendingLocked()) {
            qCWarning(SigningControllerLog)
                << "[ch" << _mavlinkChannel << "] enable rejected: operation already pending";
            return SigningFailure{FailReason::VehicleUnreachable, tr("Signing operation already pending")};
        }

        // PendingEnableVerifyOnly: signOutgoing=false, lenient policy keeps non-responding vehicle observable while lib
        // verifies its signed reply.
        const QByteArrayView keyView(reinterpret_cast<const char*>(keyBytes.data()), keyBytes.size());
        const uint64_t persisted = kName.isEmpty() ? 0 : MAVLinkSigningKeys::instance()->lastTimestamp(kName);
        if (!_channel.init(_mavlinkChannel, keyView,
                           MAVLinkSigning::callbackForPolicy(MAVLinkSigning::UnsignedAcceptancePolicy::Pending),
                           persisted, kName, /*signOutgoing=*/false)) {
            qCWarning(SigningControllerLog) << "[ch" << _mavlinkChannel << "] enable rejected: signing init failed";
            return SigningFailure{FailReason::InitFailed, tr("Failed to install signing for pending verification")};
        }

        _autoDetectGuard.emplace(_channel.suspendAutoDetect());
        _setOpLocked(PendingOp{
            .kind = OpKind::Enable,
            .expectedSysId = expectedSysId,
            .keyName = kName,
            .keyBytes = keyBytes,
        });
    }
    // QTimer is owned by main thread; tryBeginEnable is called on the main thread.
    _timeout.setInterval(_effectiveTimeout());
    _timeout.start();
    _setWallClockRefresh(true);  // channel installed (signOutgoing flips true on confirm); keep timestamp fresh meanwhile
    qCDebug(SigningControllerLog) << "[ch" << _mavlinkChannel << "] enable pending — key" << kName << "sysid"
                                  << expectedSysId << "timeout" << kTimeout << "ms";
    return std::nullopt;
}

std::optional<SigningFailure> SigningController::tryBeginDisable(uint8_t expectedSysId)
{
    {
        QMutexLocker<QRecursiveMutex> locker(&_fsmMutex);
        if (_isPendingLocked()) {
            qCWarning(SigningControllerLog)
                << "[ch" << _mavlinkChannel << "] disable rejected: operation already pending";
            return SigningFailure{FailReason::VehicleUnreachable, tr("Signing operation already pending")};
        }

        if (!_channel.setAcceptUnsignedCallback(
                MAVLinkSigning::callbackForPolicy(MAVLinkSigning::UnsignedAcceptancePolicy::Pending))) {
            qCWarning(SigningControllerLog) << "[ch" << _mavlinkChannel << "] disable rejected: channel not signing";
            return SigningFailure{FailReason::VehicleUnreachable, tr("Channel not signing — cannot disable")};
        }

        _setOpLocked(PendingOp{
            .kind = OpKind::Disable,
            .expectedSysId = expectedSysId,
            .keyName = {},
            .keyBytes = {},
        });
    }
    _timeout.setInterval(_effectiveTimeout());
    _timeout.start();
    qCDebug(SigningControllerLog) << "[ch" << _mavlinkChannel << "] disable pending — sysid" << expectedSysId
                                  << "timeout" << kTimeout << "ms";
    return std::nullopt;
}

bool SigningController::processFrame(bool framingOk, const mavlink_message_t& message)
{
    if (_channel.consumeStatusTransition(_mavlinkChannel)) {
        QMetaObject::invokeMethod(this, [this]() { emit stateChanged(); }, Qt::AutoConnection);
    }

    if (!framingOk) {
        MAVLinkSigning::logSigningFailure(_mavlinkChannel);
        bool burstFired = false;
        uint8_t burstCount = 0;
        bool pendingEnable = false;
        {
            QMutexLocker<QRecursiveMutex> locker(&_fsmMutex);
            burstFired = _badSigBurst.record();
            burstCount = _badSigBurst.count();
            pendingEnable = (_op.kind == OpKind::Enable);
        }
        if (burstFired) {
            // Pending-enable burst: user is staring at "Configuring…"; tell them it's a key mismatch.
            const QString detail =
                pendingEnable
                    ? tr("MAVLink signing: %1 consecutive bad signatures while enabling — the chosen key likely "
                         "does not match the vehicle's stored key. Verify the key on the vehicle, then retry.")
                          .arg(burstCount)
                    : tr("MAVLink signing: %1 consecutive bad signatures on this link — wrong key or vehicle clock "
                         "drift")
                          .arg(burstCount);
            QMetaObject::invokeMethod(this, [this, detail]() { emit alertRaised(detail); }, Qt::AutoConnection);
        }
        return false;
    }

    {
        QMutexLocker<QRecursiveMutex> locker(&_fsmMutex);
        _badSigBurst.reset();
    }

    bool autoDetected = false;
    if (MAVLinkSigning::isMessageSigned(message) && !_channel.isEnabled()) {
        // Auto-detect bypasses the deferred-confirm FSM intentionally: tryDetectKey verifies the HMAC first, so the
        // vehicle is provably already signing with this key.
        const QString detected = MAVLinkSigningKeys::instance()->tryDetectKey(this, message);
        if (!detected.isEmpty()) {
            QMetaObject::invokeMethod(this, [this, detected]() { emit keyAutoDetected(detected); }, Qt::AutoConnection);
            autoDetected = true;
        }
    }

    {
        QMutexLocker<QRecursiveMutex> locker(&_fsmMutex);
        _handleFsmFrameLocked(message);
    }

    return autoDetected;
}

void SigningController::resetBadSigBurst()
{
    QMutexLocker<QRecursiveMutex> locker(&_fsmMutex);
    _badSigBurst.reset();
}

void SigningController::_handleFsmFrameLocked(const mavlink_message_t& message)
{
    if (!_isPendingLocked()) {
        return;
    }
    if (message.sysid != _op.expectedSysId) {
        return;
    }

    // ERROR-severity STATUSTEXT mentioning "signing" → fast-fail (e.g. ArduPilot armed-rejection) before 5s timeout.
    if (message.msgid == MAVLINK_MSG_ID_STATUSTEXT) {
        mavlink_statustext_t st;
        mavlink_msg_statustext_decode(&message, &st);
        const QString text = QString::fromLatin1(st.text, qstrnlen(st.text, sizeof(st.text)));
        const bool errorSeverity = (st.severity <= MAV_SEVERITY_ERROR);
        const bool mentionsSigning = text.contains(QLatin1String("signing"), Qt::CaseInsensitive);
        if (errorSeverity && mentionsSigning) {
            _failLocked(FailReason::InitFailed, tr("Vehicle rejected signing change: %1").arg(text));
            return;
        }
    }

    if (_op.kind == OpKind::Enable) {
        if (message.msgid == MAVLINK_MSG_ID_HEARTBEAT && MAVLinkSigning::isMessageSigned(message)) {
            // Defence-in-depth: re-verify against committed key so a future framing-error refactor can't confirm on the
            // wrong key.
            if (!MAVLinkSigning::verifySignature(_op.keyBytes, message)) {
                qCWarning(SigningControllerLog)
                    << "[ch" << _mavlinkChannel
                    << "] pending-enable HEARTBEAT signature did not verify against committed key";
                return;
            }
            _confirmLocked();
        }
    } else if (_op.kind == OpKind::Disable) {
        if (!MAVLinkSigning::isMessageSigned(message) && message.msgid == MAVLINK_MSG_ID_HEARTBEAT) {
            _op.unsignedSeen = true;
            _confirmLocked();
        }
    }
}

void SigningController::_confirmLocked()
{
    // QTimer::stop is not thread-safe; route through queued invoke since processFrame may run on link thread.
    QMetaObject::invokeMethod(&_timeout, "stop", Qt::AutoConnection);

    if (_op.kind == OpKind::Enable) {
        const QString confirmedName = _op.keyName;
        if (!_channel.setSignOutgoing(true) || !_channel.setAcceptUnsignedCallback(MAVLinkSigning::callbackForPolicy(
                                                   MAVLinkSigning::UnsignedAcceptancePolicy::Strict))) {
            const QString detail = tr("Signing confirmation received but local activation failed");
            qCCritical(SigningControllerLog) << "[ch" << _mavlinkChannel << "]" << detail;
            _failLocked(FailReason::InitFailed, detail);
            return;
        }
        qCDebug(SigningControllerLog) << "[ch" << _mavlinkChannel << "] enable confirmed — key" << confirmedName;
        _clearLocked();
        QMetaObject::invokeMethod(
            this, [this, confirmedName]() { emit signingConfirmed(confirmedName); }, Qt::AutoConnection);
    } else if (_op.kind == OpKind::Disable) {
        qCDebug(SigningControllerLog) << "[ch" << _mavlinkChannel << "] disable confirmed";
        _completeDisableSuccessLocked();
    }
}

void SigningController::_completeDisableSuccessLocked()
{
    _channel.init(_mavlinkChannel, QByteArrayView(), nullptr);
    _clearLocked();
    QMetaObject::invokeMethod(this, [this]() { emit signingConfirmed(QString{}); }, Qt::AutoConnection);
}

void SigningController::_failLocked(FailReason reason, const QString& detail, bool cancelled)
{
    QMetaObject::invokeMethod(&_timeout, "stop", Qt::AutoConnection);

    // Disable: if vehicle already sent an unsigned heartbeat before timeout/cancel, treat as success.
    if (_op.kind == OpKind::Disable && _op.unsignedSeen && !cancelled) {
        _completeDisableSuccessLocked();
        return;
    }

    FailReason effectiveReason = reason;
    QString effectiveDetail = detail;

    if (_op.kind == OpKind::Enable) {
        _channel.init(_mavlinkChannel, QByteArrayView(), nullptr);
    } else if (_op.kind == OpKind::Disable) {
        _channel.setAcceptUnsignedCallback(
            MAVLinkSigning::callbackForPolicy(MAVLinkSigning::UnsignedAcceptancePolicy::Strict));
        effectiveReason = FailReason::VehicleUnreachable;
        effectiveDetail =
            tr("Signing disable not confirmed — vehicle is unreachable or still requires signed messages. Local "
               "signing remains enabled.");
    }

    qCWarning(SigningControllerLog) << "[ch" << _mavlinkChannel
                                    << "] signing operation failed:" << failReasonName(effectiveReason)
                                    << effectiveDetail;

    SigningFailure failure{effectiveReason, effectiveDetail};
    _clearLocked();
    QMetaObject::invokeMethod(this, [this, failure]() { emit signingFailed(failure); }, Qt::AutoConnection);
}

void SigningController::cancelPending(const QString& detail)
{
    QMutexLocker<QRecursiveMutex> locker(&_fsmMutex);
    if (!_isPendingLocked()) {
        return;
    }
    const QString effectiveDetail =
        detail.isEmpty()
            ? tr("Signing operation cancelled — primary link changed before vehicle confirmation")
            : detail;
    _failLocked(FailReason::VehicleUnreachable, effectiveDetail, /*cancelled=*/true);
}

void SigningController::_onTimeout()
{
    QMutexLocker<QRecursiveMutex> locker(&_fsmMutex);
    if (!_isPendingLocked()) {
        return;
    }
    qCDebug(SigningControllerLog) << "[ch" << _mavlinkChannel << "] timeout fired —"
                                  << (_op.kind == OpKind::Enable ? "enable" : "disable") << "not confirmed";
    const QString detail = (_op.kind == OpKind::Enable) ? tr("Signing setup not confirmed by vehicle (timeout)")
                                                        : tr("Signing disable not confirmed by vehicle (timeout)");
    _failLocked(FailReason::Timeout, detail);
}

void SigningController::_clearLocked()
{
    _autoDetectGuard.reset();
    QGC::secureZero(_op.keyBytes);
    _setOpLocked(PendingOp{});
    // Common FSM exit: enable-confirm leaves the channel signing (keep), disable/abort disables it (stop).
    _setWallClockRefresh(_channel.isEnabled());
}
