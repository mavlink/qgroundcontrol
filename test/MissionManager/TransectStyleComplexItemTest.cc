#include "TransectStyleComplexItemTest.h"

#include "MultiSignalSpy.h"
#include "PlanMasterController.h"
#include "QGroundControlQmlGlobal.h"
#include "TerrainQueryTest.h"


void TransectStyleComplexItemTest::init()
{
    TransectStyleComplexItemTestBase::init();
    _transectStyleItem = new TestTransectStyleItem(planController());
    _transectStyleItem->cameraTriggerInTurnAround()->setRawValue(false);
    _transectStyleItem->cameraCalc()->setCameraBrand(CameraCalc::canonicalManualCameraName());
    _transectStyleItem->cameraCalc()->valueSetIsDistance()->setRawValue(true);
    _transectStyleItem->cameraCalc()->distanceToSurface()->setRawValue(100);
    _transectStyleItem->setDirty(false);
    _multiSpy = new MultiSignalSpy;
    QVERIFY(_multiSpy->init(_transectStyleItem));
}

void TransectStyleComplexItemTest::cleanup()
{
    delete _multiSpy;
    _multiSpy = nullptr;
    TransectStyleComplexItemTestBase::cleanup();
    // These items are deleted when planController() is deleted
    _transectStyleItem = nullptr;
}

void TransectStyleComplexItemTest::_testDirty()
{
    QVERIFY(!_transectStyleItem->dirty());
    _transectStyleItem->setDirty(false);
    QVERIFY(!_transectStyleItem->dirty());
    QVERIFY(_multiSpy->noneEmitted());
    _transectStyleItem->setDirty(true);
    QVERIFY(_transectStyleItem->dirty());
    QVERIFY(_multiSpy->onlyEmittedOnce("dirtyChanged"));
    QVERIFY(_multiSpy->argument<bool>("dirtyChanged"));
    _multiSpy->clearAllSignals();
    _transectStyleItem->setDirty(false);
    QVERIFY(!_transectStyleItem->dirty());
    QVERIFY(_multiSpy->onlyEmittedOnce("dirtyChanged"));
    _multiSpy->clearAllSignals();
    // These facts should set dirty when changed
    QList<Fact*> rgFacts;
    rgFacts << _transectStyleItem->turnAroundDistance() << _transectStyleItem->cameraTriggerInTurnAround()
            << _transectStyleItem->hoverAndCapture() << _transectStyleItem->refly90Degrees();
    for (Fact* fact : rgFacts) {
        qDebug() << fact->name();
        QVERIFY(!_transectStyleItem->dirty());
        changeFactValue(fact);
        QVERIFY(_multiSpy->emittedOnce("dirtyChanged"));
        _transectStyleItem->setDirty(false);
        _multiSpy->clearAllSignals();
    }
    rgFacts.clear();
    _transectStyleItem->adjustSurveAreaPolygon();
    QTest::qWait(100);  // Let event loop process deferred pathChanged signal
    QVERIFY(_transectStyleItem->dirty());
    _transectStyleItem->setDirty(false);
    QVERIFY(!_transectStyleItem->surveyAreaPolygon()->dirty());
    _multiSpy->clearAllSignals();
    changeFactValue(_transectStyleItem->cameraCalc()->distanceToSurface());
    QVERIFY(_transectStyleItem->dirty());
    _transectStyleItem->setDirty(false);
    QVERIFY(!_transectStyleItem->cameraCalc()->dirty());
    _multiSpy->clearAllSignals();
}

void TransectStyleComplexItemTest::_testRebuildTransects()
{
    _transectStyleItem->cameraCalc()->setCameraBrand(CameraCalc::xlatCustomCameraName());
    // Changing the survey polygon should trigger:
    //  _rebuildTransects calls
    //  coveredAreaChanged signal
    //  lastSequenceNumberChanged signal
    _transectStyleItem->adjustSurveAreaPolygon();
    QTest::qWait(100);  // Let event loop process deferred pathChanged signal
    QVERIFY(_transectStyleItem->rebuildTransectsPhase1Called);
    QVERIFY(_transectStyleItem->recalcCameraShotsCalled);
    QVERIFY(_multiSpy->emittedByMask(_multiSpy->mask("coveredAreaChanged", "lastSequenceNumberChanged")));
    _transectStyleItem->rebuildTransectsPhase1Called = false;
    _transectStyleItem->recalcCameraShotsCalled = false;
    _transectStyleItem->recalcComplexDistanceCalled = false;
    _transectStyleItem->setDirty(false);
    _multiSpy->clearAllSignals();
    // Changes to these facts should trigger:
    //  _rebuildTransects calls
    //  lastSequenceNumberChanged signal
    QList<Fact*> rgFacts;
    rgFacts << _transectStyleItem->turnAroundDistance() << _transectStyleItem->cameraTriggerInTurnAround()
            << _transectStyleItem->hoverAndCapture() << _transectStyleItem->refly90Degrees()
            << _transectStyleItem->cameraCalc()->frontalOverlap() << _transectStyleItem->cameraCalc()->sideOverlap();
    for (Fact* fact : rgFacts) {
        qDebug() << fact->name();
        changeFactValue(fact);
        QVERIFY(_transectStyleItem->rebuildTransectsPhase1Called);
        QVERIFY(_transectStyleItem->recalcCameraShotsCalled);
        QVERIFY(_multiSpy->emitted("lastSequenceNumberChanged"));
        _transectStyleItem->setDirty(false);
        _multiSpy->clearAllSignals();
        _transectStyleItem->rebuildTransectsPhase1Called = false;
        _transectStyleItem->recalcCameraShotsCalled = false;
        _transectStyleItem->recalcComplexDistanceCalled = false;
    }
    rgFacts.clear();
    _transectStyleItem->cameraCalc()->valueSetIsDistance()->setRawValue(false);
    _transectStyleItem->rebuildTransectsPhase1Called = false;
    _transectStyleItem->recalcCameraShotsCalled = false;
    _transectStyleItem->recalcComplexDistanceCalled = false;
    changeFactValue(_transectStyleItem->cameraCalc()->imageDensity());
    QVERIFY(_transectStyleItem->rebuildTransectsPhase1Called);
    QVERIFY(_transectStyleItem->recalcCameraShotsCalled);
    QVERIFY(_multiSpy->emitted("lastSequenceNumberChanged"));
    _multiSpy->clearAllSignals();
    _transectStyleItem->cameraCalc()->valueSetIsDistance()->setRawValue(true);
    _transectStyleItem->rebuildTransectsPhase1Called = false;
    _transectStyleItem->recalcCameraShotsCalled = false;
    _transectStyleItem->recalcComplexDistanceCalled = false;
    changeFactValue(_transectStyleItem->cameraCalc()->distanceToSurface());
    QVERIFY(_transectStyleItem->rebuildTransectsPhase1Called);
    QVERIFY(_transectStyleItem->recalcCameraShotsCalled);
    QVERIFY(_multiSpy->emitted("lastSequenceNumberChanged"));
    _multiSpy->clearAllSignals();
}

void TransectStyleComplexItemTest::_testDistanceSignalling()
{
    _transectStyleItem->adjustSurveAreaPolygon();
    QTest::qWait(100);  // Let event loop process deferred pathChanged signal
    QVERIFY(_multiSpy->emittedByMask(_multiSpy->mask("complexDistanceChanged", "greatestDistanceToChanged")));
    _transectStyleItem->setDirty(false);
    _multiSpy->clearAllSignals();
    QList<Fact*> rgFacts;
    rgFacts << _transectStyleItem->turnAroundDistance() << _transectStyleItem->hoverAndCapture()
            << _transectStyleItem->refly90Degrees();
    for (Fact* fact : rgFacts) {
        qDebug() << fact->name();
        changeFactValue(fact);
        QVERIFY(_multiSpy->emittedByMask(_multiSpy->mask("complexDistanceChanged", "greatestDistanceToChanged")));
        _transectStyleItem->setDirty(false);
        _multiSpy->clearAllSignals();
    }
    rgFacts.clear();
}

void TransectStyleComplexItemTest::_testAltitudes()
{
    _transectStyleItem->cameraCalc()->distanceToSurface()->setRawValue(50);
    _transectStyleItem->cameraCalc()->adjustedFootprintFrontal()->setRawValue(10);
    _transectStyleItem->cameraCalc()->adjustedFootprintSide()->setRawValue(10);
    qDebug() << _transectStyleItem->_transectCount();
    QList<MissionItem*> rgItems;
    _transectStyleItem->appendMissionItems(rgItems, this);
    for (const MissionItem* missionItem : rgItems) {
        if (missionItem->command() == MAV_CMD_NAV_WAYPOINT) {
            qDebug() << missionItem->param7();
        }
    }
}

TestTransectStyleItem::TestTransectStyleItem(PlanMasterController* masterController)
    : TransectStyleComplexItem(masterController, false /* flyView */, QStringLiteral("UnitTestTransect")),
      rebuildTransectsPhase1Called(false),
      recalcComplexDistanceCalled(false),
      recalcCameraShotsCalled(false)
{
    // We use a 100m by 100m square test polygon
    const double edgeDistance = 100;
    surveyAreaPolygon()->appendVertex(UnitTestTerrainQuery::linearSlopeRegion.center());
    surveyAreaPolygon()->appendVertex(surveyAreaPolygon()->vertexCoordinate(0).atDistanceAndAzimuth(edgeDistance, 90));
    surveyAreaPolygon()->appendVertex(surveyAreaPolygon()->vertexCoordinate(1).atDistanceAndAzimuth(edgeDistance, 180));
    surveyAreaPolygon()->appendVertex(
        surveyAreaPolygon()->vertexCoordinate(2).atDistanceAndAzimuth(edgeDistance, -90.0));
}

void TestTransectStyleItem::_rebuildTransectsPhase1()
{
    rebuildTransectsPhase1Called = true;
    _transects.clear();
    if (_surveyAreaPolygon.count() < 3) {
        return;
    }
    _transects.append(
        QList<TransectStyleComplexItem::CoordInfo_t>{{surveyAreaPolygon()->vertexCoordinate(0), CoordTypeSurveyEntry},
                                                     {surveyAreaPolygon()->vertexCoordinate(2), CoordTypeSurveyExit}});
}

void TestTransectStyleItem::_recalcCameraShots()
{
    recalcCameraShotsCalled = true;
}

void TestTransectStyleItem::adjustSurveAreaPolygon()
{
    QGeoCoordinate vertex = surveyAreaPolygon()->vertexCoordinate(0);
    vertex.setLatitude(vertex.latitude() + 1);
    surveyAreaPolygon()->adjustVertex(0, vertex);
}

// TODO: Move To Terrain Testing
/*void TransectStyleComplexItemTest::_testFollowTerrain()
{
    _transectStyleItem->cameraCalc()->distanceToSurface()->setRawValue(50);
    _transectStyleItem->cameraCalc()->adjustedFootprintFrontal()->setRawValue(0);
    _transectStyleItem->cameraCalc()->setDistanceMode(QGroundControlQmlGlobal::AltitudeModeCalcAboveTerrain);
    QVERIFY(QTest::qWaitFor([&]() { return _transectStyleItem->readyForSaveState() ==
TransectStyleComplexItem::ReadyForSave; }, 2000)); QList<MissionItem*> rgItems;
    _transectStyleItem->appendMissionItems(rgItems, this);
    QList<double> expectedTerrainValues {497, 509, 512, 512 };
    //QCOMPARE(rgItems.count(), expectedTerrainValues.count());
    for (const MissionItem* missionItem : rgItems) {
        QCOMPARE(missionItem->command(), MAV_CMD_NAV_WAYPOINT);
        QCOMPARE(missionItem->frame(), MAV_FRAME_GLOBAL);
        QCOMPARE(missionItem->param7(), expectedTerrainValues.front());
        expectedTerrainValues.pop_front();
    }
}*/

UT_REGISTER_TEST(TransectStyleComplexItemTest, TestLabel::Unit, TestLabel::MissionManager)
