/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "SurveyComplexItemTest.h"
#include "QGCApplication.h"
#include "JsonHelper.h"

SurveyComplexItemTest::SurveyComplexItemTest(void)
{
    // We use a 100m by 100m square test polygon
    const double edgeDistance = 100;
    _polyVertices.append(QGeoCoordinate(47.633550640000003, -122.08982199));
    _polyVertices.append(_polyVertices[0].atDistanceAndAzimuth(edgeDistance, 90));
    _polyVertices.append(_polyVertices[1].atDistanceAndAzimuth(edgeDistance, 180));
    _polyVertices.append(_polyVertices[2].atDistanceAndAzimuth(edgeDistance, -90.0));
}

void SurveyComplexItemTest::init(void)
{
    UnitTest::init();

    _rgSurveySignals[surveyVisualTransectPointsChangedIndex] =    SIGNAL(visualTransectPointsChanged());
    _rgSurveySignals[surveyCameraShotsChangedIndex] =             SIGNAL(cameraShotsChanged());
    _rgSurveySignals[surveyCoveredAreaChangedIndex] =             SIGNAL(coveredAreaChanged());
    _rgSurveySignals[surveyTimeBetweenShotsChangedIndex] =        SIGNAL(timeBetweenShotsChanged());
    _rgSurveySignals[surveyRefly90DegreesChangedIndex] =          SIGNAL(refly90DegreesChanged(bool));
    _rgSurveySignals[surveyDirtyChangedIndex] =                   SIGNAL(dirtyChanged(bool));

    _planViewSettings = qgcApp()->toolbox()->settingsManager()->planViewSettings();
    _masterController = new PlanMasterController(this);
    _controllerVehicle = _masterController->controllerVehicle();
    _surveyItem = new SurveyComplexItem(_masterController, false /* flyView */, QString() /* kmlFile */, this /* parent */);
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
    // It's also important to check that things are not being over-signalled when they should not be, since that can lead
    // to incorrect ui or perf impact of uneeded signals propogating ui change.

    _multiSpy = new MultiSignalSpy();
    Q_CHECK_PTR(_multiSpy);
    QCOMPARE(_multiSpy->init(_surveyItem, _rgSurveySignals, _cSurveySignals), true);
}

void SurveyComplexItemTest::cleanup(void)
{
    delete _surveyItem;
    delete _multiSpy;
}

void SurveyComplexItemTest::_testDirty(void)
{
    QVERIFY(!_surveyItem->dirty());
    _surveyItem->setDirty(false);
    QVERIFY(!_surveyItem->dirty());
    QVERIFY(_multiSpy->checkNoSignals());

    _surveyItem->setDirty(true);
    QVERIFY(_surveyItem->dirty());
    QVERIFY(_multiSpy->checkOnlySignalByMask(surveyDirtyChangedMask));
    QVERIFY(_multiSpy->pullBoolFromSignalIndex(surveyDirtyChangedIndex));
    _multiSpy->clearAllSignals();

    _surveyItem->setDirty(false);
    QVERIFY(!_surveyItem->dirty());
    QVERIFY(_multiSpy->checkOnlySignalByMask(surveyDirtyChangedMask));
    _multiSpy->clearAllSignals();

    // These facts should set dirty when changed
    QList<Fact*> rgFacts;
    rgFacts << _surveyItem->gridAngle() << _surveyItem->flyAlternateTransects();
    for(Fact* fact: rgFacts) {
        qDebug() << fact->name();
        QVERIFY(!_surveyItem->dirty());
        if (fact->typeIsBool()) {
            fact->setRawValue(!fact->rawValue().toBool());
        } else {
            fact->setRawValue(fact->rawValue().toDouble() + 1);
        }
        QVERIFY(_multiSpy->checkSignalByMask(surveyDirtyChangedMask));
        QVERIFY(_multiSpy->pullBoolFromSignalIndex(surveyDirtyChangedIndex));
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

void SurveyComplexItemTest::_setPolygon(void)
{
    _mapPolygon->appendVertices(_polyVertices);
}

void SurveyComplexItemTest::_testGridAngle(void)
{
    _setPolygon();

    for (double gridAngle=-360.0; gridAngle<=360.0; gridAngle++) {
        _surveyItem->gridAngle()->setRawValue(gridAngle);

        QVariantList gridPoints = _surveyItem->visualTransectPoints();
        QGeoCoordinate firstTransectEntry = gridPoints[0].value<QGeoCoordinate>();
        QGeoCoordinate firstTransectExit = gridPoints[1].value<QGeoCoordinate>();
        double azimuth = firstTransectEntry.azimuthTo(firstTransectExit);
        //qDebug() << gridAngle << azimuth << _clampGridAngle180(gridAngle) << _clampGridAngle180(azimuth);
        QCOMPARE((int)_clampGridAngle180(gridAngle), (int)_clampGridAngle180(azimuth));
    }
}

void SurveyComplexItemTest::_testEntryLocation(void)
{
    _setPolygon();

    for (double gridAngle=-360.0; gridAngle<=360.0; gridAngle++) {
        _surveyItem->gridAngle()->setRawValue(gridAngle);

        // Validate that each entry location is unique
        QList<QGeoCoordinate> rgSeenEntryCoords;
        for (int rotateCount=0; rotateCount<3; rotateCount++) {
            _surveyItem->rotateEntryPoint();
            QVERIFY(!rgSeenEntryCoords.contains(_surveyItem->coordinate()));
            rgSeenEntryCoords << _surveyItem->coordinate();
        }

        _surveyItem->rotateEntryPoint();    // Rotate back for first entry point
        rgSeenEntryCoords.clear();
    }
}


void SurveyComplexItemTest::_testItemCount(void)
{
    QList<MissionItem*> items;

    _setPolygon();

    _surveyItem->hoverAndCapture()->setRawValue(false);
    _surveyItem->cameraTriggerInTurnAround()->setRawValue(false);
    _surveyItem->refly90Degrees()->setRawValue(false);
    _surveyItem->appendMissionItems(items, this);
    QCOMPARE(items.count() - 1, _surveyItem->lastSequenceNumber());
    items.clear();

    _surveyItem->hoverAndCapture()->setRawValue(false);
    _surveyItem->cameraTriggerInTurnAround()->setRawValue(true);
    _surveyItem->refly90Degrees()->setRawValue(false);
    _surveyItem->appendMissionItems(items, this);
    QCOMPARE(items.count() - 1, _surveyItem->lastSequenceNumber());
    items.clear();

    _surveyItem->hoverAndCapture()->setRawValue(true);
    _surveyItem->cameraTriggerInTurnAround()->setRawValue(false);
    _surveyItem->refly90Degrees()->setRawValue(false);
    _surveyItem->appendMissionItems(items, this);
    QCOMPARE(items.count() - 1, _surveyItem->lastSequenceNumber());
    items.clear();

    _surveyItem->hoverAndCapture()->setRawValue(true);
    _surveyItem->cameraTriggerInTurnAround()->setRawValue(false);
    _surveyItem->refly90Degrees()->setRawValue(true);
    _surveyItem->appendMissionItems(items, this);
    QCOMPARE(items.count() - 1, _surveyItem->lastSequenceNumber());
    items.clear();
}

QList<MAV_CMD> SurveyComplexItemTest::_createExpectedCommands(bool hasTurnaround, bool useConditionGate)
{
    static const QList<MAV_CMD> singleFullTransect = {
        MAV_CMD_NAV_WAYPOINT,           // Turnaround
        MAV_CMD_CONDITION_GATE,         // Survey area entry edge
        MAV_CMD_DO_SET_CAM_TRIGG_DIST,
        MAV_CMD_CONDITION_GATE,         // Survey area exit edge
        MAV_CMD_DO_SET_CAM_TRIGG_DIST,
        MAV_CMD_NAV_WAYPOINT,
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
    
    for (int i=0; i<_expectedTransectCount; i++) {
        expectedCommands.append(singleTransect);
    }

    return expectedCommands;
}

void SurveyComplexItemTest::_testItemGenerationWorker(bool imagesInTurnaround, bool hasTurnaround, bool useConditionGate, const QList<MAV_CMD>& expectedCommands)
{
    qDebug() << QStringLiteral("_testItemGenerationWorker imagesInTuraround:%1 turnaround:%2 gate:%3").arg(imagesInTurnaround).arg(hasTurnaround).arg(useConditionGate);

    _surveyItem->turnAroundDistance()->setRawValue(hasTurnaround ? 50 : 0);
    _surveyItem->cameraTriggerInTurnAround()->setRawValue(imagesInTurnaround);
    _planViewSettings->useConditionGate()->setRawValue(useConditionGate);

    QList<MissionItem*> items;
    _surveyItem->appendMissionItems(items, this);
#if 0
    // Handy for debugging failures
    for (const MissionItem* item : items) {
        qDebug() << "Cmd" << item->command();
    }
#endif
    QCOMPARE(items.count(), expectedCommands.count());
    for (int i=0; i<expectedCommands.count(); i++) {
        int actualCommand = items[i]->command();
        int expectedCommand = expectedCommands[i];
#if 0
        // Handy for debugging failures
        qDebug() << "Index" << i;
#endif
        QCOMPARE(actualCommand, expectedCommand);
    }
}

void SurveyComplexItemTest::_testItemGeneration(void)
{
    // Test all the combinations of: cameraTriggerInTurnAround: false, hasTurnAround: *, useConditionGate: *

    typedef struct {
        bool        hasTurnaround;
        bool        useConditionGate;
    } TestCase_t;

    static const TestCase_t rgTestCases[] = {
        { false,    false },
        { false,    true },
        { true,     false },
        { true,     true },
    };

    for (const TestCase_t& testCase : rgTestCases) {
        _testItemGenerationWorker(false /* imagesInTurnaround */, testCase.hasTurnaround, testCase.useConditionGate, _createExpectedCommands(testCase.hasTurnaround, testCase.useConditionGate));
    }

    // Test cameraTriggerInTurnAround = true cases

    QList<MAV_CMD> imagesInTurnaroundCommands = {
        // Transect 1
        MAV_CMD_CONDITION_GATE,         // First turaround
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

    _testItemGenerationWorker(true /* imagesInTurnaround */, true /* hasTurnaround */, true /* useConditionGate */, imagesInTurnaroundCommands);

    // Switch to non CONDITION_GATE usage
    for (MAV_CMD& cmd : imagesInTurnaroundCommands) {
        cmd = cmd == MAV_CMD_CONDITION_GATE ? MAV_CMD_NAV_WAYPOINT : cmd;
    }
    _testItemGenerationWorker(true /* imagesInTurnaround */, true /* hasTurnaround */, false /* useConditionGate */, imagesInTurnaroundCommands);
}

void SurveyComplexItemTest::_testHoverCaptureItemGeneration(void)
{
    static const QList<MAV_CMD> singleFullTransect = {
        MAV_CMD_NAV_WAYPOINT,           // Turnaround
        MAV_CMD_NAV_WAYPOINT,           // Survey area entry edge
        MAV_CMD_IMAGE_START_CAPTURE,
        MAV_CMD_NAV_WAYPOINT,           // Interior trigger
        MAV_CMD_IMAGE_START_CAPTURE,
        MAV_CMD_NAV_WAYPOINT,           // Interior trigger
        MAV_CMD_IMAGE_START_CAPTURE,
        MAV_CMD_NAV_WAYPOINT,           // Survey area exit edge
        MAV_CMD_IMAGE_START_CAPTURE,
        MAV_CMD_NAV_WAYPOINT,           // Turnaround
    };

    QList<MAV_CMD> expectedCommands;
    for (int i=0; i<_expectedTransectCount; i++) {
        expectedCommands.append(singleFullTransect);
    }

    // Set trigger distance to generates two interior capture points
    double polyHeightDistance = _polyVertices[0].distanceTo(_polyVertices[3]);
    double triggerDistance = (polyHeightDistance / 3.0) + 1.0;
    _surveyItem->cameraCalc()->adjustedFootprintFrontal()->setRawValue(triggerDistance);

    qDebug() << "_testHoverCaptureItemGeneration";
    _surveyItem->hoverAndCapture()->setRawValue(true);
    _testItemGenerationWorker(false /* imagesInTurnaround */, true /* hasTurnaround */, true /* useConditionGate */, expectedCommands);
    _testItemGenerationWorker(false /* imagesInTurnaround */, true /* hasTurnaround */, false /* useConditionGate */, expectedCommands);
}
