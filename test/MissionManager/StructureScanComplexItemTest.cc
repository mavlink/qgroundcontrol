#include "StructureScanComplexItemTest.h"

#include <numbers>

#include <QtCore/QJsonArray>
#include <QtCore/QRegularExpression>

#include "CoordFixtures.h"
#include "MultiSignalSpy.h"
#include "PlanMasterController.h"
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

UT_REGISTER_TEST(StructureScanComplexItemTest, TestLabel::Unit, TestLabel::MissionManager)
