/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "TransectStyleComplexItemTest.h"
#include "QGCApplication.h"

TransectStyleComplexItemTest::TransectStyleComplexItemTest(void)
    : _offlineVehicle(NULL)
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
    _setSurveyAreaPolygon();
    _transectStyleItem->setDirty(false);

    _rgSignals[cameraShotsChangedIndex] =               SIGNAL(cameraShotsChanged());
    _rgSignals[timeBetweenShotsChangedIndex] =          SIGNAL(timeBetweenShotsChanged());
    _rgSignals[cameraMinTriggerIntervalChangedIndex] =  SIGNAL(cameraMinTriggerIntervalChanged(double));
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
    foreach(Fact* fact, rgFacts) {
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
    foreach (const QGeoCoordinate vertex, _polygonVertices) {
        _transectStyleItem->surveyAreaPolygon()->appendVertex(vertex);
    }
}

void TransectStyleComplexItemTest::_testRebuildTransects(void)
{
    // Changing the survey polygon should trigger:
    //  _rebuildTransects call
    //  coveredAreaChanged signal
    //  lastSequenceNumberChanged signal
    _adjustSurveAreaPolygon();
    QVERIFY(_transectStyleItem->rebuildTransectsCalled);
    QVERIFY(_multiSpy->checkSignalsByMask(coveredAreaChangedMask | lastSequenceNumberChangedMask));
    _transectStyleItem->rebuildTransectsCalled = false;
    _transectStyleItem->setDirty(false);
    _multiSpy->clearAllSignals();

    // Changes to these facts should trigger:
    //  _rebuildTransects call
    //  lastSequenceNumberChanged signal
    QList<Fact*> rgFacts;
    rgFacts << _transectStyleItem->turnAroundDistance()
            << _transectStyleItem->cameraTriggerInTurnAround()
            << _transectStyleItem->hoverAndCapture()
            << _transectStyleItem->refly90Degrees()
            << _transectStyleItem->cameraCalc()->adjustedFootprintSide()
            << _transectStyleItem->cameraCalc()->adjustedFootprintFrontal();
    foreach(Fact* fact, rgFacts) {
        qDebug() << fact->name();
        changeFactValue(fact);
        QVERIFY(_transectStyleItem->rebuildTransectsCalled);
        QVERIFY(_multiSpy->checkSignalsByMask(lastSequenceNumberChangedMask));
        _transectStyleItem->setDirty(false);
        _multiSpy->clearAllSignals();
        _transectStyleItem->rebuildTransectsCalled = false;
    }
    rgFacts.clear();
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
    foreach(Fact* fact, rgFacts) {
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

TransectStyleItem::TransectStyleItem(Vehicle* vehicle, QObject* parent)
    : TransectStyleComplexItem  (vehicle, QStringLiteral("UnitTestTransect"), parent)
    , rebuildTransectsCalled    (false)
{

}

void TransectStyleItem::_rebuildTransectsPhase1(void)
{
    rebuildTransectsCalled = true;
}

void TransectStyleItem::_rebuildTransectsPhase2(void)
{

}
