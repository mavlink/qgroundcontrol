/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "TransectStyleComplexItemTest.h"
#include "QGCApplication.h"

TransectStyleComplexItemTest::TransectStyleComplexItemTest(void)
    : _offlineVehicle(nullptr)
{
    _polygonVertices << QGeoCoordinate(47.633550640000003, -122.08982199)
                     << QGeoCoordinate(47.634129020000003, -122.08887249)
                     << QGeoCoordinate(47.633619320000001, -122.08811074)
                     << QGeoCoordinate(47.633189139999999, -122.08900124);
}

void TransectStyleComplexItemTest::init(void)
{
    UnitTest::init();

    _offlineVehicle = new Vehicle(MAV_AUTOPILOT_PX4, MAV_TYPE_QUADROTOR, qgcApp()->toolbox()->firmwarePluginManager(), this);
    _transectStyleItem = new TransectStyleItem(_offlineVehicle, this);
    _transectStyleItem->cameraTriggerInTurnAround()->setRawValue(false);
    _transectStyleItem->cameraCalc()->cameraName()->setRawValue(_transectStyleItem->cameraCalc()->customCameraName());
    _transectStyleItem->cameraCalc()->valueSetIsDistance()->setRawValue(true);
    _transectStyleItem->cameraCalc()->distanceToSurface()->setRawValue(100);
    _setSurveyAreaPolygon();
    _transectStyleItem->setDirty(false);

    _rgSignals[cameraShotsChangedIndex] =               SIGNAL(cameraShotsChanged());
    _rgSignals[timeBetweenShotsChangedIndex] =          SIGNAL(timeBetweenShotsChanged());
    _rgSignals[visualTransectPointsChangedIndex] =      SIGNAL(visualTransectPointsChanged());
    _rgSignals[coveredAreaChangedIndex] =               SIGNAL(coveredAreaChanged());
    _rgSignals[dirtyChangedIndex] =                     SIGNAL(dirtyChanged(bool));
    _rgSignals[complexDistanceChangedIndex] =           SIGNAL(complexDistanceChanged());
    _rgSignals[greatestDistanceToChangedIndex] =        SIGNAL(greatestDistanceToChanged());
    _rgSignals[additionalTimeDelayChangedIndex] =       SIGNAL(additionalTimeDelayChanged());
    _rgSignals[lastSequenceNumberChangedIndex] =        SIGNAL(lastSequenceNumberChanged(int));

    _multiSpy = new MultiSignalSpy();
    QCOMPARE(_multiSpy->init(_transectStyleItem, _rgSignals, _cSignals), true);
}

void TransectStyleComplexItemTest::cleanup(void)
{
    delete _transectStyleItem;
    delete _offlineVehicle;
    delete _multiSpy;
}

void TransectStyleComplexItemTest::_testDirty(void)
{
    QVERIFY(!_transectStyleItem->dirty());
    _transectStyleItem->setDirty(false);
    QVERIFY(!_transectStyleItem->dirty());
    QVERIFY(_multiSpy->checkNoSignals());

    _transectStyleItem->setDirty(true);
    QVERIFY(_transectStyleItem->dirty());
    QVERIFY(_multiSpy->checkOnlySignalByMask(dirtyChangedMask));
    QVERIFY(_multiSpy->pullBoolFromSignalIndex(dirtyChangedIndex));
    _multiSpy->clearAllSignals();

    _transectStyleItem->setDirty(false);
    QVERIFY(!_transectStyleItem->dirty());
    QVERIFY(_multiSpy->checkOnlySignalByMask(dirtyChangedMask));
    _multiSpy->clearAllSignals();

    // These facts should set dirty when changed
    QList<Fact*> rgFacts;
    rgFacts << _transectStyleItem->turnAroundDistance()
            << _transectStyleItem->cameraTriggerInTurnAround()
            << _transectStyleItem->hoverAndCapture()
            << _transectStyleItem->refly90Degrees();
    for(Fact* fact: rgFacts) {
        qDebug() << fact->name();
        QVERIFY(!_transectStyleItem->dirty());
        changeFactValue(fact);
        QVERIFY(_multiSpy->checkSignalByMask(dirtyChangedMask));
        _transectStyleItem->setDirty(false);
        _multiSpy->clearAllSignals();
    }
    rgFacts.clear();

    _adjustSurveAreaPolygon();
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

void TransectStyleComplexItemTest::_setSurveyAreaPolygon(void)
{
    for (const QGeoCoordinate vertex: _polygonVertices) {
        _transectStyleItem->surveyAreaPolygon()->appendVertex(vertex);
    }
}

void TransectStyleComplexItemTest::_testRebuildTransects(void)
{
    // Changing the survey polygon should trigger:
    //  _rebuildTransects calls
    //  coveredAreaChanged signal
    //  lastSequenceNumberChanged signal
    _adjustSurveAreaPolygon();
    QVERIFY(_transectStyleItem->rebuildTransectsPhase1Called);
    QVERIFY(_transectStyleItem->recalcCameraShotsCalled);
    QVERIFY(_transectStyleItem->recalcComplexDistanceCalled);
    QVERIFY(_multiSpy->checkSignalsByMask(coveredAreaChangedMask | lastSequenceNumberChangedMask));
    _transectStyleItem->rebuildTransectsPhase1Called = false;
    _transectStyleItem->recalcCameraShotsCalled = false;
    _transectStyleItem->recalcComplexDistanceCalled = false;
    _transectStyleItem->setDirty(false);
    _multiSpy->clearAllSignals();

    // Changes to these facts should trigger:
    //  _rebuildTransects calls
    //  lastSequenceNumberChanged signal
    QList<Fact*> rgFacts;
    rgFacts << _transectStyleItem->turnAroundDistance()
            << _transectStyleItem->cameraTriggerInTurnAround()
            << _transectStyleItem->hoverAndCapture()
            << _transectStyleItem->refly90Degrees()
            << _transectStyleItem->cameraCalc()->frontalOverlap()
            << _transectStyleItem->cameraCalc()->sideOverlap();
    for(Fact* fact: rgFacts) {
        qDebug() << fact->name();
        changeFactValue(fact);
        QVERIFY(_transectStyleItem->rebuildTransectsPhase1Called);
        QVERIFY(_transectStyleItem->recalcCameraShotsCalled);
        QVERIFY(_transectStyleItem->recalcComplexDistanceCalled);
        QVERIFY(_multiSpy->checkSignalsByMask(lastSequenceNumberChangedMask));
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
    QVERIFY(_transectStyleItem->recalcComplexDistanceCalled);
    QVERIFY(_multiSpy->checkSignalsByMask(lastSequenceNumberChangedMask));
    _multiSpy->clearAllSignals();

    _transectStyleItem->cameraCalc()->valueSetIsDistance()->setRawValue(true);
    _transectStyleItem->rebuildTransectsPhase1Called = false;
    _transectStyleItem->recalcCameraShotsCalled = false;
    _transectStyleItem->recalcComplexDistanceCalled = false;
    changeFactValue(_transectStyleItem->cameraCalc()->distanceToSurface());
    QVERIFY(_transectStyleItem->rebuildTransectsPhase1Called);
    QVERIFY(_transectStyleItem->recalcCameraShotsCalled);
    QVERIFY(_transectStyleItem->recalcComplexDistanceCalled);
    QVERIFY(_multiSpy->checkSignalsByMask(lastSequenceNumberChangedMask));
    _multiSpy->clearAllSignals();
}

void TransectStyleComplexItemTest::_testDistanceSignalling(void)
{
    _adjustSurveAreaPolygon();
    QVERIFY(_multiSpy->checkSignalsByMask(complexDistanceChangedMask | greatestDistanceToChangedMask));
    _transectStyleItem->setDirty(false);
    _multiSpy->clearAllSignals();

    QList<Fact*> rgFacts;
    rgFacts << _transectStyleItem->turnAroundDistance()
            << _transectStyleItem->hoverAndCapture()
            << _transectStyleItem->refly90Degrees();
    for(Fact* fact: rgFacts) {
        qDebug() << fact->name();
        changeFactValue(fact);
        QVERIFY(_multiSpy->checkSignalsByMask(complexDistanceChangedMask | greatestDistanceToChangedMask));
        _transectStyleItem->setDirty(false);
        _multiSpy->clearAllSignals();
    }
    rgFacts.clear();
}

void TransectStyleComplexItemTest::_adjustSurveAreaPolygon(void)
{
    QGeoCoordinate vertex = _transectStyleItem->surveyAreaPolygon()->vertexCoordinate(0);
    vertex.setLatitude(vertex.latitude() + 1);
    _transectStyleItem->surveyAreaPolygon()->adjustVertex(0, vertex);
}

void TransectStyleComplexItemTest::_testAltMode(void)
{
    // Default should be relative
    QVERIFY(_transectStyleItem->cameraCalc()->distanceToSurfaceRelative());

    // Manual camera allows non-relative altitudes, validate that changing back to known
    // camera switches back to relative
    _transectStyleItem->cameraCalc()->cameraName()->setRawValue(_transectStyleItem->cameraCalc()->manualCameraName());
    _transectStyleItem->cameraCalc()->setDistanceToSurfaceRelative(false);
    _transectStyleItem->cameraCalc()->cameraName()->setRawValue(_transectStyleItem->cameraCalc()->customCameraName());
    QVERIFY(_transectStyleItem->cameraCalc()->distanceToSurfaceRelative());

    // When you turn off terrain following mode make sure that the altitude mode changed back to relative altitudes
    _transectStyleItem->cameraCalc()->setDistanceToSurfaceRelative(false);
    _transectStyleItem->setFollowTerrain(true);

    QVERIFY(!_transectStyleItem->cameraCalc()->distanceToSurfaceRelative());
    QVERIFY(_transectStyleItem->followTerrain());

    _transectStyleItem->setFollowTerrain(false);
    QVERIFY(_transectStyleItem->cameraCalc()->distanceToSurfaceRelative());
    QVERIFY(!_transectStyleItem->followTerrain());
}

TransectStyleItem::TransectStyleItem(Vehicle* vehicle, QObject* parent)
    : TransectStyleComplexItem      (vehicle, false /* flyView */, QStringLiteral("UnitTestTransect"), parent)
    , rebuildTransectsPhase1Called  (false)
    , recalcComplexDistanceCalled   (false)
    , recalcCameraShotsCalled       (false)
{

}

void TransectStyleItem::_rebuildTransectsPhase1(void)
{
    rebuildTransectsPhase1Called = true;
}

void TransectStyleItem::_recalcComplexDistance(void)
{
    recalcComplexDistanceCalled = true;
}

void TransectStyleItem::_recalcCameraShots(void)
{
    recalcCameraShotsCalled = true;
}
