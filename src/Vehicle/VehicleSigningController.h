#pragma once

#include <QtCore/QByteArrayView>
#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QString>
#include <QtQmlIntegration/QtQmlIntegration>

#include "LinkInterface.h"
#include "SigningFailure.h"
#include "SigningStatus.h"

class SigningController;
class Vehicle;

/// Per-vehicle signing facade. Owns the wiring between Vehicle and the active SigningController
/// (which lives on the vehicle's primary LinkInterface). Re-binds when the primary link changes.
class VehicleSigningController : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")

public:
    Q_PROPERTY(SigningStatus signingStatus READ signingStatus NOTIFY signingStatusChanged)

    explicit VehicleSigningController(Vehicle* vehicle);
    ~VehicleSigningController() override = default;

    SigningStatus signingStatus() const;

    Q_INVOKABLE void enable(const QString& keyName);
    Q_INVOKABLE void disable();

    void onPrimaryLinkChanged();

signals:
    void signingStatusChanged();
    void signingFailed(SigningFailure info);

private slots:
    void _onSigningStateChanged();
    void _onSigningAlertRaised(const QString& detail);

private:
    void _connect(SigningController* ctrl);
    void _disconnect(SigningController* ctrl);
    bool _canBeginOp() const;
    bool _sendSetupSigning(const SharedLinkInterfacePtr& sharedLink, QByteArrayView keyView);
    void _wireOneShotResult();

    Vehicle* _vehicle;
    QPointer<SigningController> _active;
};
