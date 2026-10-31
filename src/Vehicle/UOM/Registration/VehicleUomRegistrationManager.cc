#include "VehicleUomRegistrationManager.h"

#include <QtCore/QDir>
#include <QtCore/QSettings>
#include <QtCore/QStandardPaths>

#include "FirmwarePlugin.h"
#include "MultiVehicleManager.h"
#include "Vehicle.h"
#include "VehicleLinkManager.h"

namespace {
QString uomRegistrationDatabasePath()
{
    return QDir(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation))
        .filePath(QStringLiteral("uom-registration.sqlite"));
}
}  // namespace

VehicleUomRegistrationManager::VehicleUomRegistrationManager(Vehicle* vehicle)
    : QObject(vehicle)
    , _vehicle(vehicle)
    , _store(std::make_unique<UomRegistrationStore>(uomRegistrationDatabasePath()))
    , _credentials(this)
    , _client(_store.get(), this)
    , _workflow(_store.get(), &_client, &_credentials, this)
{
    connect(&_workflow, &UomRegistrationWorkflow::changed, this, &VehicleUomRegistrationManager::changed);
    if (!_vehicle) {
        return;
    }
    _adapter = _vehicle->firmwarePlugin()->createUomDeviceAdapter(_vehicle, this);
    if (_adapter) {
        connect(_adapter, &UomDeviceAdapter::changed, this, &VehicleUomRegistrationManager::_syncDevice);
        connect(_adapter, &UomDeviceAdapter::activationRead, &_workflow, &UomRegistrationWorkflow::deviceRead);
        connect(_adapter, &UomDeviceAdapter::activationWritten, &_workflow, &UomRegistrationWorkflow::deviceWritten);
        connect(_adapter, &UomDeviceAdapter::activationFailed, &_workflow, &UomRegistrationWorkflow::deviceFailed);
        connect(&_workflow, &UomRegistrationWorkflow::readDeviceRequested, _adapter, &UomDeviceAdapter::readActivation);
        connect(&_workflow, &UomRegistrationWorkflow::writeDeviceRequested, _adapter, &UomDeviceAdapter::setActivation);
    }
    connect(MultiVehicleManager::instance(), &MultiVehicleManager::activeVehicleChanged, this, [this](Vehicle* active) {
        if (active != _vehicle) {
            _workflow.interrupt();
        }
        _syncDevice();
    });
    connect(_vehicle, &Vehicle::armedChanged, this, [this](bool armed) {
        if (armed) {
            _workflow.interrupt();
        }
        _syncDevice();
    });
    connect(_vehicle, &Vehicle::underwayChanged, this, [this](bool underway) {
        if (underway) {
            _workflow.interrupt();
        }
        _syncDevice();
    });
    connect(_vehicle->vehicleLinkManager(), &VehicleLinkManager::communicationLostChanged, this, [this](bool lost) {
        if (lost) {
            _workflow.interrupt();
        }
        _syncDevice();
    });
    _loadConfiguration();
    _syncDevice();
}

void VehicleUomRegistrationManager::_syncDevice()
{
    const bool writable = _adapter && _adapter->canWrite() && _vehicle && !_vehicle->armed() && !_vehicle->underway() &&
                          !_vehicle->vehicleLinkManager()->communicationLost() &&
                          MultiVehicleManager::instance()->activeVehicle() == _vehicle;
    _workflow.observeDevice(_adapter ? _adapter->device() : UomRegistrationDevice{}, writable);
    emit changed();
}

void VehicleUomRegistrationManager::refreshDevice()
{
    if (_adapter && !busy()) {
        _adapter->refresh();
    }
}

void VehicleUomRegistrationManager::selectDevice(int index)
{
    if (_adapter && !busy() && !pending()) {
        _workflow.interrupt();
        _adapter->selectDevice(index);
    }
}

QString VehicleUomRegistrationManager::deviceStatus() const
{
    return _adapter ? _adapter->statusText() : tr("No UOM activation adapter is available for this vehicle");
}

QVariantMap VehicleUomRegistrationManager::configurationForEnvironment(const QString& environment) const
{
    if (environment != QStringLiteral("integration") && environment != QStringLiteral("official")) {
        return {};
    }
    QSettings settings;
    settings.beginGroup(QStringLiteral("UomRegistration/") + environment);
    UomRegistration::Config defaults;
    const bool integration = environment == QStringLiteral("integration");
    return {{QStringLiteral("environment"), environment},
            {QStringLiteral("producerId"), settings.value(QStringLiteral("producerId"), QStringLiteral(""))},
            {QStringLiteral("expectedSerial"), settings.value(QStringLiteral("expectedSerial"), QString())},
            {QStringLiteral("verifyUrl"),
             settings.value(QStringLiteral("verifyUrl"), integration ? defaults.verifyUrl.toString() : QString())},
            {QStringLiteral("reportUrl"),
             settings.value(QStringLiteral("reportUrl"), integration ? defaults.reportUrl.toString() : QString())},
            {QStringLiteral("cancelUrl"),
             settings.value(QStringLiteral("cancelUrl"), integration ? defaults.cancelUrl.toString() : QString())}};
}

void VehicleUomRegistrationManager::_loadConfiguration()
{
    QSettings settings;
    const QString environment =
        settings.value(QStringLiteral("UomRegistration/environment"), QStringLiteral("integration")).toString();
    const auto values = configurationForEnvironment(environment);
    UomRegistration::Config config;
    config.environment = environment;
    config.producerId = values.value(QStringLiteral("producerId")).toString();
    config.expectedSerial = values.value(QStringLiteral("expectedSerial")).toString();
    config.verifyUrl = QUrl(values.value(QStringLiteral("verifyUrl")).toString());
    config.reportUrl = QUrl(values.value(QStringLiteral("reportUrl")).toString());
    config.cancelUrl = QUrl(values.value(QStringLiteral("cancelUrl")).toString());
    if (config.valid()) {
        _workflow.configure(config);
    }
}

bool VehicleUomRegistrationManager::configure(const QString& environment, const QString& producerId,
                                              const QString& expectedSerial, const QString& verifyUrl,
                                              const QString& reportUrl, const QString& cancelUrl, const QString& keyId)
{
    UomRegistration::Config config;
    config.environment = environment;
    config.producerId = producerId.trimmed();
    config.expectedSerial = expectedSerial.trimmed();
    config.verifyUrl = QUrl(verifyUrl.trimmed());
    config.reportUrl = QUrl(reportUrl.trimmed());
    config.cancelUrl = QUrl(cancelUrl.trimmed());
    if (!_workflow.configure(config, keyId)) {
        return false;
    }
    QSettings settings;
    settings.beginGroup(QStringLiteral("UomRegistration"));
    settings.setValue(QStringLiteral("environment"), config.environment);
    settings.beginGroup(config.environment);
    settings.setValue(QStringLiteral("producerId"), config.producerId);
    settings.setValue(QStringLiteral("expectedSerial"), config.expectedSerial);
    settings.setValue(QStringLiteral("verifyUrl"), config.verifyUrl.toString());
    settings.setValue(QStringLiteral("reportUrl"), config.reportUrl.toString());
    settings.setValue(QStringLiteral("cancelUrl"), config.cancelUrl.toString());
    return true;
}

QString VehicleUomRegistrationManager::statusText() const
{
    if (!_store->ready()) {
        return tr("UOM transaction storage is unavailable; operations are disabled");
    }
    if (!busy() && !serial().isEmpty() && serial() != expectedSerial()) {
        return tr("Connected product code does not match the configured aircraft; no operation is permitted");
    }
    return _workflow.statusText();
}

QString VehicleUomRegistrationManager::safetyStatus() const
{
    return _adapter ? _adapter->safetyStatus()
                    : tr("Device firmware must enforce flight restrictions before activation and after deactivation");
}
