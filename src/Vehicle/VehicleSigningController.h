#pragma once

#include <QtCore/QBindable>
#include <QtCore/QByteArrayView>
#include <QtCore/QObject>
#include <QtCore/QObjectBindableProperty>
#include <QtCore/QPointer>
#include <QtCore/QString>
#include <QtCore/QTimer>
#include <QtQmlIntegration/QtQmlIntegration>

#include "LinkInterface.h"
#include "MAVLinkSigning.h"
#include "SigningFailure.h"
#include "SigningStatus.h"

class SigningController;
class Vehicle;

/// \brief Per-vehicle signing facade. Owns the wiring between Vehicle and the active SigningController
/// (which lives on the vehicle's primary LinkInterface). Re-binds when the primary link changes.

class VehicleSigningController : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")

public:
    Q_PROPERTY(
        SigningStatus signingStatus READ signingStatus BINDABLE bindableSigningStatus NOTIFY signingStatusChanged)

    explicit VehicleSigningController(Vehicle* vehicle);
    ~VehicleSigningController() override;

    SigningStatus signingStatus() const { return _signingStatus.value(); }

    QBindable<SigningStatus> bindableSigningStatus() { return &_signingStatus; }

    Q_INVOKABLE void enable(const QString& keyName);
    Q_INVOKABLE void disable();

signals:
    void signingStatusChanged();
    void signingFailed(SigningFailure info);

private slots:
    void _onPrimaryLinkChanged();
    void _onSigningStateChanged();
    void _onSigningAlertRaised(const QString& detail);
    void _onSigningConfirmed(const QString& confirmedKey);
    void _onSigningFailed(const SigningFailure& failure);
    void _onRetryTimer();

private:
    void _connect(SigningController* ctrl);
    void _disconnect(SigningController* ctrl);
    /// Active controller's primary link, or null (logging `op` context) when no controller/link is available.
    SharedLinkInterfacePtr _activeLink(const char* op) const;
    /// Wire the one-shot confirm/fail handlers for one pending op. Called once per enable()/disable() — never from the
    /// retransmit path, which would double-deliver the SingleShotConnection.
    void _wireConfirmHandlers();
    /// Transmit SETUP_SIGNING and arm retransmit. Cancels the pending op and returns false if transmit fails.
    bool _sendAndStartRetransmit(const SharedLinkInterfacePtr& sharedLink, QByteArrayView keyView);
    bool _sendSetupSigning(const SharedLinkInterfacePtr& sharedLink, QByteArrayView keyView);
    void _refreshStatus();
    void _startRetransmit();
    void _stopRetransmit();

    static constexpr int kRetransmitIntervalMs = 1500;

    Vehicle* _vehicle;
    QPointer<SigningController> _active;
    QTimer _retryTimer;
    MAVLinkSigning::SigningKey _pendingKey{};
    bool _pendingHasKey = false;
    Q_OBJECT_BINDABLE_PROPERTY(VehicleSigningController, SigningStatus, _signingStatus,
                               &VehicleSigningController::signingStatusChanged)
};
