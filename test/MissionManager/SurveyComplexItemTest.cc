#include "SurveyComplexItemTest.h"

#include <algorithm>
#include <cmath>

#include <QtCore/QJsonArray>
#include <QtCore/QRegularExpression>
#include <QtTest/QSignalSpy>

#include "CoordFixtures.h"
#include "JsonParsing.h"
#include "MissionItem.h"
#include "MultiSignalSpy.h"
#include "PlanViewSettings.h"
#include "QGCGeo.h"
#include "SurveyComplexItem.h"
#include "TransectStyleComplexItem.h"

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
    rgFacts << _surveyItem->gridAngle() << _surveyItem->flyAlternateTransects() << _surveyItem->splitConcavePolygons();
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

void SurveyComplexItemTest::_testCameraShots()
{
    // Triggering starts and stops with an image, so each 100m transect is imaged at 0, 30, 60, 90 and 100m
    _surveyItem->cameraTriggerInTurnAround()->setRawValue(false);
    _surveyItem->cameraCalc()->adjustedFootprintFrontal()->setRawValue(30.0);
    const int expectedShots = _expectedTransectCount * 5;
    QCOMPARE(_surveyItem->cameraShots(), expectedShots);

    // A survey loaded from a plan without an image count counts images from its saved mission items, since
    // transects are not rebuilt while loading
    QJsonArray items;
    _surveyItem->save(items);
    QJsonObject surveyObject = items[0].toObject();
    QJsonObject transectObject = surveyObject[QStringLiteral("TransectStyleComplexItem")].toObject();
    QVERIFY(transectObject.contains(QStringLiteral("CameraShots")));
    transectObject[QStringLiteral("CameraShots")] = 0;
    surveyObject[QStringLiteral("TransectStyleComplexItem")] = transectObject;
    QString errorString;
    SurveyComplexItem* loadedItem =
        new SurveyComplexItem(planController(), false /* flyView */, QString() /* kmlFile */);
    QVERIFY(loadedItem->load(surveyObject, 10, errorString));
    QCOMPARE(loadedItem->cameraShots(), expectedShots);
    loadedItem->deleteLater();

    // Hover and capture images the survey entry, exit and each interior hover point
    _surveyItem->hoverAndCapture()->setRawValue(true);
    QList<MissionItem*> missionItems;
    _surveyItem->appendMissionItems(missionItems, this);
    const auto imageCount = std::count_if(missionItems.cbegin(), missionItems.cend(), [](const MissionItem* item) {
        return item->command() == MAV_CMD_IMAGE_START_CAPTURE;
    });
    QCOMPARE(_surveyItem->cameraShots(), static_cast<int>(imageCount));
    QCOMPARE(_surveyItem->cameraShots(), expectedShots);
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

QPolygonF SurveyComplexItemTest::_concaveTestPolygon()
{
    // 100m square with a 20m wide, 60m deep notch cut into the north edge
    return {
        {0.0, 0.0},   {100.0, 0.0}, {100.0, 100.0}, {60.0, 100.0},
        {60.0, 40.0}, {40.0, 40.0}, {40.0, 100.0},  {0.0, 100.0},
    };
}

void SurveyComplexItemTest::_setLocalSurveyPolygon(const QPolygonF& localPolygon)
{
    QList<QGeoCoordinate> coordinates;
    coordinates.reserve(localPolygon.size());
    for (const QPointF& point : localPolygon) {
        QGeoCoordinate coordinate;
        QGCGeo::convertNedToGeo(point.y(), point.x(), 0.0, TestFixtures::Coord::missionTestOrigin(), coordinate);
        coordinates.append(coordinate);
    }

    _mapPolygon->beginReset();
    _mapPolygon->clear();
    _mapPolygon->appendVertices(coordinates);
    _mapPolygon->endReset();
}

QPointF SurveyComplexItemTest::_toLocalPoint(const QGeoCoordinate& coordinate)
{
    double north = 0.0;
    double east = 0.0;
    double down = 0.0;
    QGCGeo::convertGeoToNed(coordinate, TestFixtures::Coord::missionTestOrigin(), north, east, down);
    return QPointF(east, north);
}

void SurveyComplexItemTest::_setupSplitConcaveSurvey()
{
    _setLocalSurveyPolygon(_concaveTestPolygon());
    _surveyItem->turnAroundDistance()->setRawValue(0.0);
    _surveyItem->hoverAndCapture()->setRawValue(false);
    _surveyItem->gridAngle()->setRawValue(90.0);
    _surveyItem->cameraCalc()->adjustedFootprintSide()->setRawValue(20.0);
    _surveyItem->splitConcavePolygons()->setRawValue(true);
}

void SurveyComplexItemTest::_testSplitConcavePolygon()
{
    const QPolygonF localPolygon = _concaveTestPolygon();
    _setupSplitConcaveSurvey();

    const auto allTransectMidpointsInside = [&localPolygon](const QVariantList& points) {
        if ((points.size() % 2) != 0) {
            return false;
        }

        for (qsizetype index = 0; index < points.size(); index += 2) {
            const QPointF first = _toLocalPoint(points[index].value<QGeoCoordinate>());
            const QPointF second = _toLocalPoint(points[index + 1].value<QGeoCoordinate>());
            if (!localPolygon.containsPoint((first + second) / 2.0, Qt::OddEvenFill)) {
                return false;
            }
        }
        return true;
    };

    _surveyItem->splitConcavePolygons()->setRawValue(false);
    const int joinedTransectCount = _surveyItem->_transectCount();
    QVERIFY(!allTransectMidpointsInside(_surveyItem->visualTransectPoints()));

    _surveyItem->splitConcavePolygons()->setRawValue(true);
    const int splitTransectCount = _surveyItem->_transectCount();
    QVERIFY(splitTransectCount > joinedTransectCount);

    for (int refly = 0; refly < 2; ++refly) {
        _surveyItem->refly90Degrees()->setRawValue(refly != 0);
        const int expectedTransectCount = _surveyItem->_transectCount();
        QVERIFY(expectedTransectCount >= splitTransectCount);
        for (int alternate = 0; alternate < 2; ++alternate) {
            _surveyItem->flyAlternateTransects()->setRawValue(alternate != 0);
            for (int entryLocation = 0; entryLocation < 4; ++entryLocation) {
                QCOMPARE(_surveyItem->_transectCount(), expectedTransectCount);
                QVERIFY(allTransectMidpointsInside(_surveyItem->visualTransectPoints()));
                _surveyItem->rotateEntryPoint();
            }
        }
    }

    _surveyItem->refly90Degrees()->setRawValue(false);
    _surveyItem->flyAlternateTransects()->setRawValue(false);
    ignoreLogMessage("Plan.SurveyComplexItem", QtWarningMsg, QRegularExpression("Transect spacing.*raised"));
    _surveyItem->cameraCalc()->adjustedFootprintSide()->setRawValue(0.001);
    const int cappedTransectCount = _surveyItem->_transectCount();
    QVERIFY(cappedTransectCount > 0);
    QVERIFY(cappedTransectCount <= TransectStyleComplexItem::maxTransectCount);

    // A comb with more teeth than maxTransectCount cannot be split within the limit by raising the spacing
    constexpr int combToothCount = TransectStyleComplexItem::maxTransectCount + 1;
    constexpr double combHeight = 3.0;
    constexpr double combNotchDepth = 1.0;
    const double combWidth = (2.0 * combToothCount) - 1.0;
    QPolygonF combPolygon = {{0.0, 0.0}, {combWidth, 0.0}, {combWidth, combHeight}};
    for (int toothIndex = combToothCount - 1; toothIndex >= 0; --toothIndex) {
        const double toothLeft = 2.0 * toothIndex;
        combPolygon.append(QPointF(toothLeft, combHeight));
        if (toothIndex > 0) {
            combPolygon.append(QPointF(toothLeft, combNotchDepth));
            combPolygon.append(QPointF(toothLeft - 1.0, combNotchDepth));
            combPolygon.append(QPointF(toothLeft - 1.0, combHeight));
        }
    }

    _surveyItem->cameraCalc()->adjustedFootprintSide()->setRawValue(1e9);
    ignoreLogMessage("Plan.SurveyComplexItem", QtWarningMsg,
                     QRegularExpression("Unable to limit split transect count.*joining fragments"));
    _setLocalSurveyPolygon(combPolygon);
    const int fallbackTransectCount = _surveyItem->_transectCount();
    QVERIFY(fallbackTransectCount > 0);
    QVERIFY(fallbackTransectCount <= TransectStyleComplexItem::maxTransectCount);
}

void SurveyComplexItemTest::_testSplitConcaveTurnarounds()
{
    _setupSplitConcaveSurvey();
    _surveyItem->cameraTriggerInTurnAround()->setRawValue(false);
    _planViewSettings->useConditionGate()->setRawValue(false);
    // Turnarounds longer than half the 20m notch would overshoot the next fragment if added inside a sweep
    _surveyItem->turnAroundDistance()->setRawValue(30.0);

    QList<MissionItem*> items;
    _surveyItem->appendMissionItems(items, this);

    QList<QPointF> flightPath;
    int triggerStartCount = 0;
    int triggerStopCount = 0;
    for (const MissionItem* item : items) {
        if (item->command() == MAV_CMD_NAV_WAYPOINT) {
            flightPath.append(_toLocalPoint(QGeoCoordinate(item->param5(), item->param6())));
        } else if (item->command() == MAV_CMD_DO_SET_CAM_TRIGG_DIST) {
            if (item->param1() > 0) {
                triggerStartCount++;
            } else {
                triggerStopCount++;
            }
        }
    }

    // Camera triggering stops over every concave gap
    const int transectCount = _surveyItem->_transectCount();
    QCOMPARE(triggerStartCount, transectCount);
    QCOMPARE(triggerStopCount, transectCount);

    // Consecutive collinear legs must continue in the same direction. A reversal means a turnaround was
    // added inside a sweep and overshot the next fragment.
    int collinearLegCount = 0;
    for (qsizetype index = 2; index < flightPath.size(); ++index) {
        const QPointF firstLeg = flightPath[index - 1] - flightPath[index - 2];
        const QPointF secondLeg = flightPath[index] - flightPath[index - 1];
        const double firstLength = std::hypot(firstLeg.x(), firstLeg.y());
        const double secondLength = std::hypot(secondLeg.x(), secondLeg.y());
        if ((firstLength < 0.01) || (secondLength < 0.01)) {
            continue;
        }

        const double crossProduct = (firstLeg.x() * secondLeg.y()) - (firstLeg.y() * secondLeg.x());
        if (std::abs(crossProduct) <= (1e-3 * firstLength * secondLength)) {
            collinearLegCount++;
            const double dotProduct = (firstLeg.x() * secondLeg.x()) + (firstLeg.y() * secondLeg.y());
            QVERIFY2(dotProduct > 0.0,
                     qPrintable(QStringLiteral("Flight path reverses at waypoint %1").arg(index - 1)));
        }
    }
    QVERIFY(collinearLegCount > 0);
}

void SurveyComplexItemTest::_testSplitConcaveVisualIndices()
{
    _setupSplitConcaveSurvey();
    _surveyItem->turnAroundDistance()->setRawValue(30.0);

    // Split fragments have no inner turnarounds, so transects no longer occupy a fixed number of visual points
    const QVariantList points = _surveyItem->visualTransectPoints();
    const QList<int> entryIndices = _surveyItem->visualTransectEntryIndices();
    const QList<int> exitIndices = _surveyItem->visualTransectExitIndices();
    QCOMPARE(entryIndices.size(), _surveyItem->_transectCount());
    QCOMPARE(exitIndices.size(), entryIndices.size());
    QVERIFY(points.size() != (entryIndices.size() * 4));
    QCOMPARE(entryIndices.first(), 1);
    QCOMPARE(exitIndices.last(), points.size() - 2);
    for (qsizetype index = 0; index < entryIndices.size(); ++index) {
        QVERIFY(entryIndices[index] < exitIndices[index]);
        if (index > 0) {
            QVERIFY(exitIndices[index - 1] < entryIndices[index]);
        }
    }

    // Items loaded from a plan file keep their saved visuals but have no transect structure until rebuilt
    QJsonArray items;
    _surveyItem->save(items);
    QString errorString;
    SurveyComplexItem* loadedItem =
        new SurveyComplexItem(planController(), false /* flyView */, QString() /* kmlFile */);
    QVERIFY(loadedItem->load(items[0].toObject(), 10, errorString));
    QCOMPARE(loadedItem->visualTransectPoints().size(), points.size());
    QVERIFY(loadedItem->visualTransectEntryIndices().isEmpty());
    loadedItem->deleteLater();
}

void SurveyComplexItemTest::_testSplitConcaveCameraShots()
{
    _setupSplitConcaveSurvey();
    _surveyItem->cameraTriggerInTurnAround()->setRawValue(false);

    const int shotsWithoutTurnarounds = _surveyItem->cameraShots();
    QVERIFY(shotsWithoutTurnarounds > 0);

    _surveyItem->turnAroundDistance()->setRawValue(30.0);
    QCOMPARE(_surveyItem->cameraShots(), shotsWithoutTurnarounds);
}

void SurveyComplexItemTest::_testSplitConcaveSaveLoad()
{
    _setupSplitConcaveSurvey();
    const int splitTransectCount = _surveyItem->_transectCount();

    QJsonArray items;
    _surveyItem->save(items);

    QString errorString;
    SurveyComplexItem* loadedItem =
        new SurveyComplexItem(planController(), false /* flyView */, QString() /* kmlFile */);
    QVERIFY(loadedItem->load(items[0].toObject(), 10, errorString));
    QVERIFY2(errorString.isEmpty(), qPrintable(errorString));
    QVERIFY(loadedItem->splitConcavePolygons()->rawValue().toBool());

    // Loaded items keep their saved mission items until edited, so force a rebuild before comparing transects
    loadedItem->gridAngle()->setRawValue(0.0);
    loadedItem->gridAngle()->setRawValue(90.0);
    QCOMPARE(loadedItem->_transectCount(), splitTransectCount);
    loadedItem->deleteLater();
}

void SurveyComplexItemTest::_testSplitConcaveLegacyLoad()
{
    _setupSplitConcaveSurvey();

    QJsonArray items;
    _surveyItem->save(items);
    QJsonObject version4Object = items[0].toObject();
    version4Object[JsonParsing::jsonVersionKey] = 4;
    version4Object.remove(QStringLiteral("splitConcavePolygons"));

    // New items start from the last used setting, which must not carry over to surveys saved before splitting existed
    QString errorString;
    SurveyComplexItem* loadedItem =
        new SurveyComplexItem(planController(), false /* flyView */, QString() /* kmlFile */);
    loadedItem->splitConcavePolygons()->setRawValue(true);
    QVERIFY(loadedItem->load(version4Object, 10, errorString));
    QVERIFY2(errorString.isEmpty(), qPrintable(errorString));
    QVERIFY(!loadedItem->splitConcavePolygons()->rawValue().toBool());
    loadedItem->deleteLater();
}

UT_REGISTER_TEST(SurveyComplexItemTest, TestLabel::Unit, TestLabel::MissionManager)
