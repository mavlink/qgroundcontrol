#include "OnboardLogController.h"

#include <QtCore/QPersistentModelIndex>
#include <QtCore/QSignalBlocker>
#include <utility>

#include "Fact.h"
#include "FirmwarePlugin.h"
#include "FtpTransport.h"
#include "LogProtocolTransport.h"
#include "MAVLinkLib.h"
#include "MavlinkSettings.h"
#include "MultiVehicleManager.h"
#include "OnboardLogEntry.h"
#include "OnboardLogModel.h"
#include "OnboardLogSortFilterModel.h"
#include "OnboardLogTransport.h"
#include "QGCLoggingCategory.h"
#include "SettingsManager.h"
#include "Vehicle.h"

QGC_LOGGING_CATEGORY(OnboardLogControllerLog, "AnalyzeView.OnboardLogs.OnboardLogController")

namespace {

enum class TransportOverride : uint32_t
{
    Auto = 0,
    LogProtocol = 1,
    MavlinkFtp = 2,
};

}  // namespace

OnboardLogController::OnboardLogController(QObject* parent)
    : QObject(parent),
      _logTransport(new LogProtocolTransport(this)),
      _ftpTransport(new FtpTransport(this)),
      _sortModel(new OnboardLogSortFilterModel(this))
{
    qCDebug(OnboardLogControllerLog) << this;

    _setActiveTransport(_logTransport);

    (void) connect(MultiVehicleManager::instance(), &MultiVehicleManager::activeVehicleChanged, this,
                   &OnboardLogController::_setActiveVehicle);

    // Re-pick transport when the user toggles the override Fact at runtime.
    Fact* const transportFact = SettingsManager::instance()->mavlinkSettings()->onboardLogTransport();
    (void) connect(transportFact, &Fact::rawValueChanged, this, &OnboardLogController::_reevaluateTransport);

    _setActiveVehicle(MultiVehicleManager::instance()->activeVehicle());
}

OnboardLogController::~OnboardLogController() = default;

QAbstractItemModel* OnboardLogController::model() const
{
    return _sortModel;
}

OnboardLogModel* OnboardLogController::sourceModel() const
{
    return _active ? _active->model() : nullptr;
}

bool OnboardLogController::downloadingLogs() const
{
    return _active && _active->downloadingLogs();
}

OnboardLogController::TransportKind OnboardLogController::transportKind() const
{
    return (_active == _ftpTransport) ? TransportKind::MavlinkFtp : TransportKind::LogProtocol;
}

qreal OnboardLogController::batchProgress() const
{
    return _active ? _active->batchProgress() : 0.;
}

QString OnboardLogController::errorMessage() const
{
    return _active ? _active->errorMessage() : QString();
}

void OnboardLogController::ensureLoaded()
{
    if (!_vehicle || !_active || busy()) {
        return;
    }

    const bool sameLoadKey = (_loadVehicle == _vehicle) && (_loadTransport == _active);
    if (sameLoadKey && ((_loadState == LoadState::Loading) || (_loadState == LoadState::Loaded))) {
        return;
    }

    refresh();
}

void OnboardLogController::refresh()
{
    if (!_active || busy()) {
        return;
    }

    _loadVehicle = _vehicle;
    _loadTransport = _active;
    _loadState = LoadState::Loading;
    _active->refresh();

    // A transport that cannot start a listing returns to idle synchronously.
    if (!_active->requestingList() && (_loadState == LoadState::Loading)) {
        _loadState = LoadState::Failed;
    }
}

void OnboardLogController::download(const QString& path)
{
    if (_active && !busy()) {
        _active->download(path);
    }
}

bool OnboardLogController::eraseAllForVehicle(Vehicle* expectedVehicle)
{
    OnboardLogModel* const logModel = sourceModel();
    if (!expectedVehicle || (expectedVehicle != _vehicle) || !_active || busy() || !logModel ||
        (logModel->count() == 0)) {
        qCWarning(OnboardLogControllerLog)
            << "Ignoring stale or invalid onboard log erase request" << expectedVehicle << _vehicle;
        return false;
    }

    _active->eraseAll();
    return true;
}

void OnboardLogController::cancel()
{
    if (_active) {
        if ((_loadState == LoadState::Loading) && (_loadVehicle == _vehicle) && (_loadTransport == _active)) {
            _invalidateLoadState();
        }
        _active->cancel();
    }
}

void OnboardLogController::selectAll(bool select)
{
    OnboardLogTransport* const activeTransport = _active;
    OnboardLogModel* const logModel = sourceModel();
    if (!activeTransport || !logModel) {
        return;
    }

    struct SelectionTarget
    {
        QPointer<OnboardLogEntry> entry;
        QPersistentModelIndex index;
    };

    QList<SelectionTarget> entries;
    const int count = logModel->count();
    entries.reserve(count);
    for (int index = 0; index < count; ++index) {
        OnboardLogEntry* const entry = logModel->value<OnboardLogEntry*>(index);
        if (entry && entry->received() && (entry->selected() != select)) {
            entries.append({entry, QPersistentModelIndex(logModel->index(index, 0))});
        }
    }

    const auto targetIsCurrent = [this, activeTransport, logModel](const SelectionTarget& target) {
        return (_active == activeTransport) && (sourceModel() == logModel) && target.entry && target.index.isValid() &&
               (target.index.model() == logModel) &&
               (target.index.data(Qt::UserRole).value<QObject*>() == target.entry.data());
    };

    // Keep per-entry notifications alive for QML delegates while suppressing the
    // transport's aggregate selectionChanged signal until the batch is complete.
    const QSignalBlocker blocker(activeTransport);
    for (const SelectionTarget& target : std::as_const(entries)) {
        if (!targetIsCurrent(target)) {
            emit selectionChanged();
            return;
        }

        target.entry->setSelected(select);
        if (!targetIsCurrent(target)) {
            emit selectionChanged();
            return;
        }
    }
    emit selectionChanged();
}

int OnboardLogController::selectedCount() const
{
    const OnboardLogModel* const logModel = sourceModel();
    if (!logModel) {
        return 0;
    }

    int selected = 0;
    const int count = logModel->count();
    for (int i = 0; i < count; i++) {
        const OnboardLogEntry* const entry = logModel->value<const OnboardLogEntry*>(i);
        if (entry && entry->received() && entry->selected()) {
            selected++;
        }
    }

    return selected;
}

bool OnboardLogController::allLogsSelected() const
{
    const OnboardLogModel* const logModel = sourceModel();
    if (!logModel) {
        return false;
    }

    bool hasSelectableLog = false;
    const int count = logModel->count();
    for (int i = 0; i < count; i++) {
        const OnboardLogEntry* const entry = logModel->value<const OnboardLogEntry*>(i);
        if (entry && entry->received()) {
            hasSelectableLog = true;
            if (!entry->selected()) {
                return false;
            }
        }
    }

    return hasSelectableLog;
}

void OnboardLogController::toggleSortByDate()
{
    setSortAscending(!_sortAscending);
}

void OnboardLogController::setSortAscending(bool ascending)
{
    if (_sortAscending == ascending) {
        return;
    }

    _sortAscending = ascending;
    _sortModel->setAscending(ascending);
    emit sortAscendingChanged();
}

void OnboardLogController::_setActiveVehicle(Vehicle* vehicle)
{
    if (_changingVehicle && (vehicle == _vehicle)) {
        return;
    }

    _pendingVehicle = vehicle;
    _vehicleChangePending = true;
    if (_changingVehicle) {
        return;
    }

    _changingVehicle = true;
    while (_vehicleChangePending) {
        const QPointer<Vehicle> targetVehicle = _pendingVehicle;
        _vehicleChangePending = false;

        if (targetVehicle && (targetVehicle == _vehicle)) {
            _reevaluateTransport();
            continue;
        }

        if (_vehicle) {
            (void) disconnect(_vehicle, &Vehicle::capabilityBitsChanged, this,
                              &OnboardLogController::_reevaluateTransport);
        }

        _invalidateLoadState();
        _vehicle = targetVehicle;
        _logTransport->setVehicle(targetVehicle);
        if (_vehicleChangePending) {
            continue;
        }
        _ftpTransport->setVehicle(targetVehicle);
        if (_vehicleChangePending) {
            continue;
        }

        if (_vehicle) {
            // Capability bits may arrive after the vehicle is active; re-pick when they do.
            (void) connect(_vehicle, &Vehicle::capabilityBitsChanged, this,
                           &OnboardLogController::_reevaluateTransport);
        }

        _reevaluateTransport();
    }
    _changingVehicle = false;
}

bool OnboardLogController::_shouldAutoSelectFtp(bool firmwareAllowsFtp, bool capabilitiesKnown, uint64_t capabilityBits)
{
    return firmwareAllowsFtp && capabilitiesKnown && ((capabilityBits & MAV_PROTOCOL_CAPABILITY_FTP) != 0);
}

void OnboardLogController::_reevaluateTransport()
{
    const uint rawOverride = SettingsManager::instance()->mavlinkSettings()->onboardLogTransport()->rawValue().toUInt();
    TransportOverride transportOverride = TransportOverride::Auto;
    if (rawOverride <= static_cast<uint>(TransportOverride::MavlinkFtp)) {
        transportOverride = static_cast<TransportOverride>(rawOverride);
    } else {
        qCWarning(OnboardLogControllerLog) << "Invalid onboard-log transport override:" << rawOverride;
    }

    OnboardLogTransport* target = _logTransport;
    switch (transportOverride) {
        case TransportOverride::LogProtocol:
            target = _logTransport;
            break;
        case TransportOverride::MavlinkFtp:
            target = _ftpTransport;
            break;
        case TransportOverride::Auto:
            if (_vehicle && _shouldAutoSelectFtp(_vehicle->firmwarePlugin()->onboardLogPolicy(_vehicle).autoSelectFtp,
                                                 _vehicle->capabilitiesKnown(), _vehicle->capabilityBits())) {
                target = _ftpTransport;
            }
            break;
    }

    qCDebug(OnboardLogControllerLog) << "vehicle" << _vehicle << "override" << rawOverride << "→ transport"
                                     << ((target == _ftpTransport) ? "ftp" : "log");

    _setActiveTransport(target);
}

void OnboardLogController::_activeRequestingListChanged()
{
    if (_active && !_active->requestingList()) {
        _sortModel->resort();
    }
}

void OnboardLogController::_activeListingFinished(OnboardLogTransport::ListingResult result)
{
    if ((_loadState != LoadState::Loading) || (_loadVehicle != _vehicle) || (_loadTransport != _active)) {
        return;
    }

    switch (result) {
        case OnboardLogTransport::ListingResult::Success:
        case OnboardLogTransport::ListingResult::Partial:
            _loadState = LoadState::Loaded;
            break;
        case OnboardLogTransport::ListingResult::Failed:
            _loadState = LoadState::Failed;
            break;
        case OnboardLogTransport::ListingResult::Canceled:
            _loadState = LoadState::NotLoaded;
            break;
    }
}

void OnboardLogController::_invalidateLoadState()
{
    _loadVehicle = nullptr;
    _loadTransport = nullptr;
    _loadState = LoadState::NotLoaded;
}

void OnboardLogController::_setActiveTransport(OnboardLogTransport* transport)
{
    if (transport == _active) {
        return;
    }

    if (_active) {
        (void) disconnect(_active, &OnboardLogTransport::requestingListChanged, this,
                          &OnboardLogController::_activeRequestingListChanged);
        (void) disconnect(_active, &OnboardLogTransport::listingFinished, this,
                          &OnboardLogController::_activeListingFinished);
        (void) disconnect(_active, &OnboardLogTransport::downloadingLogsChanged, this,
                          &OnboardLogController::downloadingLogsChanged);
        (void) disconnect(_active, &OnboardLogTransport::busyChanged, this, &OnboardLogController::busyChanged);
        (void) disconnect(_active, &OnboardLogTransport::batchProgressChanged, this,
                          &OnboardLogController::batchProgressChanged);
        (void) disconnect(_active, &OnboardLogTransport::errorMessageChanged, this,
                          &OnboardLogController::errorMessageChanged);
        (void) disconnect(_active, &OnboardLogTransport::selectionChanged, this,
                          &OnboardLogController::selectionChanged);
        _invalidateLoadState();
        _active->cancel();  // stop any in-flight work on the deselected transport
    }

    _active = transport;

    if (_active) {
        (void) connect(_active, &OnboardLogTransport::requestingListChanged, this,
                       &OnboardLogController::_activeRequestingListChanged);
        (void) connect(_active, &OnboardLogTransport::listingFinished, this,
                       &OnboardLogController::_activeListingFinished);
        (void) connect(_active, &OnboardLogTransport::downloadingLogsChanged, this,
                       &OnboardLogController::downloadingLogsChanged);
        (void) connect(_active, &OnboardLogTransport::busyChanged, this, &OnboardLogController::busyChanged);
        (void) connect(_active, &OnboardLogTransport::batchProgressChanged, this,
                       &OnboardLogController::batchProgressChanged);
        (void) connect(_active, &OnboardLogTransport::errorMessageChanged, this,
                       &OnboardLogController::errorMessageChanged);
        (void) connect(_active, &OnboardLogTransport::selectionChanged, this, &OnboardLogController::selectionChanged);
    }

    _sortModel->setSourceModel(_active ? _active->model() : nullptr);
    _sortModel->sort(0);
    emit downloadingLogsChanged();
    emit busyChanged();
    emit transportKindChanged();
    emit batchProgressChanged();
    emit errorMessageChanged();
    emit selectionChanged();
}
