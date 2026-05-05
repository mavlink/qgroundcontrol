#include "OnboardLogController.h"

#include "Fact.h"
#include "FtpTransport.h"
#include "LogProtocolTransport.h"
#include "MAVLinkLib.h"
#include "MavlinkSettings.h"
#include "MultiVehicleManager.h"
#include "QGCLoggingCategory.h"
#include "SettingsManager.h"
#include "Vehicle.h"

QGC_LOGGING_CATEGORY(OnboardLogControllerLog, "AnalyzeView.OnboardLogController")

OnboardLogController::OnboardLogController(QObject* parent)
    : OnboardLogTransport(parent), _logTransport(new LogProtocolTransport(this)), _ftpTransport(new FtpTransport(this))
{
    qCDebug(OnboardLogControllerLog) << this;

    _setActiveTransport(_logTransport);

    (void)connect(MultiVehicleManager::instance(), &MultiVehicleManager::activeVehicleChanged, this,
                  &OnboardLogController::_setActiveVehicle);

    // Re-pick transport when the user toggles the override Fact at runtime.
    Fact* const transportFact = SettingsManager::instance()->mavlinkSettings()->onboardLogTransport();
    (void)connect(transportFact, &Fact::rawValueChanged, this, &OnboardLogController::_reevaluateTransport);

    _setActiveVehicle(MultiVehicleManager::instance()->activeVehicle());
}

OnboardLogController::~OnboardLogController() = default;

QmlObjectListModel* OnboardLogController::model() const
{
    return _active ? _active->model() : nullptr;
}

bool OnboardLogController::requestingList() const
{
    return _active && _active->requestingList();
}

bool OnboardLogController::downloadingLogs() const
{
    return _active && _active->downloadingLogs();
}

bool OnboardLogController::canErase() const
{
    return _active && _active->canErase();
}

QString OnboardLogController::transportName() const
{
    return _active ? _active->transportName() : QString();
}

void OnboardLogController::refresh()
{
    if (_active)
        _active->refresh();
}

void OnboardLogController::download(const QString& path)
{
    if (_active)
        _active->download(path);
}

void OnboardLogController::eraseAll()
{
    if (_active)
        _active->eraseAll();
}

void OnboardLogController::cancel()
{
    if (_active)
        _active->cancel();
}

void OnboardLogController::_setActiveVehicle(Vehicle* vehicle)
{
    if (vehicle == _vehicle) {
        _reevaluateTransport();
        return;
    }

    if (_vehicle) {
        disconnect(_vehicle, &Vehicle::capabilityBitsChanged, this, &OnboardLogController::_reevaluateTransport);
    }

    _vehicle = vehicle;

    if (_vehicle) {
        // Capability bits may arrive after the vehicle is active; re-pick when they do.
        (void)connect(_vehicle, &Vehicle::capabilityBitsChanged, this, &OnboardLogController::_reevaluateTransport);
    }

    _reevaluateTransport();
}

void OnboardLogController::_reevaluateTransport()
{
    enum TransportOverride { Auto = 0, LogProtocol = 1, MavlinkFtp = 2 };
    const TransportOverride override_ = static_cast<TransportOverride>(
        SettingsManager::instance()->mavlinkSettings()->onboardLogTransport()->rawValue().toUInt());

    OnboardLogTransport* target = _logTransport;
    switch (override_) {
    case LogProtocol:
        target = _logTransport;
        break;
    case MavlinkFtp:
        target = _ftpTransport;
        break;
    case Auto:
    default:
        if (_vehicle && _vehicle->capabilitiesKnown() && (_vehicle->capabilityBits() & MAV_PROTOCOL_CAPABILITY_FTP)) {
            target = _ftpTransport;
        }
        break;
    }

    qCDebug(OnboardLogControllerLog) << "vehicle" << _vehicle << "override" << override_ << "→ transport"
                                     << (target ? target->transportName() : QStringLiteral("none"));

    _setActiveTransport(target);
}

void OnboardLogController::_setActiveTransport(OnboardLogTransport* transport)
{
    if (transport == _active) {
        return;
    }

    if (_active) {
        disconnect(_active, nullptr, this, nullptr);
        _active->cancel();  // stop any in-flight work on the deselected transport
    }

    _active = transport;

    if (_active) {
        (void)connect(_active, &OnboardLogTransport::modelChanged, this, &OnboardLogController::modelChanged);
        (void)connect(_active, &OnboardLogTransport::requestingListChanged, this,
                      &OnboardLogController::requestingListChanged);
        (void)connect(_active, &OnboardLogTransport::downloadingLogsChanged, this,
                      &OnboardLogController::downloadingLogsChanged);
        (void)connect(_active, &OnboardLogTransport::canEraseChanged, this, &OnboardLogController::canEraseChanged);
        (void)connect(_active, &OnboardLogTransport::transportNameChanged, this,
                      &OnboardLogController::transportNameChanged);
        (void)connect(_active, &OnboardLogTransport::selectionChanged, this, &OnboardLogController::selectionChanged);
    }

    emit modelChanged();
    emit requestingListChanged();
    emit downloadingLogsChanged();
    emit canEraseChanged();
    emit transportNameChanged();
}
