#include "SimpleMissionItemTest.h"

#include "AppSettings.h"
#include "CameraSection.h"
#include "MissionCommandTree.h"
#include "MissionCommandUIInfo.h"
#include "MultiSignalSpy.h"
#include "PlanMasterController.h"
#include "SettingsManager.h"
#include "SimpleMissionItem.h"
#include "SpeedSection.h"
#include "Vehicle.h"


void SimpleMissionItemTest::init()
{
    VisualMissionItemTest::init();
    MissionItem missionItem(1,           // sequence number
                            MAV_CMD_NAV_WAYPOINT, MAV_FRAME_GLOBAL_RELATIVE_ALT,
                            10.1234567,  // param 1-7
                            20.1234567, 30.1234567, 40.1234567, 50.1234567, 60.1234567, 70.1234567,
                            true,        // autoContinue
                            false);      // isCurrentItem
    _simpleItem = new SimpleMissionItem(planController(), false /* flyView */, missionItem);
    // It's important to check that the right signals are emitted at the right time since that drives ui change.
    // It's also important to check that things are not being over-signalled when they should not be.
    _spySimpleItem = new MultiSignalSpy();
    QVERIFY(_spySimpleItem->init(_simpleItem));
    VisualMissionItemTest::_createSpy(_simpleItem, &_spyVisualItem);
}

void SimpleMissionItemTest::cleanup()
{
    delete _spySimpleItem;
    delete _spyVisualItem;
    _spySimpleItem = nullptr;
    _spyVisualItem = nullptr;
    VisualMissionItemTest::cleanup();
    // _simpleItem is deleted when planController() is deleted
    _simpleItem = nullptr;
}

bool SimpleMissionItemTest::_classMatch(QGCMAVLink::VehicleClass_t vehicleClass, QGCMAVLink::VehicleClass_t testClass)
{
    return vehicleClass == QGCMAVLink::VehicleClassGeneric || vehicleClass == testClass;
}

void SimpleMissionItemTest::_testEditorFactsWorker(QGCMAVLink::VehicleClass_t vehicleClass,
                                                   QGCMAVLink::VehicleClass_t vtolMode)
{
    qDebug() << "vehicleClass:vtolMode" << QGCMAVLink::vehicleClassToUserVisibleString(vehicleClass)
             << QGCMAVLink::vehicleClassToUserVisibleString(vtolMode);

    typedef struct
    {
        MAV_CMD command;
        MAV_FRAME frame;
        double altValue;
        QGroundControlQmlGlobal::AltMode altMode;
    } TestCase_t;

    TestCase_t testCases[] = {
        {MAV_CMD_NAV_WAYPOINT, MAV_FRAME_GLOBAL_RELATIVE_ALT, 70.1234567,
         QGroundControlQmlGlobal::AltitudeModeRelative},
        {MAV_CMD_NAV_LOITER_UNLIM, MAV_FRAME_GLOBAL, 70.1234567, QGroundControlQmlGlobal::AltitudeModeAbsolute},
        {MAV_CMD_NAV_LOITER_TURNS, MAV_FRAME_GLOBAL_RELATIVE_ALT, 70.1234567,
         QGroundControlQmlGlobal::AltitudeModeRelative},
        {MAV_CMD_NAV_LOITER_TIME, MAV_FRAME_GLOBAL, 70.1234567, QGroundControlQmlGlobal::AltitudeModeAbsolute},
        {MAV_CMD_NAV_LAND, MAV_FRAME_GLOBAL_RELATIVE_ALT, 70.1234567, QGroundControlQmlGlobal::AltitudeModeRelative},
        {MAV_CMD_NAV_TAKEOFF, MAV_FRAME_GLOBAL, 70.1234567, QGroundControlQmlGlobal::AltitudeModeAbsolute},
        {MAV_CMD_DO_JUMP, MAV_FRAME_MISSION, qQNaN(), QGroundControlQmlGlobal::AltitudeModeRelative},
    };
    PlanMasterController planController(MAV_AUTOPILOT_PX4, QGCMAVLink::vehicleClassToMavType(vehicleClass));
    QGCMAVLink::VehicleClass_t commandVehicleClass =
        vtolMode == QGCMAVLink::VehicleClassGeneric ? vehicleClass : vtolMode;
    QScopedPointer<Vehicle> vehicle(new Vehicle(MAV_AUTOPILOT_PX4, QGCMAVLink::vehicleClassToMavType(vehicleClass)));
    for (size_t testIdx = 0; testIdx < sizeof(testCases) / sizeof(testCases[0]); testIdx++) {
        auto& testCase = testCases[testIdx];
        auto* missionCommandTree = MissionCommandTree::instance();
        const MissionCommandUIInfo* uiInfo =
            missionCommandTree->getUIInfo(vehicle.data(), commandVehicleClass, testCase.command);
        qDebug() << "Command" << missionCommandTree->rawName(testCase.command);
        typedef QPair<int, QString> FactInfoPair_t;
        QList<FactInfoPair_t> cExpectedTextFieldInfo;
        QList<FactInfoPair_t> cExpectedComboBoxInfo;
        QList<FactInfoPair_t> cExpectedNaNFieldInfo;
        QList<FactInfoPair_t> cExpectedAdvancedTextFieldInfo;
        QList<FactInfoPair_t> cExpectedAdvancedComboBoxInfo;
        QList<FactInfoPair_t> cExpectedAdvancedNaNFieldInfo;
        for (int paramIndex = 1; paramIndex <= 7; paramIndex++) {
            bool showUI = false;
            auto paramInfo = uiInfo->getParamInfo(paramIndex, showUI);
            if (paramInfo && showUI) {
                if (paramInfo->advanced()) {
                    if (paramInfo->nanUnchanged()) {
                        cExpectedAdvancedNaNFieldInfo.append(FactInfoPair_t(paramIndex, paramInfo->label()));
                    } else if (paramInfo->enumStrings().count() > 0) {
                        cExpectedAdvancedComboBoxInfo.append(FactInfoPair_t(paramIndex, paramInfo->label()));
                    } else {
                        cExpectedAdvancedTextFieldInfo.append(FactInfoPair_t(paramIndex, paramInfo->label()));
                    }
                } else {
                    if (paramInfo->nanUnchanged()) {
                        cExpectedNaNFieldInfo.append(FactInfoPair_t(paramIndex, paramInfo->label()));
                    } else if (paramInfo->enumStrings().count() > 0) {
                        cExpectedComboBoxInfo.append(FactInfoPair_t(paramIndex, paramInfo->label()));
                    } else {
                        cExpectedTextFieldInfo.append(FactInfoPair_t(paramIndex, paramInfo->label()));
                    }
                }
            }
        }
        MissionItem missionItem(1,           // sequence number
                                testCase.command, testCase.frame,
                                10.1234567,  // param 1-7
                                20.1234567, 30.1234567, 40.1234567, 50.1234567, 60.1234567, 70.1234567,
                                true,        // autoContinue
                                false);      // isCurrentItem
        SimpleMissionItem simpleMissionItem(&planController, false /* flyView */, missionItem);
        MissionController::MissionFlightStatus_t missionFlightStatus;
        missionFlightStatus.vtolMode = vtolMode;
        missionFlightStatus.vehicleSpeed = 10;
        missionFlightStatus.gimbalYaw = qQNaN();
        missionFlightStatus.gimbalPitch = qQNaN();
        simpleMissionItem.setMissionFlightStatus(missionFlightStatus);
        QCOMPARE(simpleMissionItem.textFieldFacts()->count(), cExpectedTextFieldInfo.count());
        for (int j = 0; j < simpleMissionItem.textFieldFacts()->count(); j++) {
            Fact* fact = qobject_cast<Fact*>(simpleMissionItem.textFieldFacts()->get(j));
            qDebug() << "textFieldFact" << fact->name();
            QCOMPARE(fact->name(), cExpectedTextFieldInfo[j].second);
            QCOMPARE(fact->rawValue().toDouble(), (cExpectedTextFieldInfo[j].first * 10.0) + 0.1234567);
        }
        QCOMPARE(simpleMissionItem.comboboxFacts()->count(), cExpectedComboBoxInfo.count());
        for (int j = 0; j < simpleMissionItem.comboboxFacts()->count(); j++) {
            Fact* fact = qobject_cast<Fact*>(simpleMissionItem.comboboxFacts()->get(j));
            qDebug() << "comboBoxFact" << fact->name();
            QCOMPARE(fact->name(), cExpectedComboBoxInfo[j].second);
            QCOMPARE(fact->rawValue().toDouble(), (cExpectedComboBoxInfo[j].first * 10.0) + 0.1234567);
        }
        QCOMPARE(simpleMissionItem.nanFacts()->count(), cExpectedNaNFieldInfo.count());
        for (int j = 0; j < simpleMissionItem.nanFacts()->count(); j++) {
            Fact* fact = qobject_cast<Fact*>(simpleMissionItem.nanFacts()->get(j));
            qDebug() << "nanFieldFact" << fact->name();
            QCOMPARE(fact->name(), cExpectedNaNFieldInfo[j].second);
            QCOMPARE(fact->rawValue().toDouble(), qQNaN());
        }
        QCOMPARE(simpleMissionItem.textFieldFactsAdvanced()->count(), cExpectedAdvancedTextFieldInfo.count());
        for (int j = 0; j < simpleMissionItem.textFieldFactsAdvanced()->count(); j++) {
            Fact* fact = qobject_cast<Fact*>(simpleMissionItem.textFieldFactsAdvanced()->get(j));
            qDebug() << "advancedTextFieldFact" << fact->name();
            QCOMPARE(fact->name(), cExpectedAdvancedTextFieldInfo[j].second);
            QCOMPARE(fact->rawValue().toDouble(), (cExpectedAdvancedTextFieldInfo[j].first * 10.0) + 0.1234567);
        }
        QCOMPARE(simpleMissionItem.comboboxFactsAdvanced()->count(), cExpectedAdvancedComboBoxInfo.count());
        for (int j = 0; j < simpleMissionItem.comboboxFactsAdvanced()->count(); j++) {
            Fact* fact = qobject_cast<Fact*>(simpleMissionItem.comboboxFactsAdvanced()->get(j));
            qDebug() << "advancedComboBoxFact" << fact->name();
            QCOMPARE(fact->name(), cExpectedAdvancedComboBoxInfo[j].second);
            QCOMPARE(fact->rawValue().toDouble(), (cExpectedAdvancedComboBoxInfo[j].first * 10.0) + 0.1234567);
        }
        QCOMPARE(simpleMissionItem.nanFactsAdvanced()->count(), cExpectedAdvancedNaNFieldInfo.count());
        for (int j = 0; j < simpleMissionItem.nanFactsAdvanced()->count(); j++) {
            Fact* fact = qobject_cast<Fact*>(simpleMissionItem.nanFactsAdvanced()->get(j));
            qDebug() << "advancedNaNFieldFact" << fact->name();
            QCOMPARE(fact->name(), cExpectedAdvancedNaNFieldInfo[j].second);
            QCOMPARE(fact->rawValue().toDouble(), (cExpectedAdvancedNaNFieldInfo[j].first * 10.0) + 0.1234567);
        }
        if (!qIsNaN(testCase.altValue)) {
            QCOMPARE(simpleMissionItem.altitudeMode(), testCase.altMode);
            QCOMPARE(simpleMissionItem.altitude()->rawValue().toDouble(), testCase.altValue);
        }
    }
}

void SimpleMissionItemTest::_testEditorFacts()
{
    _testEditorFactsWorker(QGCMAVLink::VehicleClassMultiRotor, QGCMAVLink::VehicleClassGeneric);
    _testEditorFactsWorker(QGCMAVLink::VehicleClassFixedWing, QGCMAVLink::VehicleClassGeneric);
    _testEditorFactsWorker(QGCMAVLink::VehicleClassVTOL, QGCMAVLink::VehicleClassMultiRotor);
    _testEditorFactsWorker(QGCMAVLink::VehicleClassVTOL, QGCMAVLink::VehicleClassFixedWing);
}

void SimpleMissionItemTest::_testDefaultValues()
{
    SimpleMissionItem item(planController(), false /* flyView */, false /* forLoad */);
    item.missionItem().setCommand(MAV_CMD_NAV_WAYPOINT);
    item.missionItem().setFrame(MAV_FRAME_GLOBAL_RELATIVE_ALT);
    QCOMPARE(item.missionItem().param7(),
             SettingsManager::instance()->appSettings()->defaultMissionItemAltitude()->rawValue().toDouble());
}

void SimpleMissionItemTest::_testSignals()
{
    MissionItem& missionItem = _simpleItem->missionItem();
    // Check that changing to the same coordinate does not signal
    _simpleItem->setCoordinate(QGeoCoordinate(missionItem.param5(), missionItem.param6(), missionItem.param7()));
    QVERIFY(_spyVisualItem->noneEmitted());
    QVERIFY(_spySimpleItem->noneEmitted());
    // Check coordinateChanged signalling. Calling setCoordinate should trigger:
    //      coordinateChanged, exitCoordinateChanged, dirtyChanged
    //      amslEntryAltChanged, amslExitAltChanged, terrainAltitudeChanged (altitude depends on position)
    // Check that actually changing coordinate signals correctly
    _simpleItem->setCoordinate(QGeoCoordinate(missionItem.param5() + 1, missionItem.param6(), missionItem.param7()));
    QVERIFY(_spyVisualItem->onlyEmittedOnceByMask(
        _spyVisualItem->mask("coordinateChanged", "exitCoordinateChanged", "dirtyChanged", "amslEntryAltChanged",
                             "amslExitAltChanged", "terrainAltitudeChanged")));
    _spyVisualItem->clearAllSignals();
    _spySimpleItem->clearAllSignals();
    _simpleItem->setCoordinate(QGeoCoordinate(missionItem.param5(), missionItem.param6() + 1, missionItem.param7()));
    QVERIFY(_spyVisualItem->onlyEmittedOnceByMask(
        _spyVisualItem->mask("coordinateChanged", "exitCoordinateChanged", "dirtyChanged", "amslEntryAltChanged",
                             "amslExitAltChanged", "terrainAltitudeChanged")));
    _spyVisualItem->clearAllSignals();
    _spySimpleItem->clearAllSignals();
    // Altitude in coordinate is not used in setCoordinate
    _simpleItem->setCoordinate(QGeoCoordinate(missionItem.param5(), missionItem.param6(), missionItem.param7() + 1));
    QVERIFY(_spyVisualItem->noneEmitted());
    QVERIFY(_spySimpleItem->noneEmitted());
    _spyVisualItem->clearAllSignals();
    // Check dirtyChanged signalling
    missionItem.setParam1(missionItem.param1());
    QVERIFY(_spyVisualItem->noneEmitted());
    missionItem.setParam2(missionItem.param2());
    QVERIFY(_spyVisualItem->noneEmitted());
    missionItem.setParam3(missionItem.param3());
    QVERIFY(_spyVisualItem->noneEmitted());
    missionItem.setParam4(missionItem.param4());
    QVERIFY(_spyVisualItem->noneEmitted());
    // Changing params should emit dirtyChanged (may also emit other related signals)
    missionItem.setParam1(missionItem.param1() + 1);
    QVERIFY(_spyVisualItem->emitted("dirtyChanged"));
    _spyVisualItem->clearAllSignals();
    missionItem.setParam1(missionItem.param2() + 1);
    QVERIFY(_spyVisualItem->emitted("dirtyChanged"));
    _spyVisualItem->clearAllSignals();
    missionItem.setParam1(missionItem.param3() + 1);
    QVERIFY(_spyVisualItem->emitted("dirtyChanged"));
    _spyVisualItem->clearAllSignals();
    missionItem.setParam1(missionItem.param4() + 1);
    QVERIFY(_spyVisualItem->emitted("dirtyChanged"));
    _spyVisualItem->clearAllSignals();
    // Changing altitude mode should emit these signals (may also emit other related signals)
    _simpleItem->setAltitudeMode(_simpleItem->altitudeMode() == QGroundControlQmlGlobal::AltitudeModeRelative
                                     ? QGroundControlQmlGlobal::AltitudeModeAbsolute
                                     : QGroundControlQmlGlobal::AltitudeModeRelative);
    QVERIFY(_spySimpleItem->emittedByMask(
        _spySimpleItem->mask("dirtyChanged", "friendlyEditAllowedChanged", "altitudeModeChanged")));
    _spySimpleItem->clearAllSignals();
    _spyVisualItem->clearAllSignals();
    // Check commandChanged signalling. Call setCommand should trigger:
    //      commandChanged
    //      commandNameChanged
    //      dirtyChanged
    //      coordinateChanged - since altitude will be set back to default
    _simpleItem->setCommand(MAV_CMD_NAV_WAYPOINT);
    QVERIFY(_spyVisualItem->noneEmitted());
    QVERIFY(_spySimpleItem->noneEmitted());
    _simpleItem->setCommand(MAV_CMD_NAV_LOITER_TIME);
    QVERIFY(_spySimpleItem->emitted("commandChanged"));
    QVERIFY(_spyVisualItem->emittedByMask(_spyVisualItem->mask("commandNameChanged", "dirtyChanged")));
}

void SimpleMissionItemTest::_testCameraSectionDirty()
{
    CameraSection* cameraSection = _simpleItem->cameraSection();
    QVERIFY(!cameraSection->dirty());
    QVERIFY(!_simpleItem->dirty());
    // Dirtying the camera section should also dirty the item
    cameraSection->setDirty(true);
    QVERIFY(_simpleItem->dirty());
    // Clearing the dirty bit from the item should also clear the dirty bit on the camera section
    _simpleItem->setDirty(false);
    QVERIFY(!cameraSection->dirty());
}

void SimpleMissionItemTest::_testSpeedSectionDirty()
{
    SpeedSection* speedSection = _simpleItem->speedSection();
    QVERIFY(!speedSection->dirty());
    QVERIFY(!_simpleItem->dirty());
    // Dirtying the speed section should also dirty the item
    speedSection->setDirty(true);
    QVERIFY(_simpleItem->dirty());
    // Clearing the dirty bit from the item should also clear the dirty bit on the camera section
    _simpleItem->setDirty(false);
    QVERIFY(!speedSection->dirty());
}

void SimpleMissionItemTest::_testCameraSection()
{
    // No gimbal yaw to start with
    QVERIFY(qIsNaN(_simpleItem->specifiedGimbalYaw()));
    QVERIFY(qIsNaN(_simpleItem->missionGimbalYaw()));
    QCOMPARE(_simpleItem->dirty(), false);
    double gimbalYaw = 10.1234;
    _simpleItem->cameraSection()->setSpecifyGimbal(true);
    _simpleItem->cameraSection()->gimbalYaw()->setRawValue(gimbalYaw);
    QCOMPARE(_simpleItem->specifiedGimbalYaw(), gimbalYaw);
    QVERIFY(qIsNaN(_simpleItem->missionGimbalYaw()));
    QVERIFY(_spyVisualItem->emitted("specifiedGimbalYawChanged"));
    QCOMPARE(_simpleItem->dirty(), true);
}

void SimpleMissionItemTest::_testSpeedSection()
{
    // No flight speed
    QVERIFY(qIsNaN(_simpleItem->specifiedFlightSpeed()));
    QCOMPARE(_simpleItem->dirty(), false);
    double flightSpeed = 10.1234;
    _simpleItem->speedSection()->setSpecifyFlightSpeed(true);
    _simpleItem->speedSection()->flightSpeed()->setRawValue(flightSpeed);
    QCOMPARE(_simpleItem->specifiedFlightSpeed(), flightSpeed);
    QVERIFY(_spyVisualItem->emitted("specifiedFlightSpeedChanged"));
    QCOMPARE(_simpleItem->dirty(), true);
}

void SimpleMissionItemTest::_testAltitudePropogation()
{
    // Make sure that changes to altitude propogate to param 7 of the mission item
    _simpleItem->setAltitudeMode(QGroundControlQmlGlobal::AltitudeModeRelative);
    _simpleItem->altitude()->setRawValue(_simpleItem->altitude()->rawValue().toDouble() + 1);
    QCOMPARE(_simpleItem->altitude()->rawValue().toDouble(), _simpleItem->missionItem().param7());
    QCOMPARE(_simpleItem->missionItem().frame(), MAV_FRAME_GLOBAL_RELATIVE_ALT);
    _simpleItem->setAltitudeMode(QGroundControlQmlGlobal::AltitudeModeAbsolute);
    _simpleItem->altitude()->setRawValue(_simpleItem->altitude()->rawValue().toDouble() + 1);
    QCOMPARE(_simpleItem->altitude()->rawValue().toDouble(), _simpleItem->missionItem().param7());
    QCOMPARE(_simpleItem->missionItem().frame(), MAV_FRAME_GLOBAL);
}

UT_REGISTER_TEST(SimpleMissionItemTest, TestLabel::Unit, TestLabel::MissionManager)
