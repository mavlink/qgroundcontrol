#include "VehicleReadiness.h"

#include "AppSettings.h"
#include "AutoPilotPlugin.h"
#include "Fact.h"
#include "FirmwarePlugin.h"
#include "HealthAndArmingCheckReport.h"
#include "QGCLoggingCategory.h"
#include "SettingsManager.h"
#include "Vehicle.h"

QGC_LOGGING_CATEGORY(VehicleReadinessLog, "Vehicle.VehicleReadiness")

VehicleReadiness::VehicleReadiness(Vehicle* vehicle)
    : QObject(vehicle)
    , _vehicle(vehicle)
{
    connect(_vehicle, &Vehicle::armedChanged, this, &VehicleReadiness::update);
    connect(_vehicle, &Vehicle::checkListStateChanged, this, &VehicleReadiness::update);
    connect(_vehicle, &Vehicle::initialConnectComplete, this, [this]() {
        _healthReportWaitTimer.start();
        update();
    });
    connect(_vehicle, &Vehicle::readyToFlyAvailableChanged, this, &VehicleReadiness::update);
    connect(_vehicle, &Vehicle::readyToFlyChanged, this, &VehicleReadiness::update);
    connect(_vehicle, &Vehicle::allSensorsHealthyChanged, this, &VehicleReadiness::update);

    HealthAndArmingCheckReport* const report = _vehicle->healthAndArmingCheckReport();
    if (report) {
        connect(report, &HealthAndArmingCheckReport::updated, this, &VehicleReadiness::update);
    }

    AutoPilotPlugin* const autopilotPlugin = _vehicle->autopilotPlugin();
    if (autopilotPlugin) {
        connect(autopilotPlugin, &AutoPilotPlugin::setupCompleteChanged, this, &VehicleReadiness::update);
    }

    AppSettings* const appSettings = SettingsManager::instance()->appSettings();
    connect(appSettings->useChecklist(), &Fact::rawValueChanged, this, &VehicleReadiness::update);
    connect(appSettings->enforceChecklist(), &Fact::rawValueChanged, this, &VehicleReadiness::update);

    _healthReportWaitTimer.setSingleShot(true);
    _healthReportWaitTimer.setInterval(_healthReportWaitMs);
    connect(&_healthReportWaitTimer, &QTimer::timeout, this, &VehicleReadiness::_healthReportWaitTimeout);
}

void VehicleReadiness::_healthReportWaitTimeout()
{
    _healthReportWaitExpired = true;

    const Inputs inputs = _gatherInputs();
    if (inputs.healthAndArmingChecksSupported && !inputs.healthAndArmingChecksReported) {
        qCWarning(VehicleReadinessLog) << "No health and arming check report received, falling back to other sources";
    }
    update();
}

VehicleReadiness::Result VehicleReadiness::_evaluate(const Inputs& inputs)
{
    Result result;

    if (inputs.healthAndArmingChecksReported) {
        const auto modeGroupSet = [](quint64 modeGroups, int modeGroup) {
            if ((modeGroup < 0) || (modeGroup >= HealthAndArmingCheckReport::MaxModeGroups)) {
                return false;
            }
            return (modeGroups & (1ull << modeGroup)) != 0;
        };
        const auto actionReady = [&inputs, &modeGroupSet](const ActionModeGroups& modeGroups) {
            const int armModeGroup = (modeGroups.armModeGroup < 0) ? inputs.currentModeGroup : modeGroups.armModeGroup;
            const int targetModeGroup =
                (modeGroups.targetModeGroup < 0) ? inputs.currentModeGroup : modeGroups.targetModeGroup;

            // An armed vehicle only switches into the target mode
            if (inputs.armed) {
                return modeGroupSet(inputs.canRunModeGroups, targetModeGroup);
            }

            // An action which switches mode after arming also needs to be able to run in the target mode. libevents
            // only promises can_run is meaningful while armed, so this part is best effort.
            const bool canArmModeGroup = modeGroupSet(inputs.canArmModeGroups, armModeGroup);
            if (armModeGroup == targetModeGroup) {
                return canArmModeGroup;
            }
            return canArmModeGroup && modeGroupSet(inputs.canRunModeGroups, targetModeGroup);
        };

        result.canArm = actionReady(ActionModeGroups{});
        result.canTakeoff = actionReady(inputs.takeoff);
        result.canStartMission = actionReady(inputs.startMission);
        if (inputs.currentModeGroup < 0) {
            // The current mode is not in the vehicle's metadata, so its readiness is unknown
            result.status = Status::Unknown;
        } else {
            result.status = result.canArm ? Status::Ready : Status::NotReady;
        }
    } else if (!inputs.healthReportWaitExpired &&
               (inputs.healthAndArmingChecksSupported ||
                (inputs.healthAndArmingChecksExpected && !inputs.initialConnectComplete))) {
        result.status = Status::Unknown;
    } else if (inputs.prearmCheckAvailable) {
        result.canArm = inputs.prearmCheckPassed;
        result.canTakeoff = inputs.prearmCheckPassed;
        result.canStartMission = inputs.prearmCheckPassed;
        result.status = inputs.prearmCheckPassed ? Status::Ready : Status::NotReady;
    } else {
        result.canArm = true;
        result.canTakeoff = true;
        result.canStartMission = true;
        // Sensor health and setup are pre-flight hints, so they never mark an armed vehicle not ready
        result.status =
            (inputs.armed || (inputs.sensorsHealthy && inputs.setupComplete)) ? Status::Ready : Status::NotReady;
    }

    return result;
}

void VehicleReadiness::_applyChecklist(Result& result, const Inputs& inputs)
{
    // The checklist is pre-flight, so it only blocks a disarmed vehicle
    if (!inputs.armed && inputs.checklistBlocksArming) {
        result.canArm = false;
        result.canTakeoff = false;
        result.canStartMission = false;
        result.status = Status::NotReady;
    }
}

VehicleReadiness::Inputs VehicleReadiness::_gatherInputs() const
{
    Inputs inputs;

    inputs.armed = _vehicle->armed();
    inputs.initialConnectComplete = _vehicle->isInitialConnectComplete();
    inputs.healthReportWaitExpired = _healthReportWaitExpired;

    AppSettings* const appSettings = SettingsManager::instance()->appSettings();
    inputs.checklistBlocksArming = appSettings->useChecklist()->rawValue().toBool() &&
                                   appSettings->enforceChecklist()->rawValue().toBool() &&
                                   (_vehicle->checkListState() != Vehicle::CheckListPassed);

    inputs.healthAndArmingChecksExpected = _expectsHealthAndArmingChecks();
    inputs.healthAndArmingChecksSupported = _vehicle->healthAndArmingChecksSupported(MAV_COMP_ID_AUTOPILOT1);
    const HealthAndArmingCheckReport* const report = _vehicle->healthAndArmingCheckReport();
    if (report && report->supported()) {
        const HealthAndArmingCheckReport::ModeGroupResults& modeGroupResults = report->modeGroupResults();
        inputs.healthAndArmingChecksReported = true;
        inputs.currentModeGroup = modeGroupResults.currentModeGroup;
        inputs.canArmModeGroups = modeGroupResults.canArm;
        inputs.canRunModeGroups = modeGroupResults.canRun;
        inputs.takeoff = _actionModeGroups(GuidedAction::Takeoff);
        inputs.startMission = _actionModeGroups(GuidedAction::StartMission);
    }

    inputs.prearmCheckAvailable = _vehicle->readyToFlyAvailable();
    inputs.prearmCheckPassed = _vehicle->readyToFly();

    inputs.sensorsHealthy = _vehicle->allSensorsHealthy();
    const AutoPilotPlugin* const autopilotPlugin = _vehicle->autopilotPlugin();
    inputs.setupComplete = autopilotPlugin && autopilotPlugin->setupComplete();

    return inputs;
}

VehicleReadiness::ActionModeGroups VehicleReadiness::_actionModeGroups(GuidedAction action) const
{
    const ActionFlightModes flightModes = _actionFlightModes(action);
    return ActionModeGroups{.armModeGroup = _modeGroup(flightModes.armFlightMode),
                            .targetModeGroup = _modeGroup(flightModes.targetFlightMode)};
}

int VehicleReadiness::_modeGroup(const QString& flightMode) const
{
    if (flightMode.isEmpty()) {
        return -1;
    }

    uint8_t baseMode = 0;
    uint32_t customMode = 0;
    if (!_vehicle->firmwarePlugin()->setFlightMode(flightMode, &baseMode, &customMode)) {
        return -1;
    }
    return _vehicle->healthAndArmingCheckModeGroup(customMode);
}

void VehicleReadiness::update()
{
    const Inputs inputs = _gatherInputs();
    qCDebug(VehicleReadinessLog) << "armed:" << inputs.armed
                                 << "initialConnectComplete:" << inputs.initialConnectComplete
                                 << "checklistBlocksArming:" << inputs.checklistBlocksArming
                                 << "healthChecks expected/supported/reported:" << inputs.healthAndArmingChecksExpected
                                 << inputs.healthAndArmingChecksSupported << inputs.healthAndArmingChecksReported
                                 << "currentModeGroup:" << inputs.currentModeGroup
                                 << "takeoff arm/target:" << inputs.takeoff.armModeGroup
                                 << inputs.takeoff.targetModeGroup
                                 << "startMission arm/target:" << inputs.startMission.armModeGroup
                                 << inputs.startMission.targetModeGroup << "canArm:" << Qt::hex
                                 << inputs.canArmModeGroups << "canRun:" << inputs.canRunModeGroups << Qt::dec
                                 << "prearm available/passed:" << inputs.prearmCheckAvailable
                                 << inputs.prearmCheckPassed << "sensorsHealthy:" << inputs.sensorsHealthy
                                 << "setupComplete:" << inputs.setupComplete;

    const Result generic = _evaluate(inputs);
    Result result{.canArm = _canArm(generic.canArm),
                  .canTakeoff = _canTakeoff(generic.canTakeoff),
                  .canStartMission = _canStartMission(generic.canStartMission),
                  .status = _status(generic.status)};
    _applyChecklist(result, inputs);

    if (result == _result) {
        return;
    }

    _result = result;
    qCDebug(VehicleReadinessLog) << "canArm:" << _result.canArm << "canTakeoff:" << _result.canTakeoff
                                 << "canStartMission:" << _result.canStartMission << "status:" << _result.status;
    emit readinessChanged();
}
