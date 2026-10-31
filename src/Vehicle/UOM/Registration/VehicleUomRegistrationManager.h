#pragma once

#include <memory>

#include <QtQmlIntegration/QtQmlIntegration>

#include "UomDeviceAdapter.h"
#include "UomRegistrationClient.h"
#include "UomRegistrationCredentials.h"
#include "UomRegistrationStore.h"
#include "UomRegistrationWorkflow.h"

class Vehicle;

class VehicleUomRegistrationManager final : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("UOM registration belongs to a vehicle")
    Q_PROPERTY(bool available READ available CONSTANT)
    Q_PROPERTY(bool supported READ supported NOTIFY changed)
    Q_PROPERTY(QString productModel READ productModel NOTIFY changed)
    Q_PROPERTY(QString serial READ serial NOTIFY changed)
    Q_PROPERTY(QString uid READ uid NOTIFY changed)
    Q_PROPERTY(QStringList devices READ devices NOTIFY changed)
    Q_PROPERTY(int selectedDevice READ selectedDevice NOTIFY changed)
    Q_PROPERTY(QString deviceStatus READ deviceStatus NOTIFY changed)
    Q_PROPERTY(QString environment READ environment NOTIFY changed)
    Q_PROPERTY(QString producerId READ producerId NOTIFY changed)
    Q_PROPERTY(QString expectedSerial READ expectedSerial NOTIFY changed)
    Q_PROPERTY(QString verifyUrl READ verifyUrl NOTIFY changed)
    Q_PROPERTY(QString reportUrl READ reportUrl NOTIFY changed)
    Q_PROPERTY(QString cancelUrl READ cancelUrl NOTIFY changed)
    Q_PROPERTY(bool configured READ configured NOTIFY changed)
    Q_PROPERTY(bool busy READ busy NOTIFY changed)
    Q_PROPERTY(bool pending READ pending NOTIFY changed)
    Q_PROPERTY(bool canConfigure READ canConfigure NOTIFY changed)
    Q_PROPERTY(bool canVerify READ canVerify NOTIFY changed)
    Q_PROPERTY(bool canActivate READ canActivate NOTIFY changed)
    Q_PROPERTY(bool canDeactivate READ canDeactivate NOTIFY changed)
    Q_PROPERTY(bool canRetry READ canRetry NOTIFY changed)
    Q_PROPERTY(QString registrationText READ registrationText NOTIFY changed)
    Q_PROPERTY(QString deviceActivationText READ deviceActivationText NOTIFY changed)
    Q_PROPERTY(QString reportText READ reportText NOTIFY changed)
    Q_PROPERTY(QString lastVerifiedAt READ lastVerifiedAt NOTIFY changed)
    Q_PROPERTY(QString lastReportedAt READ lastReportedAt NOTIFY changed)
    Q_PROPERTY(QString statusText READ statusText NOTIFY changed)
    Q_PROPERTY(QString credentialStatus READ credentialStatus NOTIFY changed)
    Q_PROPERTY(QString safetyStatus READ safetyStatus NOTIFY changed)
public:
    explicit VehicleUomRegistrationManager(Vehicle* vehicle);

    bool available() const { return _adapter != nullptr; }

    bool supported() const { return _adapter && _adapter->supported(); }

    QString productModel() const { return _adapter ? _adapter->productModel() : QString(); }

    QString serial() const { return _adapter ? _adapter->device().serial : QString(); }

    QString uid() const { return _adapter ? _adapter->device().uid : QString(); }

    QStringList devices() const { return _adapter ? _adapter->devices() : QStringList(); }

    int selectedDevice() const { return _adapter ? _adapter->selectedDevice() : -1; }

    QString deviceStatus() const;
    Q_INVOKABLE void refreshDevice();
    Q_INVOKABLE void selectDevice(int index);

    QString environment() const { return _workflow.config().environment; }

    QString producerId() const { return _workflow.config().producerId; }

    QString expectedSerial() const { return _workflow.config().expectedSerial; }

    QString verifyUrl() const { return _workflow.config().verifyUrl.toString(); }

    QString reportUrl() const { return _workflow.config().reportUrl.toString(); }

    QString cancelUrl() const { return _workflow.config().cancelUrl.toString(); }

    bool configured() const { return _workflow.configured(); }

    bool busy() const { return _workflow.busy() || (_adapter && _adapter->busy()); }

    bool pending() const { return _workflow.pending(); }

    bool canConfigure() const { return !busy(); }

    bool canVerify() const { return _workflow.canVerify(); }

    bool canActivate() const { return _workflow.canActivate(); }

    bool canDeactivate() const { return _workflow.canDeactivate(); }

    bool canRetry() const { return _workflow.canRetry(); }

    QString registrationText() const { return _workflow.registrationText(); }

    QString deviceActivationText() const { return _workflow.deviceActivationText(); }

    QString reportText() const { return _workflow.reportText(); }

    QString lastVerifiedAt() const { return _workflow.lastVerifiedAt(); }

    QString lastReportedAt() const { return _workflow.lastReportedAt(); }

    QString statusText() const;

    QString credentialStatus() const { return _workflow.credentialStatus(); }

    QString safetyStatus() const;

    Q_INVOKABLE QVariantMap configurationForEnvironment(const QString& environment) const;
    Q_INVOKABLE bool configure(const QString& environment, const QString& producerId, const QString& expectedSerial,
                               const QString& verifyUrl, const QString& reportUrl, const QString& cancelUrl,
                               const QString& keyId);

    Q_INVOKABLE void verifyRegistration() { _workflow.verifyRegistration(); }

    Q_INVOKABLE void activate() { _workflow.activate(); }

    Q_INVOKABLE void deactivateAndCancel(const QString& type, const QString& reason)
    {
        _workflow.deactivateAndCancel(type, reason);
    }

    Q_INVOKABLE void retryPending() { _workflow.retryPending(); }

signals:
    void changed();

private:
    void _syncDevice();
    void _loadConfiguration();
    Vehicle* _vehicle;
    UomDeviceAdapter* _adapter = nullptr;
    std::unique_ptr<UomRegistrationStore> _store;
    UomRegistrationCredentials _credentials;
    UomRegistrationClient _client;
    UomRegistrationWorkflow _workflow;
};
