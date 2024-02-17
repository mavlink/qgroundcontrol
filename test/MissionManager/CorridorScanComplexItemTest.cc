/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "CorridorScanComplexItemTest.h"
#include "QGCApplication.h"

CorridorScanComplexItemTest::CorridorScanComplexItemTest(void)
{
    _polyLineVertices.append(QGeoCoordinate(47.633550640000003, -122.08982199));
    _polyLineVertices.append(_polyLineVertices[0].atDistanceAndAzimuth(_corridorLineSegmentDistance, 0));
    _polyLineVertices.append(_polyLineVertices[1].atDistanceAndAzimuth(_corridorLineSegmentDistance, 20));
}

void CorridorScanComplexItemTest::init(void)
{
    TransectStyleComplexItemTestBase::init();

    _corridorItem = new CorridorScanComplexItem(_masterController, false /* flyView */, QString() /* kmlFile */);
    _corridorItem->corridorPolyline()->appendVertices(_polyLineVertices);

    // Setup for expected transect count
    _corridorItem->corridorWidth()->setRawValue(_corridorWidth);
    _corridorItem->cameraCalc()->adjustedFootprintSide()->setRawValue((_corridorWidth * 0.5) + 1.0);
    _corridorItem->cameraCalc()->adjustedFootprintFrontal()->setRawValue(_corridorLineSegmentDistance * 0.25);

    int expectedTransectCount = _expectedTransectCount;
    QCOMPARE(_corridorItem->_transectCount(), expectedTransectCount);

    _corridorItem->setDirty(false);

    _rgCorridorPolygonSignals[corridorPolygonPathChangedIndex] = SIGNAL(pathChanged());

    _multiSpyCorridorPolygon = new MultiSignalSpy();
    QCOMPARE(_multiSpyCorridorPolygon->init(_corridorItem->surveyAreaPolygon(), _rgCorridorPolygonSignals, _cCorridorPolygonSignals), true);
}

void CorridorScanComplexItemTest::cleanup(void)
{
    TransectStyleComplexItemTestBase::cleanup();

    // _corridorItem is deleted when _masterController goes away
    _corridorItem = nullptr;
}

void CorridorScanComplexItemTest::_testDirty(void)
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
    QVERIFY(_corridorItem->dirty());
    _corridorItem->setDirty(false);
}

void CorridorScanComplexItemTest::_testCameraTrigger(void)
{
#if 0
    QCOMPARE(_corridorItem->property("cameraTrigger").toBool(), true);

    // Set up a grid

    for (int i=0; i<3; i++) {
        _mapPolyline->appendVertex(_linePoints[i]);
    }

    _corridorItem->setDirty(false);
    _multiSpy->clearAllSignals();

    int lastSeq = _corridorItem->lastSequenceNumber();
    QVERIFY(lastSeq > 0);

    // Turning off camera triggering should remove two camera trigger mission items, this should trigger:
    //      lastSequenceNumberChanged
    //      dirtyChanged

    _corridorItem->setProperty("cameraTrigger", false);
    QVERIFY(_multiSpy->checkOnlySignalByMask(lastSequenceNumberChangedMask | dirtyChangedMask | cameraTriggerChangedMask));
    QCOMPARE(_multiSpy->pullIntFromSignalIndex(lastSequenceNumberChangedIndex), lastSeq - 2);

    _corridorItem->setDirty(false);
    _multiSpy->clearAllSignals();

    // Turn on camera triggering and make sure things go back to previous count

    _corridorItem->setProperty("cameraTrigger", true);
    QVERIFY(_multiSpy->checkOnlySignalByMask(lastSequenceNumberChangedMask | dirtyChangedMask | cameraTriggerChangedMask));
    QCOMPARE(_multiSpy->pullIntFromSignalIndex(lastSequenceNumberChangedIndex), lastSeq);
#endif
}

#if 0
void CorridorScanComplexItemTest::_testEntryLocation(void)
{
    _setPolygon();

    for (double gridAngle=-360.0; gridAngle<=360.0; gridAngle++) {
        _corridorItem->gridAngle()->setRawValue(gridAngle);

        QList<QGeoCoordinate> rgSeenEntryCoords;
        QList<int> rgEntryLocation;
        rgEntryLocation << SurveyComplexItem::EntryLocationTopLeft
                        << SurveyComplexItem::EntryLocationTopRight
                        << SurveyComplexItem::EntryLocationBottomLeft
                        << SurveyComplexItem::EntryLocationBottomRight;

        // Validate that each entry location is unique
        for (int i=0; i<rgEntryLocation.count(); i++) {
            int entryLocation = rgEntryLocation[i];

            _corridorItem->gridEntryLocation()->setRawValue(entryLocation);
            QVERIFY(!rgSeenEntryCoords.contains(_corridorItem->coordinate()));
            rgSeenEntryCoords << _corridorItem->coordinate();
        }
        rgSeenEntryCoords.clear();
    }
}
#endif

void CorridorScanComplexItemTest::_waitForReadyForSave(void)
{
    int loops = 0;
    while (loops++ < 8) {
        if (_corridorItem->readyForSaveState() == CorridorScanComplexItem::ReadyForSave) {
            return;
        }
        QTest::qWait(500);
    }
    QVERIFY(false);
}

void CorridorScanComplexItemTest::_testItemCount(void)
{
    typedef struct {
        bool triggerInTurnAround;
        bool hasTurnaround;
    } TestCase_t;

    static const TestCase_t rgTestCases[] = {
        { false,    false },
        { false,    false },
        { false,    true },
        { false,    true },
    };

    QList<MissionItem*> items;
    for (const TestCase_t& testCase : rgTestCases) {
        qDebug() << "triggerInTurnAround:hasTurnaround" << testCase.triggerInTurnAround << testCase.hasTurnaround;
        _corridorItem->cameraTriggerInTurnAround()->setRawValue(testCase.triggerInTurnAround);
        _corridorItem->turnAroundDistance()->setRawValue(testCase.hasTurnaround ? 50 : 0);
        _corridorItem->appendMissionItems(items, this);
        QCOMPARE(items.count() - 1, _corridorItem->lastSequenceNumber());
        items.clear();
    }
}

void CorridorScanComplexItemTest::_testPathChanges(void)
{
     QGeoCoordinate vertex = _corridorItem->corridorPolyline()->vertexCoordinate(1);
     vertex.setLatitude(vertex.latitude() + 0.01);
     _corridorItem->corridorPolyline()->adjustVertex(1, vertex);

     QVERIFY(_multiSpyCorridorPolygon->checkSignalsByMask(corridorPolygonPathChangedMask));
}

QList<MAV_CMD> CorridorScanComplexItemTest::_createExpectedCommands(bool hasTurnaround, bool useConditionGate)
{
    static const QList<MAV_CMD> singleFullTransect = {
        MAV_CMD_NAV_WAYPOINT,           // Turnaround
        MAV_CMD_CONDITION_GATE,         // Survey area entry edge
        MAV_CMD_DO_SET_CAM_TRIGG_DIST,
        MAV_CMD_NAV_WAYPOINT,           // Polyline turn
        MAV_CMD_CONDITION_GATE,         // Survey area exit edge
        MAV_CMD_DO_SET_CAM_TRIGG_DIST,
        MAV_CMD_NAV_WAYPOINT,           // Turnaround
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



void CorridorScanComplexItemTest::_testItemGenerationWorker(bool imagesInTurnaround, bool hasTurnaround, bool useConditionGate, const QList<MAV_CMD>& expectedCommands)
{
    qDebug() << QStringLiteral("_testItemGenerationWorker imagesInTuraround:%1 turnaround:%2 gate:%3").arg(imagesInTurnaround).arg(hasTurnaround).arg(useConditionGate);

    _corridorItem->turnAroundDistance()->setRawValue(hasTurnaround ? 50 : 0);
    _corridorItem->cameraTriggerInTurnAround()->setRawValue(imagesInTurnaround);
    _planViewSettings->useConditionGate()->setRawValue(useConditionGate);

    QList<MissionItem*> items;
    _corridorItem->appendMissionItems(items, this);
    //_printItemCommands(items);
    QCOMPARE(items.count(), expectedCommands.count());
    for (int i=0; i<expectedCommands.count(); i++) {
        int actualCommand = items[i]->command();
        int expectedCommand = expectedCommands[i];
        //qDebug() << "Index" << i;
        QCOMPARE(actualCommand, expectedCommand);
    }
}

void CorridorScanComplexItemTest::_testItemGeneration(void)
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
}

