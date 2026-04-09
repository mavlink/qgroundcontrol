#include "VehicleSigningController.h"

#include <QtCore/QMetaEnum>

#include "MAVLinkProtocol.h"
#include "MAVLinkSigning.h"
#include "MAVLinkSigningKeys.h"
#include "QGCApplication.h"
#include "QGCLoggingCategory.h"
#include "SecureMemory.h"
#include "SigningController.h"
#include "Vehicle.h"
#include "VehicleLinkManager.h"

QGC_LOGGING_CATEGORY(VehicleSigningControllerLog, "Vehicle.SigningController")

VehicleSigningController::VehicleSigningController(Vehicle* vehicle) : QObject(vehicle), _vehicle(vehicle)
{
    qCDebug(VehicleSigningControllerLog) << "VehicleSigningController ctor — vehicle" << _vehicle->id();

    _retryTimer.setInterval(kRetransmitIntervalMs);
    connect(&_retryTimer, &QTimer::timeout, this, &VehicleSigningController::_onRetryTimer);

    connect(vehicle->vehicleLinkManager(), &VehicleLinkManager::primaryLinkChanged, this,
            &VehicleSigningController::_onPrimaryLinkChanged);

    if (const SharedLinkInterfacePtr sharedLink = vehicle->vehicleLinkManager()->primaryLink().lock()) {
        if (SigningController* ctrl = sharedLink->signing()) {
            _connect(ctrl);
        }
    }
}

VehicleSigningController::~VehicleSigningController()
{
    qCDebug(VehicleSigningControllerLog) << "VehicleSigningController dtor — vehicle" << _vehicle->id();
}

void VehicleSigningController::_onPrimaryLinkChanged()
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
    // Permanent listeners; signingConfirmed/signingFailed wired per-operation via Qt::SingleShotConnection in
    // enable/disable.
    connect(ctrl, &SigningController::stateChanged, this, &VehicleSigningController::_onSigningStateChanged);
    connect(ctrl, &SigningController::keyAutoDetected, this, &VehicleSigningController::_onSigningStateChanged);
    connect(ctrl, &SigningController::alertRaised, this, &VehicleSigningController::_onSigningAlertRaised);
    _refreshStatus();
}

void VehicleSigningController::_disconnect(SigningController* ctrl)
{
    if (!ctrl) {
        return;
    }
    if (ctrl->state() == SigningController::State::Enabling || ctrl->state() == SigningController::State::Disabling) {
        ctrl->cancelPending();
    }
    disconnect(ctrl, nullptr, this, nullptr);
    if (_active == ctrl) {
        _active = nullptr;
    }
    _refreshStatus();
}

void VehicleSigningController::_onSigningStateChanged()
{
    _refreshStatus();
}

void VehicleSigningController::_refreshStatus()
{
    _signingStatus = _active ? _active->status() : SigningStatus{};
}

void VehicleSigningController::_onSigningAlertRaised(const QString& detail)
{
    qCWarning(VehicleSigningControllerLog) << "[veh" << _vehicle->id() << "] signing alert:" << detail;
    qgcApp()->showAppMessage(tr("Vehicle %1: %2").arg(_vehicle->id()).arg(detail));
}

void VehicleSigningController::_onSigningConfirmed(const QString& confirmedKey)
{
    _stopRetransmit();
    if (auto link = _vehicle->vehicleLinkManager()->primaryLink().lock()) {
        MAVLinkProtocol::instance()->resetSequenceTracking(link.get());
    }
    qCDebug(VehicleSigningControllerLog) << "[veh" << _vehicle->id() << "] signing confirmed — key"
                                         << (confirmedKey.isEmpty() ? "<disabled>" : confirmedKey);
}

void VehicleSigningController::_onSigningFailed(const SigningFailure& failure)
{
    _stopRetransmit();
    qCWarning(VehicleSigningControllerLog)
        << "[veh" << _vehicle->id() << "] signing failed:"
        << QMetaEnum::fromType<SigningFailure::Reason>().valueToKey(static_cast<int>(failure.reason)) << failure.detail;
    emit signingFailed(failure);
    qgcApp()->showAppMessage(failure.detail);
}

void VehicleSigningController::_startRetransmit()
{
    _retryTimer.start();
}

void VehicleSigningController::_stopRetransmit()
{
    _retryTimer.stop();
    QGC::secureZero(_pendingKey);
    _pendingHasKey = false;
}

void VehicleSigningController::_onRetryTimer()
{
    if (!_active || _active->state() == SigningController::State::On ||
        _active->state() == SigningController::State::Off) {
        _stopRetransmit();
        return;
    }
    const SharedLinkInterfacePtr sharedLink = _vehicle->vehicleLinkManager()->primaryLink().lock();
    if (!sharedLink) {
        return;
    }
    const QByteArrayView keyView = _pendingHasKey ? QByteArrayView(reinterpret_cast<const char*>(_pendingKey.data()),
                                                                   static_cast<qsizetype>(_pendingKey.size()))
                                                  : QByteArrayView{};
    (void)_sendSetupSigning(sharedLink, keyView);
    qCDebug(VehicleSigningControllerLog) << "[veh" << _vehicle->id() << "] retransmitted SETUP_SIGNING ("
                                         << (_pendingHasKey ? "enable" : "disable") << ")";
}

bool VehicleSigningController::_sendSetupSigning(const SharedLinkInterfacePtr& sharedLink, QByteArrayView keyView)
{
    const auto channel = static_cast<mavlink_channel_t>(sharedLink->mavlinkChannel());
    const mavlink_system_t targetSystem{static_cast<uint8_t>(_vehicle->id()),
                                        static_cast<uint8_t>(_vehicle->defaultComponentId())};

    mavlink_message_t msg;
    if (!MAVLinkSigning::encodeSetupSigning(channel, MAVLinkProtocol::instance()->getSystemId(),
                                            MAVLinkProtocol::getComponentId(), targetSystem, keyView, msg)) {
        qCWarning(VehicleSigningControllerLog)
            << "[veh" << _vehicle->id() << "] failed to encode SETUP_SIGNING (channel" << channel << ")";
        return false;
    }

    // Send twice for lossy-link redundancy; enable goes unsigned (signing not yet installed), disable goes signed.
    for (uint8_t i = 0; i < 2; ++i) {
        _vehicle->sendMessageOnLinkThreadSafe(sharedLink.get(), msg);
    }
    return true;
}

void VehicleSigningController::enable(const QString& keyName)
{
    if (!_active) {
        qCWarning(VehicleSigningControllerLog) << "[veh" << _vehicle->id() << "] enable: no signing controller";
        return;
    }

    const SharedLinkInterfacePtr sharedLink = _vehicle->vehicleLinkManager()->primaryLink().lock();
    if (!sharedLink) {
        qCWarning(VehicleSigningControllerLog) << "[veh" << _vehicle->id() << "] enable: no primary link";
        return;
    }

    const auto keyBytes = MAVLinkSigningKeys::instance()->keyBytesByName(keyName);
    if (!keyBytes) {
        qCCritical(VehicleSigningControllerLog)
            << "[veh" << _vehicle->id() << "] enable: unknown signing key:" << keyName;
        return;
    }

    // Atomic FSM commit BEFORE _sendSetupSigning — re-entry rejected without putting bytes on the wire.
    if (auto fail = _active->tryBeginEnable(static_cast<uint8_t>(_vehicle->id()), keyName, *keyBytes)) {
        qgcApp()->showAppMessage(fail->detail);
        emit signingFailed(*fail);
        return;
    }

    connect(_active, &SigningController::signingConfirmed, this, &VehicleSigningController::_onSigningConfirmed,
            Qt::SingleShotConnection);
    connect(_active, &SigningController::signingFailed, this, &VehicleSigningController::_onSigningFailed,
            Qt::SingleShotConnection);

    const QByteArrayView keyView(reinterpret_cast<const char*>(keyBytes->data()), keyBytes->size());
    if (!_sendSetupSigning(sharedLink, keyView)) {
        _active->cancelPending(tr("Failed to transmit SETUP_SIGNING to vehicle"));
        return;
    }

    _pendingKey = *keyBytes;
    _pendingHasKey = true;
    _startRetransmit();

    qCDebug(VehicleSigningControllerLog) << "[veh" << _vehicle->id() << "] SETUP_SIGNING sent — key" << keyName
                                         << "awaiting signed HEARTBEAT";
}

void VehicleSigningController::disable()
{
    if (!_active) {
        qCWarning(VehicleSigningControllerLog) << "[veh" << _vehicle->id() << "] disable: no signing controller";
        return;
    }

    const SharedLinkInterfacePtr sharedLink = _vehicle->vehicleLinkManager()->primaryLink().lock();
    if (!sharedLink) {
        qCWarning(VehicleSigningControllerLog) << "[veh" << _vehicle->id() << "] disable: no primary link";
        return;
    }

    if (auto fail = _active->tryBeginDisable(static_cast<uint8_t>(_vehicle->id()))) {
        qgcApp()->showAppMessage(fail->detail);
        emit signingFailed(*fail);
        return;
    }

    connect(_active, &SigningController::signingConfirmed, this, &VehicleSigningController::_onSigningConfirmed,
            Qt::SingleShotConnection);
    connect(_active, &SigningController::signingFailed, this, &VehicleSigningController::_onSigningFailed,
            Qt::SingleShotConnection);

    // Wipe any leftover key bytes from a prior enable; disable retransmits send no key.
    QGC::secureZero(_pendingKey);
    _pendingHasKey = false;

    if (!_sendSetupSigning(sharedLink, QByteArrayView{})) {
        _active->cancelPending(tr("Failed to transmit SETUP_SIGNING to vehicle"));
        return;
    }

    _startRetransmit();

    qCDebug(VehicleSigningControllerLog) << "[veh" << _vehicle->id()
                                         << "] disable SETUP_SIGNING sent — awaiting unsigned HEARTBEAT";
}
