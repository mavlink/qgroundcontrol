#include "VehicleReadinessTest.h"

#include <optional>

#include <QtCore/QRegularExpression>
#include <QtCore/QScopeGuard>
#include <QtCore/QTemporaryDir>

#include "APMFirmwarePlugin.h"
#include "APMVehicleReadiness.h"
#include "AppSettings.h"
#include "Fact.h"
#include "FirmwarePlugin.h"
#include "HealthReportFixtures.h"
#include "MAVLinkLib.h"
#include "MockLink.h"
#include "MultiVehicleManager.h"
#include "PX4VehicleReadiness.h"
#include "SettingsManager.h"
#include "Vehicle.h"

// ---------------------------------------------------------------------------
// VehicleReadinessTest
// ---------------------------------------------------------------------------

QList<VehicleReadinessTest::EvaluateCase> VehicleReadinessTest::_evaluateCases()
{
    using Inputs = VehicleReadiness::Inputs;
    using Result = VehicleReadiness::Result;
    using Status = VehicleReadiness::Status;

    constexpr quint64 manual = 1ull << 0;
    constexpr quint64 takeoff = 1ull << 1;
    constexpr quint64 mission = 1ull << 2;
    constexpr quint64 all = manual | takeoff | mission;

    const auto libEvents = [](quint64 canArmModeGroups, quint64 canRunModeGroups) {
        Inputs inputs;
        inputs.initialConnectComplete = true;
        inputs.healthAndArmingChecksExpected = true;
        inputs.healthAndArmingChecksSupported = true;
        inputs.healthAndArmingChecksReported = true;
        inputs.currentModeGroup = 0;
        inputs.takeoff = {.armModeGroup = 1, .targetModeGroup = 1};
        inputs.startMission = {.armModeGroup = 2, .targetModeGroup = 2};
        inputs.canArmModeGroups = canArmModeGroups;
        inputs.canRunModeGroups = canRunModeGroups;
        return inputs;
    };
    const auto armed = [](Inputs inputs) {
        inputs.armed = true;
        return inputs;
    };
    const auto checklistBlocking = [](Inputs inputs) {
        inputs.checklistBlocksArming = true;
        return inputs;
    };
    const auto modeGroups = [](Inputs inputs, int current, int takeoffGroup, int missionGroup) {
        inputs.currentModeGroup = current;
        inputs.takeoff = {.armModeGroup = takeoffGroup, .targetModeGroup = takeoffGroup};
        inputs.startMission = {.armModeGroup = missionGroup, .targetModeGroup = missionGroup};
        return inputs;
    };
    // Start mission arms in the takeoff group, then switches to the mission group
    const auto armThenSwitch = [](Inputs inputs) {
        inputs.startMission = {.armModeGroup = 1, .targetModeGroup = 2};
        return inputs;
    };
    const auto prearmCheck = [](bool passed, bool expected, bool initialConnectComplete) {
        Inputs inputs;
        inputs.initialConnectComplete = initialConnectComplete;
        inputs.healthAndArmingChecksExpected = expected;
        inputs.prearmCheckAvailable = true;
        inputs.prearmCheckPassed = passed;
        return inputs;
    };
    const auto sensorHealth = [](bool sensorsHealthy, bool setupComplete) {
        Inputs inputs;
        inputs.initialConnectComplete = true;
        inputs.sensorsHealthy = sensorsHealthy;
        inputs.setupComplete = setupComplete;
        return inputs;
    };
    const auto result = [](bool canArm, bool canTakeoff, bool canStartMission, Status status) {
        return Result{.canArm = canArm, .canTakeoff = canTakeoff, .canStartMission = canStartMission, .status = status};
    };

    Inputs waitingForSupportedReport = prearmCheck(true, false, true);
    waitingForSupportedReport.healthAndArmingChecksSupported = true;
    Inputs supportedReportWaitExpired = waitingForSupportedReport;
    supportedReportWaitExpired.healthReportWaitExpired = true;

    return {
        {"libEventsAllReady", libEvents(all, all), result(true, true, true, Status::Ready)},
        {"libEventsTakeoffBlocked", libEvents(manual | mission, all), result(true, false, true, Status::Ready)},
        {"libEventsCurrentModeBlocked", libEvents(takeoff | mission, all), result(false, true, true, Status::NotReady)},
        {"libEventsDisarmedIgnoresCanRun", libEvents(all, 0), result(true, true, true, Status::Ready)},
        {"libEventsArmedUsesCanRun", armed(libEvents(0, all)), result(true, true, true, Status::Ready)},
        {"libEventsArmedMissionCannotRun", armed(libEvents(all, manual | takeoff)),
         result(true, true, false, Status::Ready)},
        {"libEventsArmedCurrentModeCannotRun", armed(libEvents(all, takeoff | mission)),
         result(false, true, true, Status::NotReady)},
        {"libEventsUnresolvedActionModesUseCurrent", modeGroups(libEvents(manual, 0), 0, -1, -1),
         result(true, true, true, Status::Ready)},
        {"libEventsUnresolvedCurrentMode", modeGroups(libEvents(all, all), -1, 1, 2),
         result(false, true, true, Status::Unknown)},
        {"libEventsUnresolvedCurrentAndActionModes", modeGroups(libEvents(all, all), -1, -1, -1),
         result(false, false, false, Status::Unknown)},
        {"libEventsModeGroupOutOfRange", modeGroups(libEvents(~0ull, ~0ull), 0, 64, 2),
         result(true, false, true, Status::Ready)},
        {"libEventsArmThenSwitchReady", armThenSwitch(libEvents(manual | takeoff, all)),
         result(true, true, true, Status::Ready)},
        {"libEventsArmThenSwitchArmModeBlocked", armThenSwitch(libEvents(manual | mission, all)),
         result(true, false, false, Status::Ready)},
        {"libEventsArmThenSwitchTargetCannotRun", armThenSwitch(libEvents(all, manual | takeoff)),
         result(true, true, false, Status::Ready)},
        {"libEventsArmedArmThenSwitchUsesTarget", armed(armThenSwitch(libEvents(0, mission))),
         result(false, false, true, Status::NotReady)},
        {"waitingForSupportedReport", waitingForSupportedReport, result(false, false, false, Status::Unknown)},
        {"supportedReportWaitExpired", supportedReportWaitExpired, result(true, true, true, Status::Ready)},
        {"waitingForExpectedReport", prearmCheck(true, true, false), result(false, false, false, Status::Unknown)},
        {"expectedReportNeverSupported", prearmCheck(true, true, true), result(true, true, true, Status::Ready)},
        {"prearmCheckPassed", prearmCheck(true, false, false), result(true, true, true, Status::Ready)},
        {"prearmCheckFailed", prearmCheck(false, false, true), result(false, false, false, Status::NotReady)},
        {"sensorHealthReady", sensorHealth(true, true), result(true, true, true, Status::Ready)},
        {"sensorHealthUnhealthyNeverBlocks", sensorHealth(false, true), result(true, true, true, Status::NotReady)},
        {"sensorHealthSetupIncompleteNeverBlocks", sensorHealth(true, false),
         result(true, true, true, Status::NotReady)},
        {"sensorHealthIgnoredWhenArmed", armed(sensorHealth(false, false)), result(true, true, true, Status::Ready)},
        {"checklistBlocksDisarmed", checklistBlocking(libEvents(all, all)),
         result(false, false, false, Status::NotReady)},
        {"checklistIgnoredWhenArmed", checklistBlocking(armed(libEvents(all, all))),
         result(true, true, true, Status::Ready)},
        {"checklistBlocksWhileWaiting", checklistBlocking(prearmCheck(true, true, false)),
         result(false, false, false, Status::NotReady)},
    };
}

void VehicleReadinessTest::_testEvaluate_data()
{
    QTest::addColumn<int>("caseIndex");

    const QList<EvaluateCase> cases = _evaluateCases();
    for (int i = 0; i < cases.count(); i++) {
        QTest::newRow(cases[i].name) << i;
    }
}

void VehicleReadinessTest::_testEvaluate()
{
    QFETCH(int, caseIndex);

    const EvaluateCase evaluateCase = _evaluateCases().at(caseIndex);
    VehicleReadiness::Result result = VehicleReadiness::_evaluate(evaluateCase.inputs);
    VehicleReadiness::_applyChecklist(result, evaluateCase.inputs);

    QCOMPARE(result.canArm, evaluateCase.expected.canArm);
    QCOMPARE(result.canTakeoff, evaluateCase.expected.canTakeoff);
    QCOMPARE(result.canStartMission, evaluateCase.expected.canStartMission);
    QCOMPARE(result.status, evaluateCase.expected.status);
}

// ---------------------------------------------------------------------------
// VehicleReadinessPX4Test
// ---------------------------------------------------------------------------

namespace {

/// Firmware readiness which always allows Arm, can veto Takeoff and always reports not ready
class OverridingVehicleReadiness : public VehicleReadiness
{
public:
    using VehicleReadiness::VehicleReadiness;

    bool takeoffAllowed = true;

protected:
    bool _canArm(bool /*genericCanArm*/) const override { return true; }

    bool _canTakeoff(bool genericCanTakeoff) const override { return genericCanTakeoff && takeoffAllowed; }

    Status _status(Status /*genericStatus*/) const override { return Status::NotReady; }
};

}  // namespace

void VehicleReadinessPX4Test::_testCurrentModeIntentionRefreshesReadiness()
{
    using namespace TestFixtures::HealthReport;

    Vehicle* const vehicle = this->vehicle();
    QVERIFY(vehicle);
    VehicleReadiness* const readiness = vehicle->readiness();
    QVERIFY(readiness);

    FirmwarePlugin* const firmwarePlugin = vehicle->firmwarePlugin();
    uint8_t baseMode = 0;
    uint32_t takeoffCustomMode = 0;
    uint32_t missionCustomMode = 0;
    QVERIFY(firmwarePlugin->setFlightMode(firmwarePlugin->takeOffFlightMode(), &baseMode, &takeoffCustomMode));
    QVERIFY(firmwarePlugin->setFlightMode(vehicle->missionFlightMode(), &baseMode, &missionCustomMode));
    const uint32_t heartbeatCustomMode = vehicle->effectiveCustomMode();

    QTemporaryDir metadataDir;
    QVERIFY(metadataDir.isValid());
    const QString metadataFile =
        writeMetadata(metadataDir.path(), heartbeatCustomMode, takeoffCustomMode, missionCustomMode);
    QVERIFY(!metadataFile.isEmpty());
    vehicle->setEventsMetadata(MAV_COMP_ID_AUTOPILOT1, metadataFile);

    // Only the takeoff mode group can arm, so the HEARTBEAT mode is not ready
    EventStream eventStream(static_cast<uint8_t>(mockLink()->vehicleId()));
    for (const mavlink_message_t& message : eventStream.report(modeGroupBit(TakeoffModeGroup), AllModeGroups)) {
        mockLink()->respondWithMavlinkMessage(message);
    }
    QVERIFY_TRUE_WAIT(readiness->status() == VehicleReadiness::Status::NotReady, TestTimeout::mediumMs());

    // CURRENT_MODE changes only the intended mode, so the displayed flight mode does not change
    const QString flightMode = vehicle->flightMode();
    mavlink_message_t currentMode{};
    (void) mavlink_msg_current_mode_pack(static_cast<uint8_t>(mockLink()->vehicleId()), MAV_COMP_ID_AUTOPILOT1,
                                         &currentMode, MAV_STANDARD_MODE_NON_STANDARD, heartbeatCustomMode,
                                         takeoffCustomMode);
    mockLink()->respondWithMavlinkMessage(currentMode);
    QVERIFY_TRUE_WAIT(vehicle->effectiveCustomMode() == takeoffCustomMode, TestTimeout::mediumMs());
    QCOMPARE(vehicle->flightMode(), flightMode);

    // Readiness must follow the intended mode as soon as CURRENT_MODE is handled, not at the next flight mode change
    QCOMPARE(readiness->status(), VehicleReadiness::Status::Ready);
    QVERIFY(readiness->canArm());
}

void VehicleReadinessPX4Test::_testUnmappedModeMakesReadinessUnknown()
{
    using namespace TestFixtures::HealthReport;

    Vehicle* const vehicle = this->vehicle();
    QVERIFY(vehicle);
    VehicleReadiness* const readiness = vehicle->readiness();
    QVERIFY(readiness);

    FirmwarePlugin* const firmwarePlugin = vehicle->firmwarePlugin();
    uint8_t baseMode = 0;
    uint32_t takeoffCustomMode = 0;
    uint32_t missionCustomMode = 0;
    QVERIFY(firmwarePlugin->setFlightMode(firmwarePlugin->takeOffFlightMode(), &baseMode, &takeoffCustomMode));
    QVERIFY(firmwarePlugin->setFlightMode(vehicle->missionFlightMode(), &baseMode, &missionCustomMode));
    const uint32_t heartbeatCustomMode = vehicle->effectiveCustomMode();

    QTemporaryDir metadataDir;
    QVERIFY(metadataDir.isValid());
    const QString metadataFile =
        writeMetadata(metadataDir.path(), heartbeatCustomMode, takeoffCustomMode, missionCustomMode);
    QVERIFY(!metadataFile.isEmpty());
    vehicle->setEventsMetadata(MAV_COMP_ID_AUTOPILOT1, metadataFile);

    EventStream eventStream(static_cast<uint8_t>(mockLink()->vehicleId()));
    for (const mavlink_message_t& message : eventStream.report(AllModeGroups, AllModeGroups)) {
        mockLink()->respondWithMavlinkMessage(message);
    }
    QVERIFY_TRUE_WAIT(readiness->status() == VehicleReadiness::Status::Ready, TestTimeout::mediumMs());

    // Switching to a mode the metadata does not cover must not keep the previous mode's readiness
    constexpr uint32_t unmappedCustomMode = 0xFFFF0000;
    expectLogMessage("MAVLink.LibEvents.HealthAndArmingCheckReport", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Flight mode group not set")));
    mavlink_message_t currentMode{};
    (void) mavlink_msg_current_mode_pack(static_cast<uint8_t>(mockLink()->vehicleId()), MAV_COMP_ID_AUTOPILOT1,
                                         &currentMode, MAV_STANDARD_MODE_NON_STANDARD, heartbeatCustomMode,
                                         unmappedCustomMode);
    mockLink()->respondWithMavlinkMessage(currentMode);
    QVERIFY_TRUE_WAIT(readiness->status() == VehicleReadiness::Status::Unknown, TestTimeout::mediumMs());
    verifyExpectedLogMessage();

    QVERIFY(!readiness->canArm());
    QVERIFY(readiness->canTakeoff());
    QVERIFY(readiness->canStartMission());
}

void VehicleReadinessPX4Test::_testMissingHealthReportFallsBack()
{
    using namespace TestFixtures::HealthReport;

    Vehicle* const vehicle = this->vehicle();
    QVERIFY(vehicle);
    VehicleReadiness* const readiness = vehicle->readiness();
    QVERIFY(readiness);

    QTemporaryDir metadataDir;
    QVERIFY(metadataDir.isValid());
    const QString metadataFile = writeMetadata(metadataDir.path(), vehicle->effectiveCustomMode(), 1, 2);
    QVERIFY(!metadataFile.isEmpty());
    vehicle->setEventsMetadata(MAV_COMP_ID_AUTOPILOT1, metadataFile);

    // The vehicle supports health reports but none arrives, so readiness waits
    readiness->update();
    QCOMPARE(readiness->status(), VehicleReadiness::Status::Unknown);
    QVERIFY(!readiness->canArm());

    // Rather than waiting forever it falls back to the other sources, which allow every action on MockLink
    expectLogMessage("Vehicle.VehicleReadiness", QtWarningMsg,
                     QRegularExpression(QStringLiteral("No health and arming check report received")));
    readiness->_healthReportWaitTimer.start(0);
    QVERIFY_TRUE_WAIT(readiness->canArm(), TestTimeout::mediumMs());
    verifyExpectedLogMessage();
    QVERIFY(readiness->canTakeoff());
    QVERIFY(readiness->canStartMission());

    // A report arriving later is still used
    EventStream eventStream(static_cast<uint8_t>(mockLink()->vehicleId()));
    for (const mavlink_message_t& message : eventStream.report(modeGroupBit(TakeoffModeGroup), AllModeGroups)) {
        mockLink()->respondWithMavlinkMessage(message);
    }
    QVERIFY_TRUE_WAIT(!readiness->canArm(), TestTimeout::mediumMs());
}

void VehicleReadinessPX4Test::_testWaitsForHealthReportDuringInitialConnect()
{
    _disconnectMockLink();

    // vehicleAdded is emitted from the Vehicle creation call stack, before any health report can arrive
    std::optional<VehicleReadiness::Result> resultWhenAdded;
    const QMetaObject::Connection addedConnection = connect(
        MultiVehicleManager::instance(), &MultiVehicleManager::vehicleAdded, this,
        [&resultWhenAdded](Vehicle* vehicle) {
            const VehicleReadiness* const readiness = vehicle->readiness();
            if (readiness) {
                resultWhenAdded = VehicleReadiness::Result{.canArm = readiness->canArm(),
                                                           .canTakeoff = readiness->canTakeoff(),
                                                           .canStartMission = readiness->canStartMission(),
                                                           .status = readiness->status()};
            }
        },
        Qt::DirectConnection);
    const auto disconnectAdded = qScopeGuard([addedConnection]() { disconnect(addedConnection); });

    _connectMockLink(MAV_AUTOPILOT_PX4);
    if (QTest::currentTestFailed()) {
        return;
    }

    QVERIFY(resultWhenAdded.has_value());
    QVERIFY(!resultWhenAdded->canArm);
    QVERIFY(!resultWhenAdded->canTakeoff);
    QVERIFY(!resultWhenAdded->canStartMission);
    QCOMPARE(resultWhenAdded->status, VehicleReadiness::Status::Unknown);
}

void VehicleReadinessPX4Test::_testFirmwareOverridesActions()
{
    Vehicle* const vehicle = this->vehicle();
    QVERIFY(vehicle);

    // MockLink sends no health reports, so the generic answer allows every action
    OverridingVehicleReadiness readiness(vehicle);
    readiness.update();
    QVERIFY(readiness.canArm());
    QVERIFY(readiness.canTakeoff());
    QVERIFY(readiness.canStartMission());
    QCOMPARE(readiness.status(), VehicleReadiness::Status::NotReady);

    readiness.takeoffAllowed = false;
    readiness.update();
    QVERIFY(!readiness.canTakeoff());
    QVERIFY(readiness.canStartMission());

    // An enforced checklist still blocks actions the firmware allows
    AppSettings* const appSettings = SettingsManager::instance()->appSettings();
    const QVariant useChecklist = appSettings->useChecklist()->rawValue();
    const QVariant enforceChecklist = appSettings->enforceChecklist()->rawValue();
    const auto restoreSettings = qScopeGuard([=] {
        appSettings->useChecklist()->setRawValue(useChecklist);
        appSettings->enforceChecklist()->setRawValue(enforceChecklist);
    });
    vehicle->setCheckListState(Vehicle::CheckListNotSetup);
    appSettings->useChecklist()->setRawValue(true);
    appSettings->enforceChecklist()->setRawValue(true);
    QVERIFY(!readiness.canArm());
}

void VehicleReadinessPX4Test::_testHealthReportReadiness()
{
    using namespace TestFixtures::HealthReport;

    Vehicle* const vehicle = this->vehicle();
    QVERIFY(vehicle);
    VehicleReadiness* const readiness = vehicle->readiness();
    QVERIFY(readiness);

    // MockLink sends no health reports, so once connected readiness falls back to sensor health
    QVERIFY(qobject_cast<PX4VehicleReadiness*>(readiness));
    QVERIFY(readiness->canArm());

    FirmwarePlugin* const firmwarePlugin = vehicle->firmwarePlugin();
    uint8_t baseMode = 0;
    uint32_t takeoffCustomMode = 0;
    uint32_t missionCustomMode = 0;
    QVERIFY(firmwarePlugin->setFlightMode(firmwarePlugin->takeOffFlightMode(), &baseMode, &takeoffCustomMode));
    QVERIFY(firmwarePlugin->setFlightMode(vehicle->missionFlightMode(), &baseMode, &missionCustomMode));
    const uint32_t currentCustomMode = vehicle->effectiveCustomMode();
    QVERIFY((currentCustomMode != takeoffCustomMode) && (currentCustomMode != missionCustomMode));

    QTemporaryDir metadataDir;
    QVERIFY(metadataDir.isValid());
    const QString metadataFile =
        writeMetadata(metadataDir.path(), currentCustomMode, takeoffCustomMode, missionCustomMode);
    QVERIFY(!metadataFile.isEmpty());
    vehicle->setEventsMetadata(MAV_COMP_ID_AUTOPILOT1, metadataFile);
    QVERIFY(vehicle->healthAndArmingChecksSupported(MAV_COMP_ID_AUTOPILOT1));

    EventStream eventStream(static_cast<uint8_t>(mockLink()->vehicleId()));
    const auto sendReport = [this, &eventStream](uint32_t canArmModeGroups, uint32_t canRunModeGroups) {
        for (const mavlink_message_t& message : eventStream.report(canArmModeGroups, canRunModeGroups)) {
            mockLink()->respondWithMavlinkMessage(message);
        }
    };

    // Disarmed: each action is gated by arming in its own mode
    sendReport(modeGroupBit(ManualModeGroup) | modeGroupBit(MissionModeGroup), AllModeGroups);
    QVERIFY_TRUE_WAIT(!readiness->canTakeoff(), TestTimeout::mediumMs());
    QVERIFY(readiness->canArm());
    QVERIFY(readiness->canStartMission());
    QCOMPARE(readiness->status(), VehicleReadiness::Status::Ready);

    // Armed: each action is gated by switching into its mode
    mockLink()->setArmed(true);
    QVERIFY_TRUE_WAIT(vehicle->armed(), TestTimeout::mediumMs());
    QVERIFY(readiness->canTakeoff());

    sendReport(AllModeGroups, modeGroupBit(ManualModeGroup) | modeGroupBit(TakeoffModeGroup));
    QVERIFY_TRUE_WAIT(!readiness->canStartMission(), TestTimeout::mediumMs());
    QVERIFY(readiness->canTakeoff());

    mockLink()->setArmed(false);
    QVERIFY_TRUE_WAIT(!vehicle->armed(), TestTimeout::mediumMs());
}

// ---------------------------------------------------------------------------
// VehicleReadinessAPMTest
// ---------------------------------------------------------------------------

void VehicleReadinessAPMTest::_testPrearmCheckReadiness()
{
    Vehicle* const vehicle = this->vehicle();
    QVERIFY(vehicle);
    VehicleReadiness* const readiness = vehicle->readiness();
    QVERIFY(readiness);

    QVERIFY(qobject_cast<APMVehicleReadiness*>(readiness));
    QVERIFY(!vehicle->readyToFlyAvailable());
    QVERIFY(readiness->canArm());

    mockLink()->setPrearmCheck(true, false);
    QVERIFY_TRUE_WAIT(vehicle->readyToFlyAvailable(), TestTimeout::mediumMs());
    QVERIFY(!readiness->canArm());
    QVERIFY(!readiness->canTakeoff());
    QVERIFY(!readiness->canStartMission());
    QCOMPARE(readiness->status(), VehicleReadiness::Status::NotReady);

    // Disabling arming checks must not leave the last failed prearm check behind
    mockLink()->setPrearmCheck(false, false);
    QVERIFY_TRUE_WAIT(!vehicle->readyToFlyAvailable(), TestTimeout::mediumMs());
    QVERIFY(readiness->canArm());

    mockLink()->setPrearmCheck(true, true);
    QVERIFY_TRUE_WAIT(vehicle->readyToFlyAvailable() && vehicle->readyToFly(), TestTimeout::mediumMs());
    QVERIFY(readiness->canArm());
    QVERIFY(readiness->canTakeoff());
    QVERIFY(readiness->canStartMission());
    QCOMPARE(readiness->status(), VehicleReadiness::Status::Ready);

    // Disabling arming checks must not leave the last passed prearm check behind either
    mockLink()->setPrearmCheck(false, false);
    QVERIFY_TRUE_WAIT(!vehicle->readyToFlyAvailable(), TestTimeout::mediumMs());
    QVERIFY(!vehicle->readyToFly());
}

void VehicleReadinessAPMTest::_testHealthReportUsesArduPilotActionModes()
{
    using namespace TestFixtures::HealthReport;

    Vehicle* const vehicle = this->vehicle();
    QVERIFY(vehicle);
    QVERIFY(vehicle->multiRotor());
    VehicleReadiness* const readiness = vehicle->readiness();
    QVERIFY(readiness);

    const auto* const firmwarePlugin = qobject_cast<const APMFirmwarePlugin*>(vehicle->firmwarePlugin());
    QVERIFY(firmwarePlugin);
    uint8_t baseMode = 0;
    uint32_t guidedCustomMode = 0;
    uint32_t autoCustomMode = 0;
    QVERIFY(firmwarePlugin->setFlightMode(firmwarePlugin->guidedFlightMode(), &baseMode, &guidedCustomMode));
    QVERIFY(firmwarePlugin->setFlightMode(vehicle->missionFlightMode(), &baseMode, &autoCustomMode));
    const uint32_t currentCustomMode = vehicle->effectiveCustomMode();
    QVERIFY((currentCustomMode != guidedCustomMode) && (currentCustomMode != autoCustomMode));

    // The fixture's takeoff and mission groups stand in for Guided and Auto
    constexpr int guidedModeGroup = TakeoffModeGroup;
    constexpr int autoModeGroup = MissionModeGroup;
    QTemporaryDir metadataDir;
    QVERIFY(metadataDir.isValid());
    const QString metadataFile = writeMetadata(metadataDir.path(), currentCustomMode, guidedCustomMode, autoCustomMode);
    QVERIFY(!metadataFile.isEmpty());
    vehicle->setEventsMetadata(MAV_COMP_ID_AUTOPILOT1, metadataFile);

    EventStream eventStream(static_cast<uint8_t>(mockLink()->vehicleId()));
    const auto sendReport = [this, &eventStream](uint32_t canArmModeGroups, uint32_t canRunModeGroups) {
        for (const mavlink_message_t& message : eventStream.report(canArmModeGroups, canRunModeGroups)) {
            mockLink()->respondWithMavlinkMessage(message);
        }
    };

    // Copter takeoff and start mission both arm in Guided, and start mission then needs Auto to run
    sendReport(modeGroupBit(ManualModeGroup) | modeGroupBit(guidedModeGroup), AllModeGroups);
    QVERIFY_TRUE_WAIT(readiness->canTakeoff() && readiness->canStartMission(), TestTimeout::mediumMs());

    sendReport(modeGroupBit(ManualModeGroup) | modeGroupBit(guidedModeGroup),
               AllModeGroups & ~modeGroupBit(autoModeGroup));
    QVERIFY_TRUE_WAIT(!readiness->canStartMission(), TestTimeout::mediumMs());
    QVERIFY(readiness->canTakeoff());

    sendReport(modeGroupBit(ManualModeGroup) | modeGroupBit(autoModeGroup), AllModeGroups);
    QVERIFY_TRUE_WAIT(!readiness->canTakeoff(), TestTimeout::mediumMs());
    QVERIFY(!readiness->canStartMission());
    QVERIFY(readiness->canArm());
}

void VehicleReadinessAPMTest::_testChecklistBlocksArming()
{
    Vehicle* const vehicle = this->vehicle();
    QVERIFY(vehicle);
    VehicleReadiness* const readiness = vehicle->readiness();
    QVERIFY(readiness);
    QVERIFY(readiness->canArm());

    AppSettings* const appSettings = SettingsManager::instance()->appSettings();
    const QVariant useChecklist = appSettings->useChecklist()->rawValue();
    const QVariant enforceChecklist = appSettings->enforceChecklist()->rawValue();

    vehicle->setCheckListState(Vehicle::CheckListNotSetup);
    appSettings->useChecklist()->setRawValue(true);
    appSettings->enforceChecklist()->setRawValue(true);
    QVERIFY(!readiness->canArm());
    QCOMPARE(readiness->status(), VehicleReadiness::Status::NotReady);

    vehicle->setCheckListState(Vehicle::CheckListPassed);
    QVERIFY(readiness->canArm());

    appSettings->useChecklist()->setRawValue(useChecklist);
    appSettings->enforceChecklist()->setRawValue(enforceChecklist);
}

UT_REGISTER_TEST(VehicleReadinessTest, TestLabel::Unit, TestLabel::Vehicle)
UT_REGISTER_TEST(VehicleReadinessPX4Test, TestLabel::Integration, TestLabel::Vehicle)
UT_REGISTER_TEST(VehicleReadinessAPMTest, TestLabel::Integration, TestLabel::Vehicle)
