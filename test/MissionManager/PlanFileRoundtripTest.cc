#include "PlanFileRoundtripTest.h"
#include "PlanMasterController.h"
#include "MissionController.h"
#include "SimpleMissionItem.h"
#include "SettingsManager.h"
#include "AppSettings.h"
#include "QGCMAVLink.h"
#include "QmlObjectListModel.h"

#include <QtCore/QTemporaryFile>
#include <QtCore/QFile>
#include <QtTest/QTest>

void PlanFileRoundtripTest::init()
{
    OfflineTest::init();

    // Set offline firmware type
    SettingsManager::instance()->appSettings()->offlineEditingFirmwareClass()->setRawValue(
        QGCMAVLink::firmwareClass(MAV_AUTOPILOT_PX4));

    _masterController = new PlanMasterController(this);
    VERIFY_NOT_NULL(_masterController);
    _masterController->setFlyView(false);
    _masterController->start();

    // Create a temporary file path for testing
    QTemporaryFile tempFile;
    tempFile.setAutoRemove(false);
    if (tempFile.open()) {
        _tempFilePath = tempFile.fileName() + ".plan";
        tempFile.close();
        tempFile.remove();
    }
}

void PlanFileRoundtripTest::cleanup()
{
    // Remove temp file if it exists
    if (!_tempFilePath.isEmpty()) {
        QFile::remove(_tempFilePath);
    }

    delete _masterController;
    _masterController = nullptr;
    _tempFilePath.clear();

    OfflineTest::cleanup();
}

void PlanFileRoundtripTest::_createMission(int waypointCount)
{
    MissionController* missionController = _masterController->missionController();

    // Add waypoints in a pattern
    QGeoCoordinate baseCoord(47.3977, 8.5456, 100.0);  // Zurich area
    for (int i = 0; i < waypointCount; i++) {
        QGeoCoordinate waypoint = baseCoord.atDistanceAndAzimuth(100.0 * (i + 1), 45.0 * i);
        waypoint.setAltitude(50.0 + i * 10.0);
        missionController->insertSimpleMissionItem(waypoint, missionController->visualItems()->count());
    }
}

bool PlanFileRoundtripTest::_compareMissions(MissionController* original, MissionController* loaded)
{
    QmlObjectListModel* originalItems = original->visualItems();
    QmlObjectListModel* loadedItems = loaded->visualItems();

    if (!TestHelpers::verifyNotNull(originalItems, "originalItems") ||
        !TestHelpers::verifyNotNull(loadedItems, "loadedItems")) {
        return false;
    }

    if (originalItems->count() != loadedItems->count()) {
        qWarning() << "Item count mismatch: original" << originalItems->count() << "loaded" << loadedItems->count();
        return false;
    }

    // Compare each item (skip settings item at index 0)
    for (int i = 1; i < originalItems->count(); i++) {
        SimpleMissionItem* origItem = qobject_cast<SimpleMissionItem*>(originalItems->get(i));
        SimpleMissionItem* loadItem = qobject_cast<SimpleMissionItem*>(loadedItems->get(i));

        if (!origItem || !loadItem) {
            // Could be a complex item, just check they're both the same type
            if ((origItem == nullptr) != (loadItem == nullptr)) {
                qWarning() << "Item type mismatch at index" << i;
                return false;
            }
            continue;
        }

        // Compare coordinates
        if (qAbs(origItem->coordinate().latitude() - loadItem->coordinate().latitude()) > 0.0001) {
            qWarning() << "Latitude mismatch at index" << i;
            return false;
        }
        if (qAbs(origItem->coordinate().longitude() - loadItem->coordinate().longitude()) > 0.0001) {
            qWarning() << "Longitude mismatch at index" << i;
            return false;
        }
    }

    return true;
}

void PlanFileRoundtripTest::_testEmptyMissionRoundtrip()
{
    // Save empty mission
    _masterController->saveToFile(_tempFilePath);
    VERIFY_FILE_EXISTS(_tempFilePath);

    // Create new controller and load
    PlanMasterController* loadController = new PlanMasterController(this);
    VERIFY_NOT_NULL(loadController);
    loadController->setFlyView(false);
    loadController->start();
    loadController->loadFromFile(_tempFilePath);

    // Verify - both should have just the settings item
    QCOMPARE(loadController->missionController()->visualItems()->count(),
             _masterController->missionController()->visualItems()->count());

    delete loadController;
}

void PlanFileRoundtripTest::_testSimpleMissionRoundtrip()
{
    const int waypointCount = 5;

    // Create mission with waypoints
    _createMission(waypointCount);
    int originalCount = _masterController->missionController()->visualItems()->count();
    QCOMPARE_GT(originalCount, 1);

    // Save to file
    _masterController->saveToFile(_tempFilePath);
    VERIFY_FILE_EXISTS(_tempFilePath);

    // Create new controller and load
    PlanMasterController* loadController = new PlanMasterController(this);
    VERIFY_NOT_NULL(loadController);
    loadController->setFlyView(false);
    loadController->start();
    loadController->loadFromFile(_tempFilePath);

    // Verify
    QCOMPARE(loadController->missionController()->visualItems()->count(), originalCount);
    QVERIFY(_compareMissions(_masterController->missionController(), loadController->missionController()));

    delete loadController;
}

void PlanFileRoundtripTest::_testComplexMissionRoundtrip()
{
    MissionController* missionController = _masterController->missionController();
    VERIFY_NOT_NULL(missionController);

    // Add some waypoints
    _createMission(3);

    // Add a survey item using the proper API
    QGeoCoordinate surveyCenter(47.4, 8.55, 100.0);
    missionController->insertComplexMissionItem(
        QStringLiteral("Survey"),
        surveyCenter,
        missionController->visualItems()->count(),
        false /* makeCurrentItem */);

    int originalCount = missionController->visualItems()->count();

    // Save to file
    _masterController->saveToFile(_tempFilePath);
    VERIFY_FILE_EXISTS(_tempFilePath);

    // Create new controller and load
    PlanMasterController* loadController = new PlanMasterController(this);
    VERIFY_NOT_NULL(loadController);
    loadController->setFlyView(false);
    loadController->start();
    loadController->loadFromFile(_tempFilePath);

    // Verify count matches
    QCOMPARE(loadController->missionController()->visualItems()->count(), originalCount);

    delete loadController;
}

void PlanFileRoundtripTest::_testCorruptedFileHandling()
{
    // Write corrupted JSON to file
    QFile file(_tempFilePath);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("{ invalid json content }}}");
    file.close();

    // Try to load - should not crash
    PlanMasterController* loadController = new PlanMasterController(this);
    VERIFY_NOT_NULL(loadController);
    loadController->setFlyView(false);
    loadController->start();

    // This should handle the error gracefully
    loadController->loadFromFile(_tempFilePath);

    // Should still have a valid (empty) mission
    VERIFY_NOT_NULL(loadController->missionController());
    VERIFY_NOT_NULL(loadController->missionController()->visualItems());

    delete loadController;
}
