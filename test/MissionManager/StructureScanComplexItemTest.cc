#include "StructureScanComplexItemTest.h"

#include <algorithm>
#include <cmath>
#include <numbers>

#include <QtCore/QJsonArray>
#include <QtCore/QRegularExpression>

#include "AppSettings.h"
#include "CoordFixtures.h"
#include "MissionItem.h"
#include "MultiSignalSpy.h"
#include "PlanMasterController.h"
#include "QGCMAVLink.h"
#include "SettingsManager.h"
#include "StructureScanComplexItem.h"

StructureScanComplexItemTest::StructureScanComplexItemTest()
{
    _polyPoints << TestFixtures::Coord::missionTestOrigin() << QGeoCoordinate(47.634129020000003, -122.08887249)
                << QGeoCoordinate(47.633619320000001, -122.08811074)
                << QGeoCoordinate(47.633189139999999, -122.08900124);
}

void StructureScanComplexItemTest::init()
{
    OfflineMissionTest::init();
    _structureScanItem = new StructureScanComplexItem(planController(), false /* flyView */, QString() /* kmlFile */);
    _structureScanItem->setDirty(false);
    _multiSpy = std::make_unique<MultiSignalSpy>();
    QVERIFY(_multiSpy);
    QCOMPARE(_multiSpy->init(_structureScanItem), true);
}

void StructureScanComplexItemTest::cleanup()
{
    _multiSpy.reset();
    _structureScanItem = nullptr;  // Deleted when planController is deleted
    OfflineMissionTest::cleanup();
}

void StructureScanComplexItemTest::_testDirty()
{
    QVERIFY(!_structureScanItem->dirty());
    _structureScanItem->setDirty(false);
    QVERIFY(!_structureScanItem->dirty());
    QVERIFY(_multiSpy->noneEmitted());
    _structureScanItem->setDirty(true);
    QVERIFY(_structureScanItem->dirty());
    QVERIFY(_multiSpy->onlyEmittedOnce("dirtyChanged"));
    QVERIFY(_multiSpy->argument<bool>("dirtyChanged"));
    _multiSpy->clearAllSignals();
    _structureScanItem->setDirty(false);
    QVERIFY(!_structureScanItem->dirty());
    QVERIFY(_multiSpy->onlyEmittedOnce("dirtyChanged"));
    QVERIFY(!_multiSpy->argument<bool>("dirtyChanged"));
    _multiSpy->clearAllSignals();
    // These facts should set dirty when changed
    QList<Fact*> rgFacts;
    rgFacts << _structureScanItem->entranceAlt() << _structureScanItem->layers();
    for (Fact* fact : rgFacts) {
        qCDebug(UnitTestLog) << fact->name();
        QVERIFY(!_structureScanItem->dirty());
        if (fact->typeIsBool()) {
            fact->setRawValue(!fact->rawValue().toBool());
        } else {
            fact->setRawValue(fact->rawValue().toDouble() + 1);
        }
        QVERIFY(_multiSpy->emittedOnce("dirtyChanged"));
        QVERIFY(_multiSpy->argument<bool>("dirtyChanged"));
        _structureScanItem->setDirty(false);
        _multiSpy->clearAllSignals();
    }
    rgFacts.clear();
}

void StructureScanComplexItemTest::_initItem()
{
    QGCMapPolygon* mapPolygon = _structureScanItem->structurePolygon();
    for (int i = 0; i < _polyPoints.count(); i++) {
        QGeoCoordinate& vertex = _polyPoints[i];
        mapPolygon->appendVertex(vertex);
    }
    _structureScanItem->cameraCalc()->setCameraBrand(CameraCalc::canonicalManualCameraName());
    _structureScanItem->layers()->setCookedValue(2);
    _structureScanItem->setDirty(false);
    _validateItem(_structureScanItem);
}

void StructureScanComplexItemTest::_validateItem(StructureScanComplexItem* item)
{
    QGCMapPolygon* mapPolygon = item->structurePolygon();
    for (int i = 0; i < _polyPoints.count(); i++) {
        QGeoCoordinate& expectedVertex = _polyPoints[i];
        QGeoCoordinate actualVertex = mapPolygon->vertexCoordinate(i);
        QCOMPARE(expectedVertex, actualVertex);
    }
    QVERIFY(_structureScanItem->cameraCalc()->isManualCamera());
    QCOMPARE(item->layers()->cookedValue().toInt(), 2);
}

void StructureScanComplexItemTest::_testSaveLoad()
{
    _initItem();
    QJsonArray items;
    _structureScanItem->save(items);
    QString errorString;
    StructureScanComplexItem* newItem =
        new StructureScanComplexItem(planController(), false /* flyView */, QString() /* kmlFile */);
    QVERIFY(newItem->load(items[0].toObject(), 10, errorString));
    QVERIFY(errorString.isEmpty());
    _validateItem(newItem);
    newItem->deleteLater();
}

void StructureScanComplexItemTest::_testItemCount()
{
    QList<MissionItem*> items;
    ignoreLogMessage("QMLControls.QGCMapPolygon", QtWarningMsg, QRegularExpression("bad vertex requested"));
    _initItem();
    _structureScanItem->appendMissionItems(items, this);
    QCOMPARE(items.count() - 1, _structureScanItem->lastSequenceNumber());
}

void StructureScanComplexItemTest::_testLargeStandoffFlightPolygon()
{
    // A 1m structure scanned from 50m away, well beyond the structure's own extent
    const QGeoCoordinate origin = TestFixtures::Coord::missionTestOrigin();
    const QGeoCoordinate north = origin.atDistanceAndAzimuth(1.0, 0.0);
    const QList<QGeoCoordinate> structure = {
        origin,
        north,
        north.atDistanceAndAzimuth(1.0, 90.0),
        origin.atDistanceAndAzimuth(1.0, 90.0),
    };
    _structureScanItem->cameraCalc()->distanceToSurface()->setRawValue(50.0);
    _structureScanItem->structurePolygon()->appendVertices(structure);

    QGCMapPolygon* flightPolygon = _structureScanItem->flightPolygon();
    QCOMPARE(flightPolygon->count(), structure.count());
    for (const QGeoCoordinate& vertex : structure) {
        QVERIFY(flightPolygon->containsCoordinate(vertex));
    }
    QVERIFY(qAbs(flightPolygon->vertexCoordinate(0).distanceTo(origin) - (50.0 * std::numbers::sqrt2)) < 0.5);

    // Clockwise winding keeps the camera yawed towards the structure
    const QList<QGeoCoordinate> flightCoordinates = flightPolygon->coordinateList();
    flightPolygon->verifyClockwiseWinding();
    QCOMPARE(flightPolygon->coordinateList(), flightCoordinates);
    QCOMPARE(_structureScanItem->readyForSaveState(), VisualMissionItem::ReadyForSave);
}

void StructureScanComplexItemTest::_testAcuteStructureCorner()
{
    // The ~11 degree corner at the origin is squared off, which adds a flight path vertex (mavlink/qgroundcontrol#5917)
    const QGeoCoordinate origin = TestFixtures::Coord::missionTestOrigin();
    const QGeoCoordinate east = origin.atDistanceAndAzimuth(100.0, 90.0);
    const QList<QGeoCoordinate> structure = {origin, east, east.atDistanceAndAzimuth(20.0, 0.0)};
    _structureScanItem->cameraCalc()->distanceToSurface()->setRawValue(10.0);
    _structureScanItem->structurePolygon()->appendVertices(structure);

    QGCMapPolygon* flightPolygon = _structureScanItem->flightPolygon();
    QCOMPARE(flightPolygon->count(), structure.count() + 1);
    for (const QGeoCoordinate& vertex : flightPolygon->coordinateList()) {
        QVERIFY(vertex.distanceTo(origin) < 30.0 || vertex.distanceTo(east) < 30.0 ||
                vertex.distanceTo(structure.last()) < 30.0);
    }

    // Mission item count follows the flight path rather than the structure vertex count
    QList<MissionItem*> items;
    _structureScanItem->appendMissionItems(items, this);
    QCOMPARE(items.count() - 1, _structureScanItem->lastSequenceNumber());
}

void StructureScanComplexItemTest::_testFlightPolygonOffsetFailure()
{
    // Coincident vertices form a three vertex structure polygon which cannot be offset
    const QGeoCoordinate origin = TestFixtures::Coord::missionTestOrigin();
    ignoreLogMessage("QMLControls.QGCMapPolygon", QtWarningMsg, QRegularExpression("Polygon offset"));
    expectAppMessage(QRegularExpression("Unable to calculate a flight path"));
    _structureScanItem->structurePolygon()->appendVertices({origin, origin, origin});
    verifyExpectedLogMessage();

    QCOMPARE(_structureScanItem->flightPolygon()->count(), 0);
    QCOMPARE(_structureScanItem->readyForSaveState(), VisualMissionItem::NotReadyForSaveData);

    // Rebuilding while still failing does not repeat the message
    Fact* distanceToSurface = _structureScanItem->cameraCalc()->distanceToSurface();
    distanceToSurface->setRawValue(distanceToSurface->rawValue().toDouble() + 1.0);
    QCOMPARE(_structureScanItem->flightPolygon()->count(), 0);

    _structureScanItem->structurePolygon()->setPath(_polyPoints);
    QCOMPARE(_structureScanItem->flightPolygon()->count(), _polyPoints.count());
    QCOMPARE(_structureScanItem->readyForSaveState(), VisualMissionItem::ReadyForSave);
}

void StructureScanComplexItemTest::_testSquareOnCameraWithoutWPNextOffset()
{
    constexpr int layerCount = 2;
    constexpr double gimbalPitch = -20.0;
    _structureScanItem->structurePolygon()->appendVertices(_polyPoints);
    _structureScanItem->layers()->setCookedValue(layerCount);
    _structureScanItem->cameraCalc()->setCameraBrand(CameraCalc::canonicalManualCameraName());
    _structureScanItem->gimbalPitch()->setRawValue(gimbalPitch);
    QGCMapPolygon* const flightPolygon = _structureScanItem->flightPolygon();
    const int legCount = flightPolygon->count();
    QCOMPARE(legCount, _polyPoints.count());

    const auto countCommand = [](const QList<MissionItem*>& items, MAV_CMD command) {
        return std::count_if(items.cbegin(), items.cend(),
                             [command](const MissionItem* item) { return item->command() == command; });
    };

    QList<MissionItem*> px4Items;
    _structureScanItem->appendMissionItems(px4Items, this);
    QCOMPARE(countCommand(px4Items, MAV_CMD_DO_SET_ROI_WPNEXT_OFFSET), 1);
    const int px4LastSequenceNumber = _structureScanItem->lastSequenceNumber();

    Fact* const firmwareClass = SettingsManager::instance()->appSettings()->offlineEditingFirmwareClass();

    // Generic firmware reports no command list, which means every command is supported
    firmwareClass->setRawValue(QGCMAVLink::firmwareClass(MAV_AUTOPILOT_GENERIC));
    QCOMPARE(_structureScanItem->lastSequenceNumber(), px4LastSequenceNumber);
    QList<MissionItem*> genericItems;
    _structureScanItem->appendMissionItems(genericItems, this);
    QCOMPARE(countCommand(genericItems, MAV_CMD_DO_SET_ROI_WPNEXT_OFFSET), 1);

    if (!apmFirmwareSupported()) {
        QSKIP("ArduPilot support not registered in this build");
    }

    // ArduPilot does not support ROI_WPNEXT_OFFSET (mavlink/qgroundcontrol#15379)
    _multiSpy->clearAllSignals();
    firmwareClass->setRawValue(QGCMAVLink::firmwareClass(MAV_AUTOPILOT_ARDUPILOTMEGA));
    QVERIFY(_multiSpy->emitted("lastSequenceNumberChanged"));
    QCOMPARE(_structureScanItem->lastSequenceNumber(), px4LastSequenceNumber + 2 * legCount * layerCount);

    // Either winding of the structure polygon keeps the camera facing it
    QList<QGeoCoordinate> reversedPoints = _polyPoints;
    std::reverse(reversedPoints.begin(), reversedPoints.end());
    for (const QList<QGeoCoordinate>& structure : {_polyPoints, reversedPoints}) {
        _structureScanItem->structurePolygon()->setPath(structure);
        QList<MissionItem*> apmItems;
        _structureScanItem->appendMissionItems(apmItems, this);
        QCOMPARE(apmItems.count() - 1, _structureScanItem->lastSequenceNumber());
        QCOMPARE(countCommand(apmItems, MAV_CMD_DO_SET_ROI_WPNEXT_OFFSET), 0);
        QCOMPARE(countCommand(apmItems, MAV_CMD_DO_SET_ROI_NONE), 0);
        QCOMPARE(countCommand(apmItems, MAV_CMD_CONDITION_YAW), legCount * layerCount);
        QCOMPARE(countCommand(apmItems, MAV_CMD_DO_MOUNT_CONTROL), legCount * layerCount + 2);

        // The gimbal is pitched before the layers and returned to neutral after them
        QCOMPARE(apmItems[1]->command(), MAV_CMD_DO_MOUNT_CONTROL);
        QCOMPARE(apmItems[1]->param1(), gimbalPitch);
        QCOMPARE(apmItems[apmItems.count() - 2]->command(), MAV_CMD_DO_MOUNT_CONTROL);
        QCOMPARE(apmItems[apmItems.count() - 2]->param1(), 0.0);

        for (qsizetype i = 0; i < apmItems.count(); i++) {
            if (apmItems[i]->command() != MAV_CMD_CONDITION_YAW) {
                continue;
            }

            // ArduCopter resets the heading after running the first command following a waypoint
            QCOMPARE(apmItems[i - 1]->command(), MAV_CMD_DO_MOUNT_CONTROL);
            QCOMPARE(apmItems[i - 1]->param1(), gimbalPitch);
            QVERIFY(apmItems[i - 2]->command() != MAV_CMD_DO_MOUNT_CONTROL);

            // The heading is square on to the leg, towards the structure
            const auto previousWaypoint =
                std::find_if(apmItems.crbegin() + (apmItems.count() - i), apmItems.crend(),
                             [](const MissionItem* item) { return item->command() == MAV_CMD_NAV_WAYPOINT; });
            const auto nextWaypoint = std::find_if(apmItems.cbegin() + i, apmItems.cend(), [](const MissionItem* item) {
                return item->command() == MAV_CMD_NAV_WAYPOINT;
            });
            QVERIFY(previousWaypoint != apmItems.crend());
            QVERIFY(nextWaypoint != apmItems.cend());
            const QGeoCoordinate legStart((*previousWaypoint)->param5(), (*previousWaypoint)->param6());
            const QGeoCoordinate legEnd((*nextWaypoint)->param5(), (*nextWaypoint)->param6());
            const double heading = apmItems[i]->param1();
            const double offLeg = std::fmod(heading - legStart.azimuthTo(legEnd) + 360.0, 360.0);
            QVERIFY(qAbs(offLeg - 90.0) < 0.01 || qAbs(offLeg - 270.0) < 0.01);
            const QGeoCoordinate legMiddle =
                legStart.atDistanceAndAzimuth(legStart.distanceTo(legEnd) / 2.0, legStart.azimuthTo(legEnd));
            QVERIFY(flightPolygon->containsCoordinate(legMiddle.atDistanceAndAzimuth(1.0, heading)));
            QCOMPARE(apmItems[i]->param4(), 0.0);
        }
    }
}

UT_REGISTER_TEST(StructureScanComplexItemTest, TestLabel::Unit, TestLabel::MissionManager)
