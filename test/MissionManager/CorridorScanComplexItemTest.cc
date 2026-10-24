#include "CorridorScanComplexItemTest.h"

#include <numbers>

#include <QtCore/QRegularExpression>
#include <QtTest/QSignalSpy>

#include "CoordFixtures.h"
#include "CorridorScanComplexItem.h"
#include "KMLPlanDomDocument.h"
#include "MultiSignalSpy.h"
#include "PlanViewSettings.h"
#include "TransectStyleComplexItem.h"

CorridorScanComplexItemTest::CorridorScanComplexItemTest()
{
    _polyLineVertices.append(TestFixtures::Coord::missionTestOrigin());
    _polyLineVertices.append(_polyLineVertices[0].atDistanceAndAzimuth(_corridorLineSegmentDistance, 0));
    _polyLineVertices.append(_polyLineVertices[1].atDistanceAndAzimuth(_corridorLineSegmentDistance, 20));
}

void CorridorScanComplexItemTest::init()
{
    TransectStyleComplexItemTestBase::init();
    _corridorItem = new CorridorScanComplexItem(planController(), false /* flyView */, QString() /* kmlOrShpFile */);
    _corridorItem->corridorPolyline()->appendVertices(_polyLineVertices);
    // Setup for expected transect count
    _corridorItem->corridorWidth()->setRawValue(_corridorWidth);
    _corridorItem->cameraCalc()->adjustedFootprintSide()->setRawValue((_corridorWidth * 0.5) + 1.0);
    _corridorItem->cameraCalc()->adjustedFootprintFrontal()->setRawValue(_corridorLineSegmentDistance * 0.25);
    int expectedTransectCount = _expectedTransectCount;
    QCOMPARE(_corridorItem->_transectCount(), expectedTransectCount);
    _corridorItem->setDirty(false);
    _multiSpyCorridorPolygon = std::make_unique<MultiSignalSpy>();
    QCOMPARE(_multiSpyCorridorPolygon->init(_corridorItem->surveyAreaPolygon()), true);
}

void CorridorScanComplexItemTest::cleanup()
{
    _multiSpyCorridorPolygon.reset();

    TransectStyleComplexItemTestBase::cleanup();
    // _corridorItem is deleted when planController() goes away
    _corridorItem = nullptr;
}

void CorridorScanComplexItemTest::_testDirty()
{
    Fact* fact = _corridorItem->corridorWidth();
    fact->setRawValue(fact->rawValue().toDouble() + 1);
    QVERIFY(_corridorItem->dirty());
    _corridorItem->setDirty(false);
    changeFactValue(_corridorItem->cameraCalc()->distanceToSurface());
    QVERIFY(_corridorItem->dirty());
    _corridorItem->setDirty(false);
    QGeoCoordinate coord = _corridorItem->corridorPolyline()->vertexCoordinate(0);
    coord.setLatitude(coord.latitude() + 1);
    _corridorItem->corridorPolyline()->adjustVertex(1, coord);
    QVERIFY_TRUE_WAIT(_corridorItem->dirty(), TestTimeout::mediumMs());
    _corridorItem->setDirty(false);
}

void CorridorScanComplexItemTest::_testSpecifiesCoordinateChanged()
{
    // Fresh item with an empty polyline so we can watch the 1 -> 2 vertex transition
    CorridorScanComplexItem* item = new CorridorScanComplexItem(planController(), false /* flyView */, QString() /* kmlOrShpFile */);

    MultiSignalSpy spy;
    QVERIFY(spy.init(item, QStringList{QStringLiteral("specifiesCoordinateChanged")}));

    QVERIFY(!item->specifiesCoordinate());

    item->corridorPolyline()->appendVertex(_polyLineVertices[0]);
    QVERIFY(!item->specifiesCoordinate());
    QVERIFY(spy.noneEmitted());

    item->corridorPolyline()->appendVertex(_polyLineVertices[1]);
    QVERIFY(item->specifiesCoordinate());
    QVERIFY(spy.emittedOnce("specifiesCoordinateChanged"));

    spy.clearAllSignals();
    item->corridorPolyline()->appendVertex(_polyLineVertices[2]);
    QVERIFY(item->specifiesCoordinate());
    QVERIFY(spy.noneEmitted());

    // Unrelated corridor rebuild triggers must not emit spuriously
    spy.clearAllSignals();
    changeFactValue(item->corridorWidth());
    QVERIFY(spy.noneEmitted());

    // true -> false transition when the polyline is cleared
    spy.clearAllSignals();
    item->corridorPolyline()->clear();
    QVERIFY(!item->specifiesCoordinate());
    QVERIFY(spy.emittedOnce("specifiesCoordinateChanged"));
}

void CorridorScanComplexItemTest::_waitForReadyForSave()
{
    QVERIFY_TRUE_WAIT(_corridorItem->readyForSaveState() == CorridorScanComplexItem::ReadyForSave,
                      TestTimeout::mediumMs());
}

void CorridorScanComplexItemTest::_testItemCount()
{
    struct TestCase_t {
        bool triggerInTurnAround;
        bool hasTurnaround;
    };

    static const TestCase_t rgTestCases[] = {
        {false, false},
        {true,  false},
        {false, true},
        {true,  true},
    };
    QList<MissionItem*> items;
    for (const TestCase_t& testCase : rgTestCases) {
        qCDebug(UnitTestLog) << "triggerInTurnAround:hasTurnaround" << testCase.triggerInTurnAround << testCase.hasTurnaround;
        _corridorItem->cameraTriggerInTurnAround()->setRawValue(testCase.triggerInTurnAround);
        _corridorItem->turnAroundDistance()->setRawValue(testCase.hasTurnaround ? 50 : 0);
        _corridorItem->appendMissionItems(items, this);
        QCOMPARE(items.count() - 1, _corridorItem->lastSequenceNumber());
        items.clear();
    }
}

void CorridorScanComplexItemTest::_testPathChanges()
{
    QGeoCoordinate vertex = _corridorItem->corridorPolyline()->vertexCoordinate(1);
    vertex.setLatitude(vertex.latitude() + 0.01);
    _corridorItem->corridorPolyline()->adjustVertex(1, vertex);
    QVERIFY_TRUE_WAIT(_multiSpyCorridorPolygon->emitted("pathChanged"), TestTimeout::mediumMs());
}

QList<MAV_CMD> CorridorScanComplexItemTest::_createExpectedCommands(bool hasTurnaround, bool useConditionGate)
{
    static const QList<MAV_CMD> singleFullTransect = {
        MAV_CMD_NAV_WAYPOINT,    // Turnaround
        MAV_CMD_CONDITION_GATE,  // Survey area entry edge
        MAV_CMD_DO_SET_CAM_TRIGG_DIST,
        MAV_CMD_NAV_WAYPOINT,    // Polyline turn
        MAV_CMD_CONDITION_GATE,  // Survey area exit edge
        MAV_CMD_DO_SET_CAM_TRIGG_DIST,
        MAV_CMD_NAV_WAYPOINT,    // Turnaround
    };
    QList<MAV_CMD> singleTransect = singleFullTransect;
    QList<MAV_CMD> expectedCommands;
    if (!useConditionGate) {
        for (MAV_CMD& cmd : singleTransect) {
            cmd = cmd == MAV_CMD_CONDITION_GATE ? MAV_CMD_NAV_WAYPOINT : cmd;
        }
    }
    if (!hasTurnaround) {
        singleTransect.takeFirst();
        singleTransect.takeLast();
    }
    for (int i = 0; i < _expectedTransectCount; i++) {
        expectedCommands.append(singleTransect);
    }
    return expectedCommands;
}

void CorridorScanComplexItemTest::_testItemGenerationWorker(bool imagesInTurnaround, bool hasTurnaround,
                                                            bool useConditionGate,
                                                            const QList<MAV_CMD>& expectedCommands)
{
    qCDebug(UnitTestLog) << QStringLiteral("_testItemGenerationWorker imagesInTurnaround:%1 turnaround:%2 gate:%3")
                    .arg(imagesInTurnaround)
                    .arg(hasTurnaround)
                    .arg(useConditionGate);
    _corridorItem->turnAroundDistance()->setRawValue(hasTurnaround ? 50 : 0);
    _corridorItem->cameraTriggerInTurnAround()->setRawValue(imagesInTurnaround);
    _planViewSettings->useConditionGate()->setRawValue(useConditionGate);
    QList<MissionItem*> items;
    _corridorItem->appendMissionItems(items, this);
    //_printItemCommands(items);
    QCOMPARE(items.count(), expectedCommands.count());
    for (int i = 0; i < expectedCommands.count(); i++) {
        int actualCommand = items[i]->command();
        int expectedCommand = expectedCommands[i];
        // qCDebug(UnitTestLog) << "Index" << i;
        QCOMPARE(actualCommand, expectedCommand);
    }
}

void CorridorScanComplexItemTest::_testItemGeneration()
{
    // Test all the combinations of: cameraTriggerInTurnAround: false, hasTurnAround: *, useConditionGate: *
    struct TestCase_t {
        bool hasTurnaround;
        bool useConditionGate;
    };

    static const TestCase_t rgTestCases[] = {
        {false, false},
        {false, true},
        {true, false},
        {true, true},
    };
    for (const TestCase_t& testCase : rgTestCases) {
        _testItemGenerationWorker(false /* imagesInTurnaround */, testCase.hasTurnaround, testCase.useConditionGate,
                                  _createExpectedCommands(testCase.hasTurnaround, testCase.useConditionGate));
    }
}

void CorridorScanComplexItemTest::_testCameraShots()
{
    // Triggering starts and stops with an image, so each 400m transect is imaged every 60m and at its end
    _corridorItem->cameraTriggerInTurnAround()->setRawValue(false);
    _corridorItem->cameraCalc()->adjustedFootprintFrontal()->setRawValue(60.0);
    QCOMPARE(_corridorItem->cameraShots(), _expectedTransectCount * 8);

    // Around a 20m square loop the inner pass cannot be offset and is not flown. The outer pass, about 180m
    // long, is imaged every 50m and at its end, where the 80m corridor path would give 3 images per pass.
    ignoreLogMessage("Plan.CorridorScanComplexItem", QtWarningMsg,
                     QRegularExpression("Unable to offset corridor transect"));
    const QGeoCoordinate origin = TestFixtures::Coord::missionTestOrigin();
    const QGeoCoordinate north = origin.atDistanceAndAzimuth(20.0, 0.0);
    const QGeoCoordinate northEast = north.atDistanceAndAzimuth(20.0, 90.0);
    const QGeoCoordinate east = origin.atDistanceAndAzimuth(20.0, 90.0);
    _corridorItem->corridorPolyline()->setPath(QList<QGeoCoordinate>{origin, north, northEast, east, origin});
    _corridorItem->cameraCalc()->adjustedFootprintFrontal()->setRawValue(50.0);
    QCOMPARE(_corridorItem->_transectCount(), 1);
    QCOMPARE(_corridorItem->cameraShots(), 5);
}

void CorridorScanComplexItemTest::_testMaxTransectCount()
{
    ignoreLogMessage("Plan.CorridorScanComplexItem", QtWarningMsg, QRegularExpression("Transect spacing.*raised"));
    // Tiny spacing triggers the cap: transect count must not exceed maxTransectCount.
    // cameraShotsChanged is emitted at the end of every _rebuildTransects(); the rebuild
    // fires synchronously (direct connection) so the spy count is already 1 on return.
    {
        QSignalSpy rebuildSpy(_corridorItem, &TransectStyleComplexItem::cameraShotsChanged);
        _corridorItem->cameraCalc()->adjustedFootprintSide()->setRawValue(0.001);
        QCOMPARE(rebuildSpy.count(), 1);
        QVERIFY(_corridorItem->_transectCount() <= TransectStyleComplexItem::maxTransectCount);
        QVERIFY(_corridorItem->_transectCount() > 0);
    }

    // Zero spacing must not crash and must fall back to a single center transect.
    {
        QSignalSpy rebuildSpy(_corridorItem, &TransectStyleComplexItem::cameraShotsChanged);
        _corridorItem->cameraCalc()->adjustedFootprintSide()->setRawValue(0);
        QCOMPARE(rebuildSpy.count(), 1);
        QCOMPARE(_corridorItem->_transectCount(), 1);
    }
}

void CorridorScanComplexItemTest::_testClosedAndCrossingCorridors()
{
    const QGeoCoordinate origin = TestFixtures::Coord::missionTestOrigin();
    const QGeoCoordinate north = origin.atDistanceAndAzimuth(_corridorLineSegmentDistance, 0.0);
    const QGeoCoordinate northEast = north.atDistanceAndAzimuth(_corridorLineSegmentDistance, 90.0);
    const QGeoCoordinate east = origin.atDistanceAndAzimuth(_corridorLineSegmentDistance, 90.0);
    const QGeoCoordinate center = origin.atDistanceAndAzimuth(_corridorLineSegmentDistance / std::numbers::sqrt2, 45.0);
    QGCMapPolyline* corridorPolyline = _corridorItem->corridorPolyline();
    QGCMapPolygon* corridorPolygon = _corridorItem->surveyAreaPolygon();

    // A closed loop keeps the interior hole of the buffer
    corridorPolyline->setPath(QList<QGeoCoordinate>{origin, north, northEast, east, origin});
    QVERIFY(corridorPolygon->isValid());
    QVERIFY(corridorPolygon->containsCoordinate(origin.atDistanceAndAzimuth(_corridorLineSegmentDistance / 2.0, 0.0)));
    QVERIFY(!corridorPolygon->containsCoordinate(center));
    QCOMPARE(_corridorItem->_transectCount(), _expectedTransectCount);

    // The outer boundary and the hole are drawn as separate closed outlines and exported as a polygon with a hole
    const QVariantList boundaries = _corridorItem->corridorBoundaries();
    QCOMPARE(boundaries.size(), 2);
    for (const QVariant& boundary : boundaries) {
        const QVariantList path = boundary.toList();
        QVERIFY(path.size() > 3);
        QCOMPARE(path.first().value<QGeoCoordinate>(), path.last().value<QGeoCoordinate>());
    }
    KMLPlanDomDocument kmlDocument;
    _corridorItem->addKMLVisuals(kmlDocument);
    QCOMPARE(kmlDocument.elementsByTagName(QStringLiteral("outerBoundaryIs")).count(), 1);
    QCOMPARE(kmlDocument.elementsByTagName(QStringLiteral("innerBoundaryIs")).count(), 1);

    // A self-crossing path still produces a single buffer and a full set of transects
    corridorPolyline->setPath(QList<QGeoCoordinate>{origin, northEast, north, east});
    QVERIFY(corridorPolygon->isValid());
    QVERIFY(corridorPolygon->containsCoordinate(center));
    QCOMPARE(_corridorItem->_transectCount(), _expectedTransectCount);
}

void CorridorScanComplexItemTest::_testNoTransects()
{
    // Small offsets of a closed figure-eight path split it in two, so none of its transects can be generated
    // even though the corridor area is valid
    const QGeoCoordinate origin = TestFixtures::Coord::missionTestOrigin();
    const QGeoCoordinate north = origin.atDistanceAndAzimuth(_corridorLineSegmentDistance, 0.0);
    const QGeoCoordinate east = origin.atDistanceAndAzimuth(_corridorLineSegmentDistance, 90.0);
    const QGeoCoordinate northEast = north.atDistanceAndAzimuth(_corridorLineSegmentDistance, 90.0);
    QCOMPARE(_corridorItem->readyForSaveState(), VisualMissionItem::ReadyForSave);

    QSignalSpy readySpy(_corridorItem, &VisualMissionItem::readyForSaveStateChanged);
    ignoreLogMessage("Plan.CorridorScanComplexItem", QtWarningMsg,
                     QRegularExpression("Unable to offset corridor transect"));
    expectAppMessage(QRegularExpression("Unable to calculate transects"));
    _corridorItem->corridorPolyline()->setPath(QList<QGeoCoordinate>{origin, northEast, east, north, origin});
    verifyExpectedLogMessage();

    QVERIFY(_corridorItem->surveyAreaPolygon()->isValid());
    QCOMPARE(_corridorItem->_transectCount(), 0);
    QCOMPARE(_corridorItem->readyForSaveState(), VisualMissionItem::NotReadyForSaveData);
    QVERIFY(readySpy.count() > 0);

    // Generating transects again restores readiness
    readySpy.clear();
    _corridorItem->corridorPolyline()->setPath(_polyLineVertices);
    QCOMPARE(_corridorItem->_transectCount(), _expectedTransectCount);
    QVERIFY(readySpy.count() > 0);
    _waitForReadyForSave();
}

void CorridorScanComplexItemTest::_testEntryPointCameraTriggers()
{
    // Camera triggering starts at the entry and stops at the exit of every transect, from whichever end the
    // scan starts (mavlink/qgroundcontrol#11758)
    _corridorItem->turnAroundDistance()->setRawValue(0.0);
    _corridorItem->cameraTriggerInTurnAround()->setRawValue(false);
    _planViewSettings->useConditionGate()->setRawValue(false);
    for (int entryLocation = 0; entryLocation < 4; ++entryLocation) {
        QList<MissionItem*> items;
        _corridorItem->appendMissionItems(items, this);

        QList<bool> triggerStarts;
        for (const MissionItem* item : items) {
            if (item->command() == MAV_CMD_DO_SET_CAM_TRIGG_DIST) {
                triggerStarts.append(item->param1() > 0.0);
            }
        }
        QCOMPARE(triggerStarts.size(), _expectedTransectCount * 2);
        for (qsizetype index = 0; index < triggerStarts.size(); ++index) {
            QCOMPARE(triggerStarts[index], (index % 2) == 0);
        }
        _corridorItem->rotateEntryPoint();
    }
}

void CorridorScanComplexItemTest::_testZeroWidthCorridor()
{
    // Zero width is a single pass along the path; the area must stay valid so the item can be saved
    ignoreLogMessage("Plan.CorridorScanComplexItem", QtWarningMsg,
                     QRegularExpression("Corridor width 0 is invalid, skipping transect count cap"));
    _corridorItem->corridorWidth()->setRawValue(0.0);

    QGCMapPolygon* corridorPolygon = _corridorItem->surveyAreaPolygon();
    QVERIFY(corridorPolygon->isValid());
    QCOMPARE(corridorPolygon->count(), _polyLineVertices.count() * 2);
    QCOMPARE(_corridorItem->_transectCount(), 1);
    _waitForReadyForSave();

    // The single pass outline is the path itself
    const QVariantList boundaries = _corridorItem->corridorBoundaries();
    QCOMPARE(boundaries.size(), 1);
    QCOMPARE(boundaries.first().toList().size(), _polyLineVertices.count());
}

void CorridorScanComplexItemTest::_testVisualTransectIndices()
{
    // Each transect follows the corridor path: entry, one interior point per inner path vertex, then exit.
    // This holds for every entry point location.
    for (int entryLocation = 0; entryLocation < 4; ++entryLocation) {
        const QList<int> entryIndices = _corridorItem->visualTransectEntryIndices();
        const QList<int> exitIndices = _corridorItem->visualTransectExitIndices();
        QCOMPARE(entryIndices.size(), _expectedTransectCount);
        QCOMPARE(exitIndices.size(), _expectedTransectCount);
        for (qsizetype index = 0; index < entryIndices.size(); ++index) {
            QCOMPARE(exitIndices[index] - entryIndices[index], _polyLineVertices.count() - 1);
        }
        _corridorItem->rotateEntryPoint();
    }
}

UT_REGISTER_TEST(CorridorScanComplexItemTest, TestLabel::Unit, TestLabel::MissionManager)
