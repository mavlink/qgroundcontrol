#include "SigningController.h"

#include <QtCore/QMetaEnum>

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

}

SigningController::SigningController(mavlink_channel_t channel, QObject* parent)
    : QObject(parent)
    , _mavlinkChannel(channel)
{
    _timeout.setSingleShot(true);
    _timeout.setInterval(kTimeout);
    connect(&_timeout, &QTimer::timeout, this, &SigningController::_onTimeout);
}

SigningController::~SigningController()
{
    _timeout.stop();

    if (_op.kind == OpKind::Enable) {
        _channel.setAutoDetectSuspended(false);
        QGC::secureZero(_op.keyBytes);
    } else if (_op.kind == OpKind::Disable) {
        _channel.setAcceptUnsignedCallback(MAVLinkSigning::secureConnectionAcceptUnsignedCallback);
    }
}

void SigningController::_setOp(PendingOp next)
{
    _op = std::move(next);
    emit stateChanged();
}

SigningController::State SigningController::state() const
{
    switch (_op.kind) {
        case OpKind::Enable:  return State::Enabling;
        case OpKind::Disable: return State::Disabling;
        case OpKind::None:    break;
    }
    return _channel.isEnabled() ? State::On : State::Off;
}

SigningStatus SigningController::status() const
{
    SigningStatus s;
    s.state = state();
    s.enabled = _channel.isEnabled();
    s.keyName = _channel.keyHint();
    s.statusText = statusText();
    s.streamCount = _channel.streamCount();
    return s;
}

bool SigningController::isEnabled() const
{
    return _channel.isEnabled();
}

QString SigningController::keyName() const
{
    return _channel.keyHint();
}

int SigningController::streamCount() const
{
    return _channel.streamCount();
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
    const mavlink_status_t* const status = mavlink_get_channel_status(_mavlinkChannel);
    const mavlink_signing_status_t lastStatus = (status && status->signing) ? status->signing->last_status : MAVLINK_SIGNING_STATUS_NONE;
    switch (lastStatus) {
        case MAVLINK_SIGNING_STATUS_OK:
            return QStringLiteral("OK");
        case MAVLINK_SIGNING_STATUS_BAD_SIGNATURE:
            return QStringLiteral("Bad Signature");
        case MAVLINK_SIGNING_STATUS_NO_STREAMS:
            return QStringLiteral("No Streams");
        case MAVLINK_SIGNING_STATUS_TOO_MANY_STREAMS:
            return QStringLiteral("Too Many Streams");
        case MAVLINK_SIGNING_STATUS_OLD_TIMESTAMP:
            return QStringLiteral("Stale Timestamp");
        case MAVLINK_SIGNING_STATUS_REPLAY:
            return QStringLiteral("Replay Detected");
        case MAVLINK_SIGNING_STATUS_NONE:
        default:
            return tr("On");
    }
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
    return ok;
}

bool SigningController::initSigningImmediate(QByteArrayView key, mavlink_accept_unsigned_t callback,
                                              const QString& keyNameHint)
{
    const uint64_t persisted =
        keyNameHint.isEmpty() ? 0 : MAVLinkSigningKeys::instance()->lastTimestamp(keyNameHint);
    return _channel.init(_mavlinkChannel, key, callback, persisted, keyNameHint);
}

void SigningController::_emitFailedDeferred(FailReason reason, const QString& detail)
{
    // Defer so callers can wire one-shot opCtx after begin*() returns and still receive the failure.
    QMetaObject::invokeMethod(this, [this, reason, detail] {
        emit signingFailed({reason, detail});
    }, Qt::QueuedConnection);
}

void SigningController::beginEnable(uint8_t expectedSysId, const QString& kName,
                                    const MAVLinkSigning::SigningKey& keyBytes)
{
    if (_isPending()) {
        qCWarning(SigningControllerLog) << "Begin enable rejected: operation already pending";
        _emitFailedDeferred(FailReason::VehicleUnreachable, tr("Signing operation already pending"));
        return;
    }

    PendingOp next;
    next.kind = OpKind::Enable;
    next.keyName = kName;
    next.keyBytes = keyBytes;

    _expectedSysId = expectedSysId;
    _channel.setAutoDetectSuspended(true);
    _setOp(std::move(next));
    _timeout.start();
}

void SigningController::beginDisable(uint8_t expectedSysId)
{
    if (_isPending()) {
        qCWarning(SigningControllerLog) << "Begin disable rejected: operation already pending";
        _emitFailedDeferred(FailReason::VehicleUnreachable, tr("Signing operation already pending"));
        return;
    }

    if (!_channel.setAcceptUnsignedCallback(MAVLinkSigning::pendingDisableAcceptUnsignedCallback)) {
        qCWarning(SigningControllerLog) << "Begin disable rejected: channel not signing";
        _emitFailedDeferred(FailReason::VehicleUnreachable, tr("Channel not signing — cannot disable"));
        return;
    }

    PendingOp next;
    next.kind = OpKind::Disable;

    _expectedSysId = expectedSysId;
    _setOp(std::move(next));
    _timeout.start();
}

bool SigningController::processFrame(bool framingOk, const mavlink_message_t& message)
{
    if (_channel.consumeStatusTransition(_mavlinkChannel)) {
        emit stateChanged();
    }

    if (!framingOk) {
        MAVLinkSigning::logSigningFailure(_mavlinkChannel);
        if (++_consecutiveBadSig >= kBadSignatureAlertThreshold && !_badSigAlertRaised) {
            _badSigAlertRaised = true;
            emit alertRaised(
                tr("MAVLink signing: %1 consecutive bad signatures on this link — wrong key or vehicle clock drift")
                    .arg(_consecutiveBadSig));
        }
        return false;
    }

    if (_consecutiveBadSig != 0 || _badSigAlertRaised) {
        _consecutiveBadSig = 0;
        _badSigAlertRaised = false;
    }

    bool autoDetected = false;
    if (MAVLinkSigning::isMessageSigned(message) && !_channel.isEnabled()) {
        const QString detected = MAVLinkSigningKeys::instance()->tryDetectKey(this, message);
        if (!detected.isEmpty()) {
            emit keyAutoDetected(detected);
            autoDetected = true;
        }
    }

    _handleFsmFrame(message);
    return autoDetected;
}

void SigningController::resetBadSigBurst()
{
    _consecutiveBadSig = 0;
    _badSigAlertRaised = false;
}

void SigningController::_handleFsmFrame(const mavlink_message_t& message)
{
    if (!_isPending()) {
        return;
    }
    if (message.sysid != _expectedSysId) {
        return;
    }

    if (_op.kind == OpKind::Enable) {
        if (message.msgid == MAVLINK_MSG_ID_HEARTBEAT && MAVLinkSigning::isMessageSigned(message) &&
            MAVLinkSigning::verifySignature(_op.keyBytes, message)) {
            _confirm();
        }
    } else if (_op.kind == OpKind::Disable) {
        if (!MAVLinkSigning::isMessageSigned(message) && message.msgid == MAVLINK_MSG_ID_HEARTBEAT) {
            _op.unsignedSeen = true;
            _confirm();
        }
    }
}

void SigningController::_confirm()
{
    _timeout.stop();

    if (_op.kind == OpKind::Enable) {
        const QByteArrayView keyView(reinterpret_cast<const char*>(_op.keyBytes.data()), _op.keyBytes.size());
        const QString confirmedName = _op.keyName;
        if (!initSigningImmediate(keyView, MAVLinkSigning::secureConnectionAcceptUnsignedCallback, _op.keyName)) {
            const QString detail = tr("Signing confirmation received but local initialization failed");
            qCCritical(SigningControllerLog) << detail << "channel" << _mavlinkChannel;
            _fail(FailReason::InitFailed, detail);
            return;
        }
        _clear();
        emit signingConfirmed(confirmedName);
    } else if (_op.kind == OpKind::Disable) {
        _completeDisableSuccess();
    }
}

void SigningController::_completeDisableSuccess()
{
    _channel.init(_mavlinkChannel, QByteArrayView(), nullptr);
    _clear();
    emit signingConfirmed(QString{});
}

void SigningController::_fail(FailReason reason, const QString& detail, bool cancelled)
{
    _timeout.stop();

    // Disable: if vehicle already sent an unsigned heartbeat before timeout/cancel, treat as success.
    if (_op.kind == OpKind::Disable && _op.unsignedSeen && !cancelled) {
        _completeDisableSuccess();
        return;
    }

    FailReason effectiveReason = reason;
    QString effectiveDetail = detail;

    if (_op.kind == OpKind::Disable) {
        _channel.setAcceptUnsignedCallback(MAVLinkSigning::secureConnectionAcceptUnsignedCallback);
        effectiveReason = FailReason::VehicleUnreachable;
        effectiveDetail =
            tr("Signing disable not confirmed — vehicle is unreachable or still requires signed messages. Local "
               "signing remains enabled.");
    }

    qCWarning(SigningControllerLog)
        << "Signing operation failed:" << failReasonName(effectiveReason) << effectiveDetail;

    SigningFailure failure{effectiveReason, effectiveDetail};
    _clear();
    emit signingFailed(failure);
}

void SigningController::cancelPending()
{
    if (!_isPending()) {
        return;
    }
    _fail(FailReason::VehicleUnreachable,
          tr("Signing operation cancelled — primary link changed before vehicle confirmation"),
          /*cancelled=*/true);
}

void SigningController::_onTimeout()
{
    const QString detail = (_op.kind == OpKind::Enable)
                               ? tr("Signing setup not confirmed by vehicle (timeout)")
                               : tr("Signing disable not confirmed by vehicle (timeout)");
    _fail(FailReason::Timeout, detail);
}

void SigningController::_clear()
{
    _channel.setAutoDetectSuspended(false);
    QGC::secureZero(_op.keyBytes);
    _setOp(PendingOp{});
    _expectedSysId = 0;
}
