#include "SurveyComplexItemTest.h"

#include <algorithm>
#include <cmath>
#include <limits>

#include <QtCore/QRegularExpression>
#include <QtGui/QPolygonF>
#include <QtTest/QSignalSpy>

#include "CoordFixtures.h"
#include "GeoFenceController.h"
#include "MultiSignalSpy.h"
#include "PlanMasterController.h"
#include "PlanViewSettings.h"
#include "QGCFenceCircle.h"
#include "QGCFencePolygon.h"
#include "QGCGeo.h"
#include "SurveyComplexItem.h"
#include "TransectStyleComplexItem.h"

namespace {

void compareWaypointMarkers(SurveyComplexItem* survey, const QList<MissionItem*>& items)
{
    QList<const MissionItem*> waypoints;
    for (const MissionItem* item : items) {
        if (item->command() == MAV_CMD_NAV_WAYPOINT || item->command() == MAV_CMD_CONDITION_GATE) {
            waypoints.append(item);
        }
    }
    const QVariantList markers = survey->waypointMarkers();
    QCOMPARE(markers.count(), waypoints.count());
    for (int i = 0; i < markers.count(); ++i) {
        const QVariantMap marker = markers[i].toMap();
        QCOMPARE(marker.value(QStringLiteral("sequenceNumber")).toInt(), waypoints[i]->sequenceNumber());
        QCOMPARE(marker.value(QStringLiteral("coordinate")).value<QGeoCoordinate>(), waypoints[i]->coordinate());
    }
}

}  // namespace

SurveyComplexItemTest::SurveyComplexItemTest()
{
    // We use a 100m by 100m square test polygon
    const double edgeDistance = 100;
    _polyVertices.append(TestFixtures::Coord::missionTestOrigin());
    _polyVertices.append(_polyVertices[0].atDistanceAndAzimuth(edgeDistance, 90));
    _polyVertices.append(_polyVertices[1].atDistanceAndAzimuth(edgeDistance, 180));
    _polyVertices.append(_polyVertices[2].atDistanceAndAzimuth(edgeDistance, -90.0));
}

void SurveyComplexItemTest::init()
{
    TransectStyleComplexItemTestBase::init();
    _surveyItem = new SurveyComplexItem(planController(), false /* flyView */, QString() /* kmlFile */);
    _mapPolygon = _surveyItem->surveyAreaPolygon();
    _mapPolygon->appendVertices(_polyVertices);
    QVERIFY(_surveyItem->cameraCalc()->isManualCamera());
    // Set grid spacing to match expected transect count
    double polyWidthDistance = _polyVertices[0].distanceTo(_polyVertices[1]);
    double polyHeightDistance = _polyVertices[0].distanceTo(_polyVertices[3]);
    _surveyItem->cameraCalc()->adjustedFootprintSide()->setRawValue((polyWidthDistance * 0.5) - 1.0);
    _surveyItem->cameraCalc()->adjustedFootprintFrontal()->setRawValue(polyHeightDistance * 0.25);
    _surveyItem->gridAngle()->setRawValue(0);
    int expectedTransectCount = _expectedTransectCount;
    QCOMPARE(_surveyItem->_transectCount(), expectedTransectCount);
    _surveyItem->setDirty(false);
    // It's important to check that the right signals are emitted at the right time since that drives ui change.
    // It's also important to check that things are not being over-signalled when they should not be, since that can
    // lead to incorrect ui or perf impact of uneeded signals propogating ui change.
    _multiSpy = std::make_unique<MultiSignalSpy>();
    QVERIFY(_multiSpy);
    QCOMPARE(_multiSpy->init(_surveyItem), true);
}

void SurveyComplexItemTest::cleanup()
{
    _multiSpy.reset();
    TransectStyleComplexItemTestBase::cleanup();
    // These items are deleted when planController() is deleted
    _surveyItem = nullptr;
}

void SurveyComplexItemTest::_testDirty()
{
    QVERIFY(!_surveyItem->dirty());
    _surveyItem->setDirty(false);
    QVERIFY(!_surveyItem->dirty());
    QVERIFY(_multiSpy->noneEmitted());
    _surveyItem->setDirty(true);
    QVERIFY(_surveyItem->dirty());
    QVERIFY(_multiSpy->onlyEmittedOnce("dirtyChanged"));
    QVERIFY(_multiSpy->argument<bool>("dirtyChanged"));
    _multiSpy->clearAllSignals();
    _surveyItem->setDirty(false);
    QVERIFY(!_surveyItem->dirty());
    QVERIFY(_multiSpy->onlyEmittedOnce("dirtyChanged"));
    _multiSpy->clearAllSignals();
    // These facts should set dirty when changed
    QList<Fact*> rgFacts;
    rgFacts << _surveyItem->gridAngle() << _surveyItem->flyAlternateTransects();
    for (Fact* fact : rgFacts) {
        qCDebug(UnitTestLog) << fact->name();
        QVERIFY(!_surveyItem->dirty());
        if (fact->typeIsBool()) {
            fact->setRawValue(!fact->rawValue().toBool());
        } else {
            fact->setRawValue(fact->rawValue().toDouble() + 1);
        }
        QVERIFY(_multiSpy->emittedOnce("dirtyChanged"));
        QVERIFY(_multiSpy->argument<bool>("dirtyChanged"));
        _surveyItem->setDirty(false);
        _multiSpy->clearAllSignals();
    }
    rgFacts.clear();
}

// Clamp expected grid angle from 0<->180. We don't care about opposite angles like 90/270
double SurveyComplexItemTest::_clampGridAngle180(double gridAngle)
{
    if (gridAngle >= 0.0) {
        if (gridAngle == 360.0) {
            gridAngle = 0.0;
        } else if (gridAngle >= 180.0) {
            gridAngle -= 180.0;
        }
    } else {
        if (gridAngle < -180.0) {
            gridAngle += 360.0;
        } else {
            gridAngle += 180.0;
        }
    }
    return gridAngle;
}

void SurveyComplexItemTest::_testGridAngle()
{
    for (double gridAngle = -360.0; gridAngle <= 360.0; gridAngle++) {
        _surveyItem->gridAngle()->setRawValue(gridAngle);
        QVariantList gridPoints = _surveyItem->visualTransectPoints();
        QGeoCoordinate firstTransectEntry = gridPoints[0].value<QGeoCoordinate>();
        QGeoCoordinate firstTransectExit = gridPoints[1].value<QGeoCoordinate>();
        double azimuth = firstTransectEntry.azimuthTo(firstTransectExit);
        // qCDebug(UnitTestLog) << gridAngle << azimuth << _clampGridAngle180(gridAngle) << _clampGridAngle180(azimuth);
        int clampGridAngle = qRound(_clampGridAngle180(gridAngle));
        int clampAzimuth = qRound(_clampGridAngle180(azimuth));
        QCOMPARE(clampGridAngle, clampAzimuth);
    }
}

void SurveyComplexItemTest::_testEntryLocation()
{
    for (double gridAngle = -360.0; gridAngle <= 360.0; gridAngle++) {
        _surveyItem->gridAngle()->setRawValue(gridAngle);
        // Validate that each entry location is unique
        QList<QGeoCoordinate> rgSeenEntryCoords;
        for (int rotateCount = 0; rotateCount < 3; rotateCount++) {
            _surveyItem->rotateEntryPoint();
            QVERIFY(!rgSeenEntryCoords.contains(_surveyItem->coordinate()));
            rgSeenEntryCoords << _surveyItem->coordinate();
        }
        _surveyItem->rotateEntryPoint();  // Rotate back for first entry point
        rgSeenEntryCoords.clear();
    }
}

void SurveyComplexItemTest::_testItemCount()
{
    struct TestCase_t {
        bool hoverAndCapture;
        bool triggerInTurnAround;
        bool refly90;
        bool hasTurnaround;
    };

    QList<TestCase_t> rgTestCases;
    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 2; j++) {
            for (int k = 0; k < 2; k++) {
                for (int l = 0; l < 2; l++) {
                    TestCase_t testCase;
                    testCase.hoverAndCapture = i;
                    testCase.triggerInTurnAround = j;
                    testCase.refly90 = k;
                    testCase.hasTurnaround = l;
                    rgTestCases.append(testCase);
                }
            }
        }
    }
    QList<MissionItem*> items;
    for (const TestCase_t& testCase : rgTestCases) {
        qCDebug(UnitTestLog) << "hoverAndCapture:triggerInTurnAround:refly90:hasTurnaround" << testCase.hoverAndCapture
                 << testCase.triggerInTurnAround << testCase.refly90 << testCase.hasTurnaround;
        _surveyItem->hoverAndCapture()->setRawValue(testCase.hoverAndCapture);
        _surveyItem->cameraTriggerInTurnAround()->setRawValue(testCase.triggerInTurnAround);
        _surveyItem->refly90Degrees()->setRawValue(testCase.refly90);
        _surveyItem->turnAroundDistance()->setRawValue(testCase.hasTurnaround ? 50 : 0);
        _surveyItem->appendMissionItems(items, this);
        QCOMPARE(items.count() - 1, _surveyItem->lastSequenceNumber());
        items.clear();
    }
}

QList<MAV_CMD> SurveyComplexItemTest::_createExpectedCommands(bool hasTurnaround, bool useConditionGate)
{
    static const QList<MAV_CMD> singleFullTransect = {
        MAV_CMD_NAV_WAYPOINT,    // Turnaround
        MAV_CMD_CONDITION_GATE,  // Survey area entry edge
        MAV_CMD_DO_SET_CAM_TRIGG_DIST,
        MAV_CMD_CONDITION_GATE,  // Survey area exit edge
        MAV_CMD_DO_SET_CAM_TRIGG_DIST, MAV_CMD_NAV_WAYPOINT,
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

void SurveyComplexItemTest::_testItemGenerationWorker(bool imagesInTurnaround, bool hasTurnaround,
                                                      bool useConditionGate, const QList<MAV_CMD>& expectedCommands)
{
    qCDebug(UnitTestLog) << QStringLiteral("_testItemGenerationWorker imagesInTurnaround:%1 turnaround:%2 gate:%3")
                    .arg(imagesInTurnaround)
                    .arg(hasTurnaround)
                    .arg(useConditionGate);
    _surveyItem->turnAroundDistance()->setRawValue(hasTurnaround ? 50 : 0);
    _surveyItem->cameraTriggerInTurnAround()->setRawValue(imagesInTurnaround);
    _planViewSettings->useConditionGate()->setRawValue(useConditionGate);
    QList<MissionItem*> items;
    _surveyItem->appendMissionItems(items, this);
    QCOMPARE(items.count(), expectedCommands.count());
    for (int i = 0; i < expectedCommands.count(); i++) {
        int actualCommand = items[i]->command();
        int expectedCommand = expectedCommands[i];
        QCOMPARE(actualCommand, expectedCommand);
    }
    compareWaypointMarkers(_surveyItem, items);
}

void SurveyComplexItemTest::_testItemGeneration()
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
    // Test cameraTriggerInTurnAround = true cases
    QList<MAV_CMD> imagesInTurnaroundWithTurnaroundDistanceCommands = {
        // Transect 1
        MAV_CMD_CONDITION_GATE,         // First turnaround
        MAV_CMD_DO_SET_CAM_TRIGG_DIST,
        MAV_CMD_CONDITION_GATE,         // Survey entry
        MAV_CMD_DO_SET_CAM_TRIGG_DIST,  // Survey entry also has trigger start
        MAV_CMD_NAV_WAYPOINT,           // Survey exit
        MAV_CMD_NAV_WAYPOINT,           // Turnaround
        // Transect 2
        MAV_CMD_NAV_WAYPOINT,           // Turnaround
        MAV_CMD_CONDITION_GATE,         // Survey entry
        MAV_CMD_DO_SET_CAM_TRIGG_DIST,  // Survey entry also has trigger start
        MAV_CMD_NAV_WAYPOINT,           // Survey exit
        MAV_CMD_CONDITION_GATE,         // Final turnaround
        MAV_CMD_DO_SET_CAM_TRIGG_DIST,
    };
    _testItemGenerationWorker(true /* imagesInTurnaround */, true /* hasTurnaround */, true /* useConditionGate */,
                              imagesInTurnaroundWithTurnaroundDistanceCommands);
    // Switch to non CONDITION_GATE usage
    for (MAV_CMD& cmd : imagesInTurnaroundWithTurnaroundDistanceCommands) {
        cmd = cmd == MAV_CMD_CONDITION_GATE ? MAV_CMD_NAV_WAYPOINT : cmd;
    }
    _testItemGenerationWorker(true /* imagesInTurnaround */, true /* hasTurnaround */, false /* useConditionGate */,
                              imagesInTurnaroundWithTurnaroundDistanceCommands);
    QList<MAV_CMD> imagesInTurnaroundWithoutTurnaroundDistanceCommands = {
        // Transect 1
        MAV_CMD_CONDITION_GATE,         // Survey entry
        MAV_CMD_DO_SET_CAM_TRIGG_DIST,  // Camera trigger start for entire survey
        MAV_CMD_NAV_WAYPOINT,           // Survey exit
        // Transect 2
        MAV_CMD_CONDITION_GATE,         // Survey entry
        MAV_CMD_DO_SET_CAM_TRIGG_DIST,  // Survey entry also has trigger start
        MAV_CMD_CONDITION_GATE,         // Survey exit
        MAV_CMD_DO_SET_CAM_TRIGG_DIST,  // Camera trigger stop for entire survey
    };
    _testItemGenerationWorker(true /* imagesInTurnaround */, false /* hasTurnaround */, true /* useConditionGate */,
                              imagesInTurnaroundWithoutTurnaroundDistanceCommands);
    // Switch to non CONDITION_GATE usage
    for (MAV_CMD& cmd : imagesInTurnaroundWithoutTurnaroundDistanceCommands) {
        cmd = cmd == MAV_CMD_CONDITION_GATE ? MAV_CMD_NAV_WAYPOINT : cmd;
    }
    _testItemGenerationWorker(true /* imagesInTurnaround */, false /* hasTurnaround */, false /* useConditionGate */,
                              imagesInTurnaroundWithoutTurnaroundDistanceCommands);
}

void SurveyComplexItemTest::_testHoverCaptureItemGeneration()
{
    static const QList<MAV_CMD> singleFullTransect = {
        MAV_CMD_NAV_WAYPOINT,  // Turnaround
        MAV_CMD_NAV_WAYPOINT,  // Survey area entry edge
        MAV_CMD_IMAGE_START_CAPTURE,
        MAV_CMD_NAV_WAYPOINT,  // Interior trigger
        MAV_CMD_IMAGE_START_CAPTURE,
        MAV_CMD_NAV_WAYPOINT,  // Interior trigger
        MAV_CMD_IMAGE_START_CAPTURE,
        MAV_CMD_NAV_WAYPOINT,  // Survey area exit edge
        MAV_CMD_IMAGE_START_CAPTURE,
        MAV_CMD_NAV_WAYPOINT,  // Turnaround
    };
    QList<MAV_CMD> expectedCommands;
    for (int i = 0; i < _expectedTransectCount; i++) {
        expectedCommands.append(singleFullTransect);
    }
    // Set trigger distance to generates two interior capture points
    double polyHeightDistance = _polyVertices[0].distanceTo(_polyVertices[3]);
    double triggerDistance = (polyHeightDistance / 3.0) + 1.0;
    _surveyItem->cameraCalc()->adjustedFootprintFrontal()->setRawValue(triggerDistance);
    _surveyItem->hoverAndCapture()->setRawValue(true);
    _testItemGenerationWorker(false /* imagesInTurnaround */, true /* hasTurnaround */, true /* useConditionGate */,
                              expectedCommands);
    _testItemGenerationWorker(false /* imagesInTurnaround */, true /* hasTurnaround */, false /* useConditionGate */,
                              expectedCommands);
}

void SurveyComplexItemTest::_testMaxTransectCount()
{
    ignoreLogMessage("Plan.SurveyComplexItem", QtWarningMsg, QRegularExpression("Transect spacing.*raised"));
    ignoreLogMessage("Plan.SurveyComplexItem", QtWarningMsg, QRegularExpression("Grid spacing 0 is invalid"));
    // Tiny spacing triggers the cap: transect count must not exceed maxTransectCount.
    // cameraShotsChanged is emitted at the end of every _rebuildTransects(); the rebuild
    // fires synchronously (direct connection) so the spy count is already 1 on return.
    {
        QSignalSpy rebuildSpy(_surveyItem, &TransectStyleComplexItem::cameraShotsChanged);
        _surveyItem->cameraCalc()->adjustedFootprintSide()->setRawValue(0.001);
        QCOMPARE(rebuildSpy.count(), 1);
        QVERIFY(_surveyItem->_transectCount() <= TransectStyleComplexItem::maxTransectCount);
        QVERIFY(_surveyItem->_transectCount() > 0);
    }

    // Zero spacing must not crash and must fall back to a single center transect.
    {
        QSignalSpy rebuildSpy(_surveyItem, &TransectStyleComplexItem::cameraShotsChanged);
        _surveyItem->cameraCalc()->adjustedFootprintSide()->setRawValue(0);
        QCOMPARE(rebuildSpy.count(), 1);
        QCOMPARE(_surveyItem->_transectCount(), 1);
    }
}

namespace {

QGeoCoordinate surveyCoordinate(const QGeoCoordinate& origin, double east, double north)
{
    QGeoCoordinate coordinate;
    QGCGeo::convertNedToGeo(north, east, 0, origin, coordinate);
    return coordinate;
}

QList<QGeoCoordinate> obstacleVertices(const QGeoCoordinate& origin, const QRectF& bounds)
{
    return {surveyCoordinate(origin, bounds.left(), bounds.top()),
            surveyCoordinate(origin, bounds.right(), bounds.top()),
            surveyCoordinate(origin, bounds.right(), bounds.bottom()),
            surveyCoordinate(origin, bounds.left(), bounds.bottom())};
}

QPointF localSurveyPoint(const QGeoCoordinate& coordinate, const QGeoCoordinate& origin)
{
    double north, east, down;
    QGCGeo::convertGeoToNed(coordinate, origin, north, east, down);
    return QPointF(east, north);
}

double distanceToSurveySegment(const QPointF& point, const QLineF& segment)
{
    const QPointF direction = segment.p2() - segment.p1();
    const double lengthSquared = QPointF::dotProduct(direction, direction);
    const double t = lengthSquared > 0
                         ? std::clamp(QPointF::dotProduct(point - segment.p1(), direction) / lengthSquared, 0.0, 1.0)
                         : 0;
    return QLineF(point, segment.pointAt(t)).length();
}

QList<QLineF> surveyCoverageSegments(const QList<MissionItem*>& items, const QGeoCoordinate& origin)
{
    QList<QLineF> segments;
    QPointF position;
    QPointF entry;
    bool capturing = false;
    for (const MissionItem* item : items) {
        if (item->command() == MAV_CMD_NAV_WAYPOINT || item->command() == MAV_CMD_CONDITION_GATE) {
            position = localSurveyPoint(QGeoCoordinate(item->param5(), item->param6()), origin);
        } else if (item->command() == MAV_CMD_DO_SET_CAM_TRIGG_DIST) {
            if (item->param1() > 0) {
                entry = position;
                capturing = true;
            } else if (capturing) {
                segments.append(QLineF(entry, position));
                capturing = false;
            }
        }
    }
    return segments;
}

}  // namespace

void SurveyComplexItemObstacleTest::init()
{
    TransectStyleComplexItemTestBase::init();
    const QGeoCoordinate origin = TestFixtures::Coord::missionTestOrigin();
    _polyVertices = {origin, surveyCoordinate(origin, 100, 0), surveyCoordinate(origin, 100, -100),
                     surveyCoordinate(origin, 0, -100)};
    // Survey polygons persist latitude/longitude only.
    for (QGeoCoordinate& vertex : _polyVertices) {
        vertex.setAltitude(qQNaN());
    }
    _surveyItem = new SurveyComplexItem(planController(), false, QString());
    _surveyItem->turnAroundDistance()->setRawValue(0);
    _surveyItem->cameraCalc()->adjustedFootprintSide()->setRawValue(49);
    _surveyItem->cameraCalc()->adjustedFootprintFrontal()->setRawValue(25);
    _surveyItem->gridAngle()->setRawValue(0);
    _surveyItem->surveyAreaPolygon()->appendVertices(_polyVertices);
    QCOMPARE(_surveyItem->_transectCount(), 2);
    _surveyItem->setDirty(false);
}

void SurveyComplexItemObstacleTest::cleanup()
{
    TransectStyleComplexItemTestBase::cleanup();
    _surveyItem = nullptr;
}

void SurveyComplexItemObstacleTest::_testObstacleAvoidance_data()
{
    QTest::addColumn<bool>("circle");
    QTest::addColumn<bool>("hover");
    QTest::addColumn<bool>("refly");
    QTest::addColumn<double>("turnaround");
    QTest::addColumn<double>("angle");
    QTest::addColumn<bool>("imagesInTurnarounds");
    QTest::addColumn<bool>("alternate");
    QTest::newRow("polygon") << false << false << false << 0.0 << 0.0 << false << false;
    QTest::newRow("circle") << true << false << false << 0.0 << 0.0 << false << false;
    QTest::newRow("rotated-circle") << true << false << false << 0.0 << 33.0 << false << false;
    QTest::newRow("rotated-circle-117") << true << false << false << 0.0 << 117.0 << false << false;
    QTest::newRow("hover") << false << true << false << 0.0 << 33.0 << false << false;
    QTest::newRow("refly") << false << false << true << 0.0 << 0.0 << false << false;
    QTest::newRow("turnaround") << false << false << false << 25.0 << 90.0 << false << false;
    QTest::newRow("circle-refly-turnaround") << true << false << true << 25.0 << 33.0 << false << false;
    QTest::newRow("images-in-turnarounds") << true << false << false << 25.0 << 0.0 << true << false;
    QTest::newRow("alternate") << false << false << false << 25.0 << 0.0 << false << true;
}

void SurveyComplexItemObstacleTest::_testObstacleAvoidance()
{
    QFETCH(bool, circle);
    QFETCH(bool, hover);
    QFETCH(bool, refly);
    QFETCH(double, turnaround);
    QFETCH(double, angle);
    QFETCH(bool, imagesInTurnarounds);
    QFETCH(bool, alternate);
    _surveyItem->turnAroundDistance()->setRawValue(turnaround);
    _surveyItem->hoverAndCapture()->setRawValue(hover);
    _surveyItem->refly90Degrees()->setRawValue(refly);
    _surveyItem->gridAngle()->setRawValue(angle);
    _surveyItem->cameraCalc()->adjustedFootprintSide()->setRawValue(20.0);
    _surveyItem->cameraTriggerInTurnAround()->setRawValue(imagesInTurnarounds);
    _surveyItem->flyAlternateTransects()->setRawValue(alternate);
    const int originalTransects = _surveyItem->_transectCount();
    const QGeoCoordinate origin = _polyVertices.first();
    QList<MissionItem*> originalItems;
    if (!hover && !imagesInTurnarounds) {
        _surveyItem->appendMissionItems(originalItems, this);
    }
    const QList<QLineF> originalCoverage = surveyCoverageSegments(originalItems, origin);
    QPolygonF field;
    for (const QGeoCoordinate& vertex : _polyVertices) {
        double north, east, down;
        QGCGeo::convertGeoToNed(vertex, origin, north, east, down);
        field.append(QPointF(east, north));
    }
    const QGeoCoordinate center = surveyCoordinate(origin, 50, -50);
    GeoFenceController* fences = planController()->geoFenceController();
    if (circle) {
        fences->circles()->append(new QGCFenceCircle(center, 15, false, fences));
    } else {
        auto* polygon = new QGCFencePolygon(false, fences);
        polygon->appendVertices(obstacleVertices(origin, QRectF(35, -65, 30, 30)));
        fences->polygons()->append(polygon);
    }

    QVERIFY(_surveyItem->dirty());
    QCOMPARE(_surveyItem->readyForSaveState(), VisualMissionItem::ReadyForSave);
    QVERIFY(_surveyItem->_transectCount() > originalTransects);
    QList<MissionItem*> items;
    _surveyItem->setSequenceNumber(7);
    _surveyItem->appendMissionItems(items, this);
    QCOMPARE(_surveyItem->sequenceNumber() + items.count() - 1, _surveyItem->lastSequenceNumber());
    compareWaypointMarkers(_surveyItem, items);
    const QVariantList markers = _surveyItem->waypointMarkers();
    QSignalSpy markerSpy(_surveyItem, &SurveyComplexItem::waypointMarkersChanged);
    _surveyItem->setSequenceNumber(18);
    QCOMPARE(markerSpy.count(), 1);
    const QVariantList renumberedMarkers = _surveyItem->waypointMarkers();
    QCOMPARE(renumberedMarkers.count(), markers.count());
    for (int i = 0; i < markers.count(); ++i) {
        QCOMPARE(renumberedMarkers[i].toMap().value(QStringLiteral("sequenceNumber")).toInt(),
                 markers[i].toMap().value(QStringLiteral("sequenceNumber")).toInt() + 11);
        QCOMPARE(renumberedMarkers[i].toMap().value(QStringLiteral("coordinate")),
                 markers[i].toMap().value(QStringLiteral("coordinate")));
    }
    QList<QGeoCoordinate> waypoints;
    int captures = 0;
    double lastTriggerDistance = -1;
    bool capturing = false;
    int transitionWaypoints = 0;
    for (const MissionItem* item : items) {
        QVERIFY(item->command() != MAV_CMD_CONDITION_GATE);
        if (item->command() == MAV_CMD_NAV_WAYPOINT) {
            waypoints.append(QGeoCoordinate(item->param5(), item->param6()));
            if (!capturing) {
                ++transitionWaypoints;
            }
        } else if (item->command() == MAV_CMD_IMAGE_START_CAPTURE) {
            ++captures;
        } else if (item->command() == MAV_CMD_DO_SET_CAM_TRIGG_DIST) {
            lastTriggerDistance = item->param1();
            if (circle && !imagesInTurnarounds && lastTriggerDistance > 0) {
                QVERIFY2(transitionWaypoints <= 10, "Circle detour contains too many short waypoint segments");
            }
            capturing = lastTriggerDistance > 0;
            transitionWaypoints = 0;
        }
    }
    QVERIFY(waypoints.count() > 4);
    const QVariantList visualPoints = _surveyItem->visualTransectPoints();
    QCOMPARE(waypoints.count(), visualPoints.count());
    for (int i = 0; i < waypoints.count(); ++i) {
        const QGeoCoordinate visualPoint = visualPoints[i].value<QGeoCoordinate>();
        QCOMPARE(waypoints[i].latitude(), visualPoint.latitude());
        QCOMPARE(waypoints[i].longitude(), visualPoint.longitude());
    }
    if (hover) {
        QCOMPARE(captures, _surveyItem->cameraShots());
        QCOMPARE(_surveyItem->additionalTimeDelay(), captures * 4.0);
    } else {
        QCOMPARE(lastTriggerDistance, 0.0);
    }
    if (!originalCoverage.isEmpty()) {
        const QList<QLineF> coverage = surveyCoverageSegments(items, origin);
        int previousRow = 0;
        bool reflyStarted = false;
        for (const QLineF& segment : coverage) {
            int row = -1;
            for (int i = 0; i < originalCoverage.count(); ++i) {
                if (distanceToSurveySegment(segment.p1(), originalCoverage[i]) < 0.05 &&
                    distanceToSurveySegment(segment.p2(), originalCoverage[i]) < 0.05) {
                    row = i;
                    break;
                }
            }
            QVERIFY2(row >= 0, "Obstacle avoidance changes the survey coverage lines");
            if (alternate) {
                QVERIFY2(row >= previousRow, "Obstacle avoidance changes the alternate transect order");
                QVERIFY2(QPointF::dotProduct(segment.p2() - segment.p1(),
                                             originalCoverage[row].p2() - originalCoverage[row].p1()) > 0,
                         "A split alternate transect reverses the survey direction");
            }
            const QLineF& first = originalCoverage.first();
            const bool reflySegment =
                std::abs(QPointF::dotProduct(segment.p2() - segment.p1(), first.p2() - first.p1())) <
                0.5 * segment.length() * first.length();
            QVERIFY2(!reflyStarted || reflySegment, "Primary and refly coverage sweeps are interleaved");
            reflyStarted = reflyStarted || reflySegment;
            previousRow = row;
        }
        // Independently sample the original coverage lines outside the expanded exclusions.
        for (const QLineF& original : originalCoverage) {
            const int steps = qMax(1, qCeil(original.length() / 0.25));
            for (int step = 0; step <= steps; ++step) {
                const QPointF point = original.pointAt(double(step) / steps);
                double boundaryDistance;
                if (circle) {
                    boundaryDistance = -std::numeric_limits<double>::infinity();
                    for (int side = 0; side < 8; ++side) {
                        const double normalAngle = (side * 45.0 - angle) * M_PI / 180;
                        const QPointF normal(std::cos(normalAngle), std::sin(normalAngle));
                        boundaryDistance =
                            qMax(boundaryDistance, QPointF::dotProduct(point - QPointF(50, -50), normal) - 17.1);
                    }
                } else {
                    boundaryDistance =
                        qMax(qMax(32.9 - point.x(), point.x() - 67.1), qMax(-67.1 - point.y(), point.y() + 32.9));
                }
                if (std::abs(boundaryDistance) < 0.05) {
                    continue;
                }
                int coveringSegments = 0;
                for (const QLineF& segment : coverage) {
                    const bool parallel =
                        std::abs(QPointF::dotProduct(segment.p2() - segment.p1(), original.p2() - original.p1())) >
                        0.5 * segment.length() * original.length();
                    if (parallel && distanceToSurveySegment(point, segment) < 0.05) {
                        ++coveringSegments;
                    }
                }
                QCOMPARE(coveringSegments, boundaryDistance > 0 ? 1 : 0);
            }
        }
    }
    // Check the segments actually flown, including turns and transitions, independently of the planner.
    for (int i = 1; i < waypoints.count(); ++i) {
        const double distance = waypoints[i - 1].distanceTo(waypoints[i]);
        const double azimuth = waypoints[i - 1].azimuthTo(waypoints[i]);
        const int steps = qMax(1, qCeil(distance / 0.25));
        for (int step = 0; step <= steps; ++step) {
            const QGeoCoordinate point = waypoints[i - 1].atDistanceAndAzimuth(distance * step / steps, azimuth);
            double north, east, down;
            QGCGeo::convertGeoToNed(point, origin, north, east, down);
            if (turnaround == 0) {
                const QPointF localPoint(east, north);
                double edgeDistance = std::numeric_limits<double>::infinity();
                for (int edge = 0; edge < field.count(); ++edge) {
                    const QPointF start = field[edge];
                    const QPointF direction = field[(edge + 1) % field.count()] - start;
                    const double t = std::clamp(
                        QPointF::dotProduct(localPoint - start, direction) / QPointF::dotProduct(direction, direction),
                        0.0, 1.0);
                    edgeDistance = qMin(edgeDistance, QLineF(localPoint, start + direction * t).length());
                }
                QVERIFY2(field.containsPoint(localPoint, Qt::OddEvenFill) || edgeDistance < 0.05,
                         "Flight segment leaves the survey boundary");
            }
            if (circle) {
                QVERIFY2(point.distanceTo(center) >= 16.95, "Flight segment enters the circle clearance");
            } else {
                const double dx = qMax(qMax(35.0 - east, east - 65.0), 0.0);
                const double dy = qMax(qMax(-65.0 - north, north + 35.0), 0.0);
                QVERIFY2(std::hypot(dx, dy) >= 1.95, "Flight segment enters the polygon clearance");
            }
        }
    }
}

void SurveyComplexItemObstacleTest::_testObstacleRouteEfficiency_data()
{
    QTest::addColumn<int>("entryPoint");
    QTest::addColumn<double>("spacing");
    QTest::addColumn<double>("maximumDistance");
    for (int entry = 0; entry < 4; ++entry) {
        QTest::newRow(qPrintable(QStringLiteral("entry-%1").arg(entry))) << entry << 10.0 << 1100.0;
    }
    QTest::newRow("dense") << 0 << 1.0 << 10000.0;
}

void SurveyComplexItemObstacleTest::_testObstacleRouteEfficiency()
{
    QFETCH(int, entryPoint);
    QFETCH(double, spacing);
    QFETCH(double, maximumDistance);
    _surveyItem->cameraTriggerInTurnAround()->setRawValue(false);
    _surveyItem->hoverAndCapture()->setRawValue(false);
    _surveyItem->refly90Degrees()->setRawValue(false);
    _surveyItem->flyAlternateTransects()->setRawValue(false);
    _surveyItem->cameraCalc()->adjustedFootprintSide()->setRawValue(spacing);
    for (int entry = 0; entry < entryPoint; ++entry) {
        _surveyItem->rotateEntryPoint();
    }
    const QGeoCoordinate entry = _surveyItem->coordinate();
    const QGeoCoordinate exit = _surveyItem->exitCoordinate();
    const QGeoCoordinate origin = _polyVertices.first();
    QList<MissionItem*> originalItems;
    _surveyItem->appendMissionItems(originalItems, this);
    int segmentCount = 0;
    double expectedCoverage = 0;
    for (const QLineF& segment : surveyCoverageSegments(originalItems, origin)) {
        const bool crossesBarrier = segment.x1() > 7.9 && segment.x1() < 92.1;
        segmentCount += crossesBarrier ? 2 : 1;
        // The 20 m barrier plus clearance removes 24.2 m from each crossing line.
        expectedCoverage += segment.length() - (crossesBarrier ? 24.2 : 0);
    }
    GeoFenceController* fences = planController()->geoFenceController();
    auto* polygon = new QGCFencePolygon(false, fences);
    polygon->appendVertices(obstacleVertices(origin, QRectF(10, -60, 80, 20)));
    fences->polygons()->append(polygon);

    QCOMPARE(_surveyItem->readyForSaveState(), VisualMissionItem::ReadyForSave);
    QVERIFY(_surveyItem->coordinate().distanceTo(entry) < 0.01);
    QVERIFY(_surveyItem->exitCoordinate().distanceTo(exit) < 0.01);
    QList<MissionItem*> items;
    _surveyItem->appendMissionItems(items, this);
    const QList<QLineF> coverage = surveyCoverageSegments(items, origin);
    QCOMPARE(coverage.count(), segmentCount);
    double coverageLength = 0;
    for (const QLineF& segment : coverage) {
        coverageLength += segment.length();
    }
    QVERIFY(std::abs(coverageLength - expectedCoverage) < 0.05);
    double distance = 0;
    QGeoCoordinate previous;
    for (const MissionItem* item : items) {
        if (item->command() == MAV_CMD_NAV_WAYPOINT) {
            const QGeoCoordinate coordinate = item->coordinate();
            if (previous.isValid()) {
                distance += previous.distanceTo(coordinate);
                const QLineF flight(localSurveyPoint(previous, origin), localSurveyPoint(coordinate, origin));
                const int steps = qMax(1, qCeil(flight.length() / 0.25));
                for (int step = 0; step <= steps; ++step) {
                    const QPointF point = flight.pointAt(double(step) / steps);
                    const double dx = qMax(qMax(10.0 - point.x(), point.x() - 90.0), 0.0);
                    const double dy = qMax(qMax(-60.0 - point.y(), point.y() + 40.0), 0.0);
                    QVERIFY2(std::hypot(dx, dy) >= 1.95, "Optimized route enters the barrier clearance");
                    QVERIFY(point.x() >= -0.05 && point.x() <= 100.05);
                    QVERIFY(point.y() >= -100.05 && point.y() <= 0.05);
                }
            }
            previous = coordinate;
        }
    }
    QVERIFY2(distance < maximumDistance, qPrintable(QStringLiteral("Survey route is %1 m").arg(distance)));
}

void SurveyComplexItemObstacleTest::_testObstacleChanges()
{
    _surveyItem->turnAroundDistance()->setRawValue(0);
    const QVariantList originalPoints = _surveyItem->visualTransectPoints();
    GeoFenceController* fences = planController()->geoFenceController();
    auto* polygon = new QGCFencePolygon(true, fences);
    polygon->appendVertices(obstacleVertices(_polyVertices.first(), QRectF(30, -65, 35, 30)));
    fences->polygons()->append(polygon);
    QCOMPARE(_surveyItem->visualTransectPoints(), originalPoints);

    polygon->setInclusion(false);
    QVERIFY(_surveyItem->visualTransectPoints() != originalPoints);
    // Changes must still rebuild after the fence and its model are already dirty.
    const QVariantList obstaclePoints = _surveyItem->visualTransectPoints();
    polygon->setCenter(surveyCoordinate(_polyVertices.first(), 50, -25));
    QTRY_VERIFY_WITH_TIMEOUT(_surveyItem->visualTransectPoints() != obstaclePoints, TestTimeout::shortMs());
    polygon->setInclusion(true);
    QCOMPARE(_surveyItem->visualTransectPoints(), originalPoints);
    polygon->setInclusion(false);
    fences->deletePolygon(0);
    QCOMPARE(_surveyItem->visualTransectPoints(), originalPoints);

    auto* circle = new QGCFenceCircle(surveyCoordinate(_polyVertices.first(), 42, -50), 10, false, fences);
    fences->circles()->append(circle);
    const QVariantList circlePoints = _surveyItem->visualTransectPoints();
    circle->radius()->setRawValue(20);
    QVERIFY(_surveyItem->visualTransectPoints() != circlePoints);
    circle->setCenter(surveyCoordinate(_polyVertices.first(), 500, -50));
    QCOMPARE(_surveyItem->visualTransectPoints(), originalPoints);
    fences->deleteCircle(0);
    QCOMPARE(_surveyItem->visualTransectPoints(), originalPoints);
}

void SurveyComplexItemObstacleTest::_testBlockedSurvey_data()
{
    QTest::addColumn<QRectF>("obstacle");
    QTest::newRow("disconnected") << QRectF(48, -110, 4, 120);
    QTest::newRow("fully-covered") << QRectF(-10, -110, 120, 120);
}

void SurveyComplexItemObstacleTest::_testBlockedSurvey()
{
    QFETCH(QRectF, obstacle);
    _surveyItem->turnAroundDistance()->setRawValue(0);
    GeoFenceController* fences = planController()->geoFenceController();
    auto* polygon = new QGCFencePolygon(false, fences);
    polygon->appendVertices(obstacleVertices(_polyVertices.first(), obstacle));
    fences->polygons()->append(polygon);
    QCOMPARE(_surveyItem->readyForSaveState(), VisualMissionItem::NotReadyForSaveData);
    QVERIFY(!_surveyItem->obstacleError().isEmpty());
    QVERIFY(_surveyItem->visualTransectPoints().isEmpty());
    QList<MissionItem*> items;
    _surveyItem->appendMissionItems(items, this);
    QVERIFY(items.isEmpty());
    QVERIFY(_surveyItem->waypointMarkers().isEmpty());
    fences->deletePolygon(0);
    QCOMPARE(_surveyItem->readyForSaveState(), VisualMissionItem::ReadyForSave);
    QVERIFY(_surveyItem->obstacleError().isEmpty());
    QVERIFY(!_surveyItem->visualTransectPoints().isEmpty());
}

void SurveyComplexItemObstacleTest::_testObstacleLoad()
{
    _surveyItem->turnAroundDistance()->setRawValue(0);
    QJsonArray savedItems;
    _surveyItem->save(savedItems);
    QString error;
    SurveyComplexItem loaded(planController(), false, QString());
    QVERIFY2(loaded.load(savedItems.first().toObject(), 19, error), qPrintable(error));
    QList<MissionItem*> loadedItems;
    loaded.appendMissionItems(loadedItems, this);
    compareWaypointMarkers(&loaded, loadedItems);
    QCOMPARE(loaded.waypointMarkers().first().toMap().value(QStringLiteral("sequenceNumber")).toInt(), 19);
    GeoFenceController* fences = planController()->geoFenceController();
    auto* polygon = new QGCFencePolygon(false, fences);
    polygon->appendVertices(obstacleVertices(_polyVertices.first(), QRectF(30, -65, 35, 30)));
    fences->polygons()->append(polygon);
    const QVariantList obstaclePoints = _surveyItem->visualTransectPoints();
    QVERIFY2(_surveyItem->load(savedItems.first().toObject(), 0, error), qPrintable(error));
    QCOMPARE(_surveyItem->visualTransectPoints(), obstaclePoints);
}

UT_REGISTER_TEST(SurveyComplexItemTest, TestLabel::Unit, TestLabel::MissionManager)
UT_REGISTER_TEST(SurveyComplexItemObstacleTest, TestLabel::Unit, TestLabel::MissionManager)
