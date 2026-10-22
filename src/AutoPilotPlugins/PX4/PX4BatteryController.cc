#include "PX4BatteryController.h"

#include <QtCore/QHash>
#include <QtCore/QPointer>

#include "Fact.h"
#include "ParameterManager.h"
#include "Vehicle.h"

namespace {

struct UnansweredParams
{
    QPointer<const ParameterManager> parameterManager;
    QSet<QString> names;
};

/// Names each vehicle did not answer, so reopening the page does not wait for them again. PX4 does not reply to a
/// name it lacks, so a read lost on a bad link is also remembered until the vehicle reconnects.
QSet<QString>& unansweredParams(const ParameterManager* parameterManager)
{
    static QHash<const ParameterManager*, UnansweredParams> cache;
    UnansweredParams& entry = cache[parameterManager];
    // A reconnected vehicle can get a new parameter manager at the address of a destroyed one
    if (entry.parameterManager != parameterManager) {
        entry.parameterManager = parameterManager;
        entry.names.clear();
    }
    return entry.names;
}

}  // namespace

PX4BatteryController::PX4BatteryController(QObject* parent)
    : FactPanelController(parent)
{
    if (!_vehicle) {
        return;
    }
    _parameterManager = _vehicle->parameterManager();
    if (!_parameterManager) {
        return;
    }

    (void) connect(_parameterManager, &ParameterManager::factAdded, this, &PX4BatteryController::_factAdded);
    (void) connect(_parameterManager, &ParameterManager::_paramRequestReadFailure, this,
                   &PX4BatteryController::_parameterReadFailed);
}

void PX4BatteryController::requestParameters(const QStringList& names)
{
    if (!_parameterManager) {
        return;
    }

    const QSet<QString> unanswered = unansweredParams(_parameterManager);
    const qsizetype pendingCount = _pendingParams.count();
    for (const QString& name : names) {
        if (_requestedParams.contains(name) || unanswered.contains(name) ||
            _parameterManager->parameterExists(ParameterManager::defaultComponentId, name)) {
            continue;
        }
        _requestedParams.insert(name);
        _pendingParams.insert(name);
        _parameterManager->refreshParameter(ParameterManager::defaultComponentId, name, false /* notifyFailure */);
    }

    if (_pendingParams.count() != pendingCount) {
        emit pendingParamsChanged();
    }
}

bool PX4BatteryController::parameterReady(const QString& name) const
{
    return _parameterManager && !_pendingParams.contains(name) &&
           _parameterManager->parameterExists(ParameterManager::defaultComponentId, name);
}

void PX4BatteryController::_factAdded(int componentId, Fact* fact)
{
    if (!_vehicle || !fact || (componentId != _vehicle->defaultComponentId())) {
        return;
    }

    // Queued because factAdded is emitted before ParameterManager sets the new fact's value from the vehicle
    const QString name = fact->name();
    (void) QMetaObject::invokeMethod(this, [this, name]() { _paramAdded(name); }, Qt::QueuedConnection);
}

void PX4BatteryController::_parameterReadFailed(int componentId, const QString& paramName)
{
    if (!_vehicle || !_parameterManager || (componentId != _vehicle->defaultComponentId()) ||
        !_pendingParams.contains(paramName)) {
        return;
    }

    unansweredParams(_parameterManager).insert(paramName);
    _removePending(paramName);
}

void PX4BatteryController::_paramAdded(const QString& paramName)
{
    const bool wasPending = _pendingParams.remove(paramName);

    _addedParamCount++;
    emit addedParamCountChanged();
    if (wasPending) {
        emit pendingParamsChanged();
    }
}

void PX4BatteryController::_removePending(const QString& paramName)
{
    if (_pendingParams.remove(paramName)) {
        emit pendingParamsChanged();
    }
}
