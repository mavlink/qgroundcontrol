#include "PlanMasterControllerTest.h"

#include "AppSettings.h"
#include "SurveyPlanCreator.h"
#include "MissionManager.h"
#include "MultiSignalSpy.h"
#include "MultiVehicleManager.h"
#include "PlanMasterController.h"
#include "SettingsManager.h"
#include "Vehicle.h"

#include <QtCore/QDateTime>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QTemporaryDir>
#include <QtTest/QSignalSpy>

void PlanMasterControllerTest::init()
{
    UnitTest::init();
    MultiVehicleManager::instance()->init();
    _masterController = new PlanMasterController(this);
    _masterController->setFlyView(false);
    _masterController->start();
}

void PlanMasterControllerTest::cleanup()
{
    delete _masterController;
    _masterController = nullptr;
    _disconnectMockLink();
    UnitTest::cleanup();
}

void PlanMasterControllerTest::_testMissionPlannerFileLoad()
{
    _masterController->loadFromFile(":/unittest/MissionPlanner.waypoints");
    QCOMPARE(_masterController->missionController()->visualItems()->count(), 6);
}

void PlanMasterControllerTest::_testActiveVehicleChanged()
{
    // There was a defect where the PlanMasterController would, upon a new active vehicle,
    // overzelously disconnect all subscribers interested in the outgoing active vechicle.
    Vehicle* outgoingManagerVehicle = _masterController->managerVehicle();
    // spyMissionManager emulates a subscriber that should not be disconnected when
    // the active vehicle changes
    MultiSignalSpy spyMissionManager;
    spyMissionManager.init(outgoingManagerVehicle->missionManager());
    MultiSignalSpy spyMasterController;
    spyMasterController.init(_masterController);
    // Since MissionManager works with actual vehicles (which we don't have in the test cycle)
    // we have to be a bit creative emulating a signal emitted by a MissionManager.
    emit outgoingManagerVehicle->missionManager()->error(0, "");
    auto missionManagerErrorSignalMask = spyMissionManager.mask("error");
    QVERIFY(spyMissionManager.onlyEmittedOnceByMask(missionManagerErrorSignalMask));
    spyMissionManager.clearSignal("error");
    QVERIFY(spyMissionManager.noneEmitted());

    _connectMockLink(MAV_AUTOPILOT_PX4);
    auto masterControllerMgrVehicleChanged = spyMasterController.mask("managerVehicleChanged");
    QVERIFY(spyMasterController.emittedOnceByMask(masterControllerMgrVehicleChanged));

    emit outgoingManagerVehicle->missionManager()->error(0, "");
    // This signal was affected by the defect - it wouldn't reach the subscriber. Here
    // we make sure it does.
    QVERIFY(spyMissionManager.onlyEmittedOnceByMask(missionManagerErrorSignalMask));
}

void PlanMasterControllerTest::_testDirtyFlagsMatrix_data()
{
    // Dirty-state transition matrix ("unchanged" means preserve prior value):
    //
    // | State \ Action | Upload OK | Clear | SaveDirty=true | Load plan | Save file OK | Clear save-dirty | Download w/ items | Download empty |
    // |----------------|-----------|-------|----------------|-----------|--------------|------------------|-------------------|----------------|
    // | dirtyForSave   | unchanged | false | true           | false     | false        | false            | true              | false          |
    // | dirtyForUpload | false     | false | true           | true      | unchanged    | unchanged        | false             | false          |

    // Data columns:
    //  - scenario: DirtyScenario enum value selecting which action path to execute
    //  - initialDirtyForSave: initial dirtyForSave state before action (DirtyStateTrue/False)
    //  - initialDirtyForUpload: initial dirtyForUpload state before action (DirtyStateTrue/False)
    //  - expectedDirtyForSave: expected final dirtyForSave state (DirtyState)
    //  - expectedDirtyForUpload: expected final dirtyForUpload state (DirtyState)

    QTest::addColumn<int>("scenario");
    QTest::addColumn<int>("initialDirtyForSave");
    QTest::addColumn<int>("initialDirtyForUpload");
    QTest::addColumn<int>("expectedDirtyForSave");
    QTest::addColumn<int>("expectedDirtyForUpload");

    struct ScenarioExpectation {
        DirtyScenario scenario;
        const char* name;
        DirtyState expectedDirtyForSave;
        DirtyState expectedDirtyForUpload;
    };

    const QList<ScenarioExpectation> scenarioExpectations = {
        { UploadPreservesSaveDirtyFalse,      "upload completion keeps save false",  DirtyStateUnchanged, DirtyStateFalse },
        { UploadPreservesSaveDirtyTrue,       "upload completion keeps save true",   DirtyStateUnchanged, DirtyStateFalse },
        { UploadFalseOnPlanClear,             "upload false on clear",               DirtyStateFalse,     DirtyStateFalse },
        { UploadTrueWhenSaveTrue,             "upload true when save true",          DirtyStateTrue,      DirtyStateTrue },
        { UploadTrueOnNewPlanLoad,            "upload true on new plan load",        DirtyStateFalse,     DirtyStateTrue },
        { SaveToFilePreservesUploadDirtyTrue, "saveToFile keeps upload true",        DirtyStateFalse,     DirtyStateUnchanged },
        { SaveToFilePreservesUploadDirtyFalse,"saveToFile keeps upload false",       DirtyStateFalse,     DirtyStateUnchanged },
        { SaveFalseOnSuccessfulLoad,          "save false on successful load",       DirtyStateFalse,     DirtyStateTrue },
        { ClearSaveDirtyPreservesUploadTrue,  "clear save dirty keeps upload true",  DirtyStateFalse,     DirtyStateUnchanged },
        { ClearSaveDirtyPreservesUploadFalse, "clear save dirty keeps upload false", DirtyStateFalse,     DirtyStateUnchanged },
        { DownloadWithItemsDirtyForSave,      "download with items marks save dirty",DirtyStateTrue,      DirtyStateFalse },
        { DownloadEmptyNotDirtyForSave,       "download empty keeps save clean",     DirtyStateFalse,     DirtyStateFalse },
    };

    const QList<DirtyState> initialStates = {
        DirtyStateFalse,
        DirtyStateTrue,
    };

    for (const ScenarioExpectation& expectation : scenarioExpectations) {
        for (const DirtyState initialDirtyForSave : initialStates) {
            for (const DirtyState initialDirtyForUpload : initialStates) {
                DirtyState expectedDirtyForSave = expectation.expectedDirtyForSave;
                DirtyState expectedDirtyForUpload = expectation.expectedDirtyForUpload;

                if ((expectation.scenario == UploadTrueWhenSaveTrue) && (initialDirtyForSave == DirtyStateTrue)) {
                    // _setDirtyForSave(true) only drives dirtyForUpload when dirtyForSave transitions false->true.
                    // If dirtyForSave already starts true, dirtyForUpload is preserved.
                    expectedDirtyForUpload = DirtyStateUnchanged;
                }

                const QString rowName = QStringLiteral("%1 [init save=%2 upload=%3]")
                                            .arg(expectation.name)
                                            .arg(initialDirtyForSave == DirtyStateTrue ? QStringLiteral("true") : QStringLiteral("false"))
                                            .arg(initialDirtyForUpload == DirtyStateTrue ? QStringLiteral("true") : QStringLiteral("false"));
                QTest::newRow(rowName.toLatin1().constData())
                    << +expectation.scenario
                    << +initialDirtyForSave
                    << +initialDirtyForUpload
                    << +expectedDirtyForSave
                    << +expectedDirtyForUpload;
            }
        }
    }
}

void PlanMasterControllerTest::_testDirtyFlagsMatrix()
{
    QFETCH(int, scenario);
    QFETCH(int, initialDirtyForSave);
    QFETCH(int, initialDirtyForUpload);
    QFETCH(int, expectedDirtyForSave);
    QFETCH(int, expectedDirtyForUpload);

    QVERIFY(initialDirtyForSave != DirtyStateUnchanged);
    QVERIFY(initialDirtyForUpload != DirtyStateUnchanged);

    // Pre-load items for scenarios that need containsItems() == true
    if (scenario == DownloadWithItemsDirtyForSave) {
        _masterController->loadFromFile(":/unittest/MissionPlanner.waypoints");
    }

    const auto dirtyStateToBool = [](int state) -> bool {
        switch (state) {
        case DirtyStateFalse:
            return false;
        case DirtyStateTrue:
            return true;
        default:
            Q_ASSERT(false); // Invalid test data
            return false;
        }
    };

    _masterController->_setDirtyForSaveUnitTest(dirtyStateToBool(initialDirtyForSave));
    _masterController->_setDirtyForUploadUnitTest(dirtyStateToBool(initialDirtyForUpload));

    const bool initialDirtyForSaveBool = _masterController->dirtyForSave();
    const bool initialDirtyForUploadBool = _masterController->dirtyForUpload();

    QSignalSpy dirtyForSaveChangedSpy(_masterController, &PlanMasterController::dirtyForSaveChanged);
    QSignalSpy dirtyForUploadChangedSpy(_masterController, &PlanMasterController::dirtyForUploadChanged);

    switch (scenario) {
    case UploadPreservesSaveDirtyFalse: {
        const bool invoked = QMetaObject::invokeMethod(_masterController, "_sendRallyPointsComplete", Qt::DirectConnection);
        QVERIFY(invoked);
        break;
    }
    case UploadPreservesSaveDirtyTrue: {
        const bool invoked = QMetaObject::invokeMethod(_masterController, "_sendRallyPointsComplete", Qt::DirectConnection);
        QVERIFY(invoked);
        break;
    }
    case UploadFalseOnPlanClear:
        _masterController->removeAll();
        break;
    case UploadTrueWhenSaveTrue:
        _masterController->_setDirtyForSave(true);
        break;
    case UploadTrueOnNewPlanLoad:
        _masterController->loadFromFile(":/unittest/MissionPlanner.waypoints");
        break;
    case SaveToFilePreservesUploadDirtyTrue: {
        const QString saveFile = QDir::temp().filePath(QStringLiteral("qgc_planmaster_test_%1.plan").arg(QDateTime::currentMSecsSinceEpoch()));
        QVERIFY(_masterController->saveToFile(saveFile));
        QFile::remove(saveFile);
        break;
    }
    case SaveToFilePreservesUploadDirtyFalse: {
        const QString saveFile = QDir::temp().filePath(QStringLiteral("qgc_planmaster_test_%1.plan").arg(QDateTime::currentMSecsSinceEpoch()));
        QVERIFY(_masterController->saveToFile(saveFile));
        QFile::remove(saveFile);
        break;
    }
    case SaveFalseOnSuccessfulLoad:
        _masterController->loadFromFile(":/unittest/MissionPlanner.waypoints");
        break;
    case ClearSaveDirtyPreservesUploadTrue:
        _masterController->_setDirtyForSave(false);
        break;
    case ClearSaveDirtyPreservesUploadFalse:
        _masterController->_setDirtyForSave(false);
        break;
    case DownloadWithItemsDirtyForSave: {
        QVERIFY(_masterController->containsItems());
        const bool invoked = QMetaObject::invokeMethod(_masterController, "_loadRallyPointsComplete", Qt::DirectConnection);
        QVERIFY(invoked);
        break;
    }
    case DownloadEmptyNotDirtyForSave: {
        QVERIFY(!_masterController->containsItems());
        const bool invoked = QMetaObject::invokeMethod(_masterController, "_loadRallyPointsComplete", Qt::DirectConnection);
        QVERIFY(invoked);
        break;
    }
    }

    const auto resolveExpected = [](int expectedState, bool unchangedValue) -> bool {
        switch (expectedState) {
        case DirtyStateFalse:
            return false;
        case DirtyStateTrue:
            return true;
        case DirtyStateUnchanged:
            return unchangedValue;
        }
        return unchangedValue;
    };

    const bool expectedDirtyForSaveBool = resolveExpected(expectedDirtyForSave, initialDirtyForSaveBool);
    const bool expectedDirtyForUploadBool = resolveExpected(expectedDirtyForUpload, initialDirtyForUploadBool);
    const int expectedDirtyForSaveSignalCount = (expectedDirtyForSaveBool != initialDirtyForSaveBool) ? 1 : 0;
    const int expectedDirtyForUploadSignalCount = (expectedDirtyForUploadBool != initialDirtyForUploadBool) ? 1 : 0;

    QCOMPARE(_masterController->dirtyForSave(), expectedDirtyForSaveBool);
    QCOMPARE(_masterController->dirtyForUpload(), expectedDirtyForUploadBool);

    QCOMPARE(dirtyForSaveChangedSpy.count(), expectedDirtyForSaveSignalCount);
    QCOMPARE(dirtyForUploadChangedSpy.count(), expectedDirtyForUploadSignalCount);

    if (dirtyForSaveChangedSpy.count() > 0) {
        const QList<QVariant>& args = dirtyForSaveChangedSpy.at(dirtyForSaveChangedSpy.count() - 1);
        QCOMPARE(args.count(), 1);
        QCOMPARE(args.first().toBool(), _masterController->dirtyForSave());
    }

    if (dirtyForUploadChangedSpy.count() > 0) {
        const QList<QVariant>& args = dirtyForUploadChangedSpy.at(dirtyForUploadChangedSpy.count() - 1);
        QCOMPARE(args.count(), 1);
        QCOMPARE(args.first().toBool(), _masterController->dirtyForUpload());
    }
}

void PlanMasterControllerTest::_testFileNamesSetOnLoad()
{
    QSignalSpy currentNameSpy(_masterController, &PlanMasterController::currentPlanFileNameChanged);
    QSignalSpy originalNameSpy(_masterController, &PlanMasterController::originalPlanFileNameChanged);

    // Before load, names should be empty
    QVERIFY(_masterController->currentPlanFileName().isEmpty());
    QVERIFY(_masterController->originalPlanFileName().isEmpty());

    _masterController->loadFromFile(":/unittest/MissionPlanner.waypoints");

    // After successful load, both names should be set to the base name
    QCOMPARE(_masterController->currentPlanFileName(), QStringLiteral("MissionPlanner"));
    QCOMPARE(_masterController->originalPlanFileName(), QStringLiteral("MissionPlanner"));

    // Signals should have fired
    QVERIFY(currentNameSpy.count() >= 1);
    QVERIFY(originalNameSpy.count() >= 1);
}

void PlanMasterControllerTest::_testCurrentPlanFileNameWritable()
{
    _masterController->loadFromFile(":/unittest/MissionPlanner.waypoints");

    QSignalSpy currentNameSpy(_masterController, &PlanMasterController::currentPlanFileNameChanged);
    QSignalSpy originalNameSpy(_masterController, &PlanMasterController::originalPlanFileNameChanged);

    // Rename via the writable property
    _masterController->setCurrentPlanFileName(QStringLiteral("RenamedPlan"));

    QCOMPARE(_masterController->currentPlanFileName(), QStringLiteral("RenamedPlan"));
    // Original should remain unchanged
    QCOMPARE(_masterController->originalPlanFileName(), QStringLiteral("MissionPlanner"));

    QCOMPARE(currentNameSpy.count(), 1);
    QCOMPARE(originalNameSpy.count(), 0);

    // Setting to the same value should not emit again
    _masterController->setCurrentPlanFileName(QStringLiteral("RenamedPlan"));
    QCOMPARE(currentNameSpy.count(), 1);
}

void PlanMasterControllerTest::_testPlanFileRenamed()
{
    // Before load, planFileRenamed should be false (both names empty)
    QVERIFY(!_masterController->planFileRenamed());

    _masterController->loadFromFile(":/unittest/MissionPlanner.waypoints");

    // After load, current == original → not renamed
    QVERIFY(!_masterController->planFileRenamed());

    // Rename
    _masterController->setCurrentPlanFileName(QStringLiteral("NewName"));
    QVERIFY(_masterController->planFileRenamed());

    // Rename back to original
    _masterController->setCurrentPlanFileName(QStringLiteral("MissionPlanner"));
    QVERIFY(!_masterController->planFileRenamed());
}

void PlanMasterControllerTest::_testSaveWithCurrentName()
{
    _masterController->loadFromFile(":/unittest/MissionPlanner.waypoints");

    // First save to a real (writable) directory so _currentPlanFile points somewhere valid
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());
    const QString initialPath = QStringLiteral("%1/MissionPlanner.%2").arg(tmpDir.path(), _masterController->fileExtension());
    QVERIFY(_masterController->saveToFile(initialPath));

    // Rename
    _masterController->setCurrentPlanFileName(QStringLiteral("TestSaveRenamed"));

    // Save with the renamed name
    QVERIFY(_masterController->saveWithCurrentName());

    // After save, original should now match the renamed name
    QCOMPARE(_masterController->originalPlanFileName(), QStringLiteral("TestSaveRenamed"));
    QCOMPARE(_masterController->currentPlanFileName(), QStringLiteral("TestSaveRenamed"));
    QVERIFY(!_masterController->planFileRenamed());
}

void PlanMasterControllerTest::_testSaveWithCurrentNameNoFile()
{
    // No file loaded — saveWithCurrentName with empty name should fail
    QVERIFY(!_masterController->saveWithCurrentName());

    // Set a name without loading a file first (simulates typing a name in the UI)
    _masterController->setCurrentPlanFileName(QStringLiteral("BrandNewPlan"));

    // Should save to the default mission save directory
    QVERIFY(_masterController->saveWithCurrentName());

    const QString expectedDir = SettingsManager::instance()->appSettings()->missionSavePath();
    const QString expectedPath = QStringLiteral("%1/BrandNewPlan.%2").arg(expectedDir, _masterController->fileExtension());
    QCOMPARE(_masterController->currentPlanFile(), expectedPath);
    QCOMPARE(_masterController->originalPlanFileName(), QStringLiteral("BrandNewPlan"));

    // Clean up
    QFile::remove(expectedPath);
}

void PlanMasterControllerTest::_testResolvedPlanFileExists()
{
    // Empty name → should return false
    QVERIFY(!_masterController->resolvedPlanFileExists());

    // Save a file so it exists on disk
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());
    const QString savePath = QStringLiteral("%1/ExistingPlan.%2").arg(tmpDir.path(), _masterController->fileExtension());
    QVERIFY(_masterController->saveToFile(savePath));

    // Now rename to the same base name — file exists at resolved path
    _masterController->setCurrentPlanFileName(QStringLiteral("ExistingPlan"));
    QVERIFY(_masterController->resolvedPlanFileExists());

    // Rename to something non-existent
    _masterController->setCurrentPlanFileName(QStringLiteral("DoesNotExist"));
    QVERIFY(!_masterController->resolvedPlanFileExists());
}

void PlanMasterControllerTest::_testFileNamesClearedOnRemoveAll()
{
    _masterController->loadFromFile(":/unittest/MissionPlanner.waypoints");

    // Verify names are set
    QVERIFY(!_masterController->currentPlanFileName().isEmpty());
    QVERIFY(!_masterController->originalPlanFileName().isEmpty());

    QSignalSpy currentNameSpy(_masterController, &PlanMasterController::currentPlanFileNameChanged);
    QSignalSpy originalNameSpy(_masterController, &PlanMasterController::originalPlanFileNameChanged);

    _masterController->removeAll();

    // Names should be cleared
    QVERIFY(_masterController->currentPlanFileName().isEmpty());
    QVERIFY(_masterController->originalPlanFileName().isEmpty());
    QVERIFY(_masterController->currentPlanFile().isEmpty());

    QVERIFY(currentNameSpy.count() >= 1);
    QVERIFY(originalNameSpy.count() >= 1);
}

void PlanMasterControllerTest::_testFileNamesClearedOnRemoveAllFromVehicle()
{
    _connectMockLink(MAV_AUTOPILOT_PX4);

    _masterController->loadFromFile(":/unittest/MissionPlanner.waypoints");

    // Verify names are set
    QVERIFY(!_masterController->currentPlanFileName().isEmpty());
    QVERIFY(!_masterController->originalPlanFileName().isEmpty());

    QSignalSpy currentNameSpy(_masterController, &PlanMasterController::currentPlanFileNameChanged);
    QSignalSpy originalNameSpy(_masterController, &PlanMasterController::originalPlanFileNameChanged);

    _masterController->removeAllFromVehicle();

    // Names should be cleared
    QVERIFY(_masterController->currentPlanFileName().isEmpty());
    QVERIFY(_masterController->originalPlanFileName().isEmpty());
    QVERIFY(_masterController->currentPlanFile().isEmpty());

    QVERIFY(currentNameSpy.count() >= 1);
    QVERIFY(originalNameSpy.count() >= 1);
}

void PlanMasterControllerTest::_testSaveUpdatesOriginalFileName()
{
    _masterController->loadFromFile(":/unittest/MissionPlanner.waypoints");
    QCOMPARE(_masterController->originalPlanFileName(), QStringLiteral("MissionPlanner"));

    // Save to a completely different path
    const QString saveFile = QDir::temp().filePath(
        QStringLiteral("qgc_planmaster_rename_%1.plan").arg(QDateTime::currentMSecsSinceEpoch()));
    QVERIFY(_masterController->saveToFile(saveFile));

    // Both names should now reflect the new file base name
    const QString expectedBase = QFileInfo(saveFile).completeBaseName();
    QCOMPARE(_masterController->currentPlanFileName(), expectedBase);
    QCOMPARE(_masterController->originalPlanFileName(), expectedBase);

    // Clean up
    QFile::remove(saveFile);
}

void PlanMasterControllerTest::_testTemplateModeHidesTemplatesOnPlanCreatorSelection()
{
    // Initial state: empty plan → templates shown
    QVERIFY(_masterController->showCreateFromTemplate());

    QSignalSpy spyShow(_masterController, &PlanMasterController::showCreateFromTemplateChanged);

    // User selects a plan creator (e.g. Survey) — adds items to the plan
    SurveyPlanCreator creator(_masterController);
    creator.createPlan(QGeoCoordinate(47.0, -122.0));

    QVERIFY(_masterController->containsItems());
    QVERIFY(!_masterController->showCreateFromTemplate());
    QCOMPARE(spyShow.count(), 1);
}

void PlanMasterControllerTest::_testTemplateModeHidesTemplatesOnFileLoad()
{
    // Initial state: empty plan, not manual creation → templates shown
    QVERIFY(_masterController->showCreateFromTemplate());

    QSignalSpy spyShow(_masterController, &PlanMasterController::showCreateFromTemplateChanged);

    _masterController->loadFromFile(":/unittest/MissionPlanner.waypoints");

    QVERIFY(_masterController->containsItems());
    QVERIFY(!_masterController->showCreateFromTemplate());
    QCOMPARE(spyShow.count(), 1);
}

void PlanMasterControllerTest::_testTemplateModeRestoredOnRemoveAll()
{
    _masterController->loadFromFile(":/unittest/MissionPlanner.waypoints");
    QVERIFY(!_masterController->showCreateFromTemplate());

    QSignalSpy spyShow(_masterController, &PlanMasterController::showCreateFromTemplateChanged);

    _masterController->removeAll();

    QVERIFY(!_masterController->containsItems());
    QVERIFY(_masterController->showCreateFromTemplate());
    QCOMPARE(spyShow.count(), 1);
}

void PlanMasterControllerTest::_testTemplateModeRestoredOnIndividualItemRemoval()
{
    _masterController->loadFromFile(":/unittest/MissionPlanner.waypoints");
    QVERIFY(!_masterController->showCreateFromTemplate());

    QSignalSpy spyShow(_masterController, &PlanMasterController::showCreateFromTemplateChanged);

    _masterController->missionController()->removeAll();
    _masterController->geoFenceController()->removeAll();
    _masterController->rallyPointController()->removeAll();

    QVERIFY(!_masterController->containsItems());
    QVERIFY(_masterController->showCreateFromTemplate());
    QCOMPARE(spyShow.count(), 1);
}

void PlanMasterControllerTest::_testManualCreationHidesTemplates()
{
    // Initial state: empty plan → templates shown
    QVERIFY(_masterController->showCreateFromTemplate());
    QVERIFY(!_masterController->userSelectedManualCreation());

    QSignalSpy spyShow(_masterController, &PlanMasterController::showCreateFromTemplateChanged);
    QSignalSpy spyManual(_masterController, &PlanMasterController::userSelectedManualCreationChanged);

    // User clicks "No Template" — hides templates even though plan is empty
    _masterController->setUserSelectedManualCreation(true);

    QVERIFY(_masterController->userSelectedManualCreation());
    QVERIFY(!_masterController->showCreateFromTemplate());
    QCOMPARE(spyShow.count(), 1);
    QCOMPARE(spyManual.count(), 1);

    // Setting the same value again should not re-emit
    _masterController->setUserSelectedManualCreation(true);
    QCOMPARE(spyShow.count(), 1);
    QCOMPARE(spyManual.count(), 1);
}

void PlanMasterControllerTest::_testManualCreationRestoredOnRemoveAll()
{
    _masterController->setUserSelectedManualCreation(true);
    _masterController->loadFromFile(":/unittest/MissionPlanner.waypoints");
    QVERIFY(!_masterController->showCreateFromTemplate());

    QSignalSpy spyShow(_masterController, &PlanMasterController::showCreateFromTemplateChanged);
    QSignalSpy spyManual(_masterController, &PlanMasterController::userSelectedManualCreationChanged);

    _masterController->removeAll();

    QVERIFY(!_masterController->containsItems());
    QVERIFY(!_masterController->userSelectedManualCreation());
    QVERIFY(_masterController->showCreateFromTemplate());
    QCOMPARE(spyShow.count(), 1);
    QCOMPARE(spyManual.count(), 1);
}

void PlanMasterControllerTest::_testManualCreationRestoredOnIndividualItemRemoval()
{
    _masterController->setUserSelectedManualCreation(true);
    _masterController->loadFromFile(":/unittest/MissionPlanner.waypoints");
    QVERIFY(!_masterController->showCreateFromTemplate());

    QSignalSpy spyShow(_masterController, &PlanMasterController::showCreateFromTemplateChanged);
    QSignalSpy spyManual(_masterController, &PlanMasterController::userSelectedManualCreationChanged);

    _masterController->missionController()->removeAll();
    _masterController->geoFenceController()->removeAll();
    _masterController->rallyPointController()->removeAll();

    QVERIFY(!_masterController->containsItems());
    QVERIFY(!_masterController->userSelectedManualCreation());
    QVERIFY(_masterController->showCreateFromTemplate());
    QCOMPARE(spyShow.count(), 1);
    QCOMPARE(spyManual.count(), 1);
}

void PlanMasterControllerTest::_testPlanCreatorsFiltered()
{
    // MultiRotor supports StructureScan — expect all 4 creators
    PlanMasterController multiRotorController(MAV_AUTOPILOT_PX4, MAV_TYPE_QUADROTOR);
    multiRotorController.setFlyView(false);
    multiRotorController.start();
    QVERIFY(multiRotorController.planCreators() != nullptr);
    const int multiRotorCount = multiRotorController.planCreators()->count();
    QVERIFY(multiRotorCount > 0);

    // FixedWing does not support StructureScan — expect one fewer creator
    PlanMasterController fixedWingController(MAV_AUTOPILOT_PX4, MAV_TYPE_FIXED_WING);
    fixedWingController.setFlyView(false);
    fixedWingController.start();
    QVERIFY(fixedWingController.planCreators() != nullptr);
    const int fixedWingCount = fixedWingController.planCreators()->count();
    QVERIFY(fixedWingCount > 0);

    QCOMPARE(fixedWingCount, multiRotorCount - 1);
}

#include "UnitTest.h"

UT_REGISTER_TEST(PlanMasterControllerTest, TestLabel::Integration, TestLabel::MissionManager)
