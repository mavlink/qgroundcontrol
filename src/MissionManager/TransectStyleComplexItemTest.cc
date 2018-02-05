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
    _linePoints << QGeoCoordinate(47.633550640000003, -122.08982199)
                << QGeoCoordinate(47.634129020000003, -122.08887249)
                << QGeoCoordinate(47.633619320000001, -122.08811074);
}

void TransectStyleComplexItemTest::init(void)
{
    UnitTest::init();

    _offlineVehicle = new Vehicle(MAV_AUTOPILOT_PX4, MAV_TYPE_QUADROTOR, qgcApp()->toolbox()->firmwarePluginManager(), this);
    _transectStyleItem = new TransectStyleItem(_offlineVehicle, this);

    _rgSignals[cameraShotsChangedIndex] =               SIGNAL(cameraShotsChanged());
    _rgSignals[timeBetweenShotsChangedIndex] =          SIGNAL(timeBetweenShotsChanged());
    _rgSignals[cameraMinTriggerIntervalChangedIndex] =  SIGNAL(cameraMinTriggerIntervalChanged(double));
    _rgSignals[transectPointsChangedIndex] =            SIGNAL(transectPointsChanged());
    _rgSignals[coveredAreaChangedIndex] =               SIGNAL(coveredAreaChanged());
    _rgSignals[dirtyChangedIndex] =                     SIGNAL(dirtyChanged(bool));
    _rgSignals[complexDistanceChangedIndex] =           SIGNAL(complexDistanceChanged());
    _rgSignals[greatestDistanceToChangedIndex] =        SIGNAL(greatestDistanceToChanged());
    _rgSignals[additionalTimeDelayChangedIndex] =       SIGNAL(additionalTimeDelayChanged());

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
        if (fact->typeIsBool()) {
            fact->setRawValue(!fact->rawValue().toBool());
        } else {
            fact->setRawValue(fact->rawValue().toDouble() + 1);
        }
        QVERIFY(_multiSpy->checkSignalByMask(dirtyChangedMask));
        _transectStyleItem->setDirty(false);
        _multiSpy->clearAllSignals();
    }
    rgFacts.clear();

    _setPolyline();
    QVERIFY(_transectStyleItem->dirty());
    _transectStyleItem->setDirty(false);
    QVERIFY(!_transectStyleItem->surveyAreaPolygon()->dirty());
    _multiSpy->clearAllSignals();

    _transectStyleItem->cameraCalc()->distanceToSurface()->setRawValue(_transectStyleItem->cameraCalc()->distanceToSurface()->rawValue().toDouble() + 1);
    QVERIFY(_transectStyleItem->dirty());
    _transectStyleItem->setDirty(false);
    QVERIFY(!_transectStyleItem->cameraCalc()->dirty());
    _multiSpy->clearAllSignals();
}

void TransectStyleComplexItemTest::_setPolyline(void)
{
    for (int i=0; i<_linePoints.count(); i++) {
        QGeoCoordinate& vertex = _linePoints[i];
        _transectStyleItem->surveyAreaPolygon()->appendVertex(vertex);
    }
}

TransectStyleItem::TransectStyleItem(Vehicle* vehicle, QObject* parent)
    : TransectStyleComplexItem  (vehicle, QStringLiteral("UnitTestTransect"), parent)
    , _rebuildTransectsCalled   (false)
{

}

void TransectStyleItem::_rebuildTransects(void)
{
    _rebuildTransectsCalled = true;
}
