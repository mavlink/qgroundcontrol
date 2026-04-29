#include "VehicleSigningController.h"

#include <QtCore/QMetaEnum>

#include "MAVLinkProtocol.h"
#include "MAVLinkSigning.h"
#include "MAVLinkSigningKeys.h"
#include "QGCApplication.h"
#include "QGCLoggingCategory.h"
#include "SigningController.h"
#include "Vehicle.h"
#include "VehicleLinkManager.h"

QGC_LOGGING_CATEGORY(VehicleSigningControllerLog, "Vehicle.SigningController")

VehicleSigningController::VehicleSigningController(Vehicle* vehicle)
    : QObject(vehicle)
    , _vehicle(vehicle)
{
    if (const SharedLinkInterfacePtr sharedLink = vehicle->vehicleLinkManager()->primaryLink().lock()) {
        if (SigningController* ctrl = sharedLink->signing()) {
            _connect(ctrl);
        }
    }
}

SigningStatus VehicleSigningController::signingStatus() const
{
    return _active ? _active->status() : SigningStatus{};
}

void VehicleSigningController::onPrimaryLinkChanged()
{
    _disconnect(_active);

    if (const SharedLinkInterfacePtr sharedLink = _vehicle->vehicleLinkManager()->primaryLink().lock()) {
        if (SigningController* ctrl = sharedLink->signing()) {
            _connect(ctrl);
        }
    }
}

void VehicleSigningController::_connect(SigningController* ctrl)
{
    if (!ctrl || ctrl == _active) {
        return;
    }
    _active = ctrl;
    connect(ctrl, &SigningController::stateChanged, this, &VehicleSigningController::_onSigningStateChanged);
    connect(ctrl, &SigningController::keyAutoDetected, this, &VehicleSigningController::_onSigningStateChanged);
    connect(ctrl, &SigningController::alertRaised, this, &VehicleSigningController::_onSigningAlertRaised);
    emit signingStatusChanged();
}

void VehicleSigningController::_disconnect(SigningController* ctrl)
{
    if (!ctrl) {
        return;
    }
    if (ctrl->state() == SigningController::State::Enabling ||
        ctrl->state() == SigningController::State::Disabling) {
        ctrl->cancelPending();
    }
    disconnect(ctrl, &SigningController::stateChanged, this, &VehicleSigningController::_onSigningStateChanged);
    disconnect(ctrl, &SigningController::keyAutoDetected, this, &VehicleSigningController::_onSigningStateChanged);
    disconnect(ctrl, &SigningController::alertRaised, this, &VehicleSigningController::_onSigningAlertRaised);
    if (_active == ctrl) {
        _active = nullptr;
    }
    emit signingStatusChanged();
}

void VehicleSigningController::_onSigningStateChanged()
{
    emit signingStatusChanged();
}

void VehicleSigningController::_onSigningAlertRaised(const QString& detail)
{
    qCWarning(VehicleSigningControllerLog) << "Signing alert:" << detail;
    qgcApp()->showAppMessage(tr("Vehicle %1: %2").arg(_vehicle->id()).arg(detail));
}

bool VehicleSigningController::_canBeginOp() const
{
    if (!_active) {
        qCWarning(VehicleSigningControllerLog) << "No signing controller";
        return false;
    }
    if (_active->state() == SigningController::State::Enabling ||
        _active->state() == SigningController::State::Disabling) {
        qCWarning(VehicleSigningControllerLog) << "Signing operation already in progress";
        return false;
    }
    return true;
}

bool VehicleSigningController::_sendSetupSigning(const SharedLinkInterfacePtr& sharedLink, QByteArrayView keyView)
{
    const auto channel = static_cast<mavlink_channel_t>(sharedLink->mavlinkChannel());
    const mavlink_system_t targetSystem{static_cast<uint8_t>(_vehicle->id()),
                                         static_cast<uint8_t>(_vehicle->defaultComponentId())};

    mavlink_message_t msg;
    if (!MAVLinkSigning::encodeSetupSigning(channel, MAVLinkProtocol::instance()->getSystemId(),
                                            MAVLinkProtocol::getComponentId(), targetSystem, keyView, msg)) {
        qCWarning(VehicleSigningControllerLog) << "Failed to encode SETUP_SIGNING";
        return false;
    }

    // Send unsigned twice — local signing deferred until vehicle confirms.
    for (uint8_t i = 0; i < 2; ++i) {
        _vehicle->sendMessageOnLinkThreadSafe(sharedLink.get(), msg);
    }
    return true;
}

void VehicleSigningController::_wireOneShotResult()
{
    auto* opCtx = new QObject(this);
    connect(_active, &SigningController::signingConfirmed, opCtx,
            [this, opCtx](const QString& confirmedKey) {
                if (auto link = _vehicle->vehicleLinkManager()->primaryLink().lock()) {
                    MAVLinkProtocol::instance()->resetSequenceTracking(link.get());
                }
                qCDebug(VehicleSigningControllerLog) << "Signing confirmed, key:" << confirmedKey;
                opCtx->deleteLater();
            });
    connect(_active, &SigningController::signingFailed, opCtx,
            [this, opCtx](const SigningFailure& f) {
                qCWarning(VehicleSigningControllerLog)
                    << "Signing failed:"
                    << QMetaEnum::fromType<SigningFailure::Reason>().valueToKey(static_cast<int>(f.reason))
                    << f.detail;
                emit signingFailed(f);
                qgcApp()->showAppMessage(f.detail);
                opCtx->deleteLater();
            });
}

void VehicleSigningController::enable(const QString& keyName)
{
    if (!_canBeginOp()) {
        return;
    }

    const SharedLinkInterfacePtr sharedLink = _vehicle->vehicleLinkManager()->primaryLink().lock();
    if (!sharedLink) {
        qCWarning(VehicleSigningControllerLog) << "No primary link";
        return;
    }

    const auto keyBytes = MAVLinkSigningKeys::instance()->keyBytesByName(keyName);
    if (!keyBytes) {
        qCCritical(VehicleSigningControllerLog) << "Unknown signing key:" << keyName;
        return;
    }

    const QByteArrayView keyView(reinterpret_cast<const char*>(keyBytes->data()), keyBytes->size());
    if (!_sendSetupSigning(sharedLink, keyView)) {
        return;
    }

    _wireOneShotResult();
    _active->beginEnable(static_cast<uint8_t>(_vehicle->id()), keyName, *keyBytes);
    qCDebug(VehicleSigningControllerLog) << "SETUP_SIGNING sent, awaiting signed response";
}

void VehicleSigningController::disable()
{
    if (!_canBeginOp()) {
        return;
    }

    const SharedLinkInterfacePtr sharedLink = _vehicle->vehicleLinkManager()->primaryLink().lock();
    if (!sharedLink) {
        qCWarning(VehicleSigningControllerLog) << "No primary link";
        return;
    }

    if (!_active->isEnabled()) {
        qCWarning(VehicleSigningControllerLog) << "Channel not signing — cannot disable";
        qgcApp()->showAppMessage(tr("Signing disable failed: channel is not signed"));
        return;
    }

    if (!_sendSetupSigning(sharedLink, QByteArrayView{})) {
        return;
    }

    _wireOneShotResult();
    _active->beginDisable(static_cast<uint8_t>(_vehicle->id()));
    qCDebug(VehicleSigningControllerLog) << "Disable signing sent, awaiting unsigned heartbeat";
}
