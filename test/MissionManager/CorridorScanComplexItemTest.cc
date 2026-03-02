#include "CorridorScanComplexItemTest.h"

#include "CoordFixtures.h"
#include "CorridorScanComplexItem.h"
#include "MultiSignalSpy.h"
#include "PlanViewSettings.h"

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
    _multiSpyCorridorPolygon = new MultiSignalSpy();
    QCOMPARE(_multiSpyCorridorPolygon->init(_corridorItem->surveyAreaPolygon()), true);
}

void CorridorScanComplexItemTest::cleanup()
{
    delete _multiSpyCorridorPolygon;
    _multiSpyCorridorPolygon = nullptr;

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

void CorridorScanComplexItemTest::_testCameraTrigger()
{
    QSKIP("cameraTrigger property API removed in TransectStyle refactor");
}

void CorridorScanComplexItemTest::_waitForReadyForSave()
{
    QVERIFY_TRUE_WAIT(_corridorItem->readyForSaveState() == CorridorScanComplexItem::ReadyForSave,
                      TestTimeout::mediumMs());
}

void CorridorScanComplexItemTest::_testItemCount()
{
    typedef struct
    {
        bool triggerInTurnAround;
        bool hasTurnaround;
    } TestCase_t;

    static const TestCase_t rgTestCases[] = {
        {false, false},
        {false, false},
        {false, true},
        {false, true},
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
    qDebug() << QStringLiteral("_testItemGenerationWorker imagesInTuraround:%1 turnaround:%2 gate:%3")
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
        // qDebug() << "Index" << i;
        QCOMPARE(actualCommand, expectedCommand);
    }
}

void CorridorScanComplexItemTest::_testItemGeneration()
{
    // Test all the combinations of: cameraTriggerInTurnAround: false, hasTurnAround: *, useConditionGate: *
    typedef struct
    {
        bool hasTurnaround;
        bool useConditionGate;
    } TestCase_t;

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

UT_REGISTER_TEST(CorridorScanComplexItemTest, TestLabel::Unit, TestLabel::MissionManager)
