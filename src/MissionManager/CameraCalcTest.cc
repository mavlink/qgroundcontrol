/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "CameraCalcTest.h"
#include "QGCApplication.h"

CameraCalcTest::CameraCalcTest(void)
    : _offlineVehicle(nullptr)
{

}

void CameraCalcTest::init(void)
{
    UnitTest::init();

    _offlineVehicle = new Vehicle(MAV_AUTOPILOT_PX4, MAV_TYPE_QUADROTOR, qgcApp()->toolbox()->firmwarePluginManager(), this);
    _cameraCalc = new CameraCalc(_offlineVehicle, "CameraCalcUnitTest" /* settingsGroup */, this);
    _cameraCalc->cameraName()->setRawValue(_cameraCalc->customCameraName());
    _cameraCalc->setDirty(false);

    _rgSignals[dirtyChangedIndex] =                     SIGNAL(dirtyChanged(bool));
    _rgSignals[imageFootprintSideChangedIndex] =        SIGNAL(imageFootprintSideChanged(double));
    _rgSignals[imageFootprintFrontalChangedIndex] =     SIGNAL(imageFootprintFrontalChanged(double));
    _rgSignals[distanceToSurfaceRelativeChangedIndex] = SIGNAL(distanceToSurfaceRelativeChanged(bool));

    _multiSpy = new MultiSignalSpy();
    QCOMPARE(_multiSpy->init(_cameraCalc, _rgSignals, _cSignals), true);
}

void CameraCalcTest::cleanup(void)
{
    delete _cameraCalc;
    delete _offlineVehicle;
    delete _multiSpy;
}

void CameraCalcTest::_testDirty(void)
{
    QVERIFY(!_cameraCalc->dirty());
    _cameraCalc->setDirty(false);
    QVERIFY(!_cameraCalc->dirty());
    QVERIFY(_multiSpy->checkNoSignals());

    _cameraCalc->setDirty(true);
    QVERIFY(_cameraCalc->dirty());
    QVERIFY(_multiSpy->checkOnlySignalByMask(dirtyChangedMask));
    QVERIFY(_multiSpy->pullBoolFromSignalIndex(dirtyChangedIndex));
    _multiSpy->clearAllSignals();

    _cameraCalc->setDirty(false);
    QVERIFY(!_cameraCalc->dirty());
    QVERIFY(_multiSpy->checkOnlySignalByMask(dirtyChangedMask));
    _multiSpy->clearAllSignals();

    // These facts should set dirty when changed
    QList<Fact*> rgFacts;
    rgFacts << _cameraCalc->valueSetIsDistance()
            << _cameraCalc->distanceToSurface()
            << _cameraCalc->imageDensity()
            << _cameraCalc->frontalOverlap ()
            << _cameraCalc->sideOverlap ()
            << _cameraCalc->adjustedFootprintSide()
            << _cameraCalc->adjustedFootprintFrontal();
    for(Fact* fact: rgFacts) {
        qDebug() << fact->name();
        QVERIFY(!_cameraCalc->dirty());
        if (fact->typeIsBool()) {
            fact->setRawValue(!fact->rawValue().toBool());
        } else {
            fact->setRawValue(fact->rawValue().toDouble() + 1);
        }
        QVERIFY(_multiSpy->checkSignalByMask(dirtyChangedMask));
        _cameraCalc->setDirty(false);
        _multiSpy->clearAllSignals();
    }
    rgFacts.clear();


    _cameraCalc->setDistanceToSurfaceRelative(!_cameraCalc->distanceToSurfaceRelative());
    QVERIFY(_cameraCalc->dirty());
    _multiSpy->clearAllSignals();

    _cameraCalc->cameraName()->setRawValue(_cameraCalc->manualCameraName());
    QVERIFY(_cameraCalc->dirty());
    _multiSpy->clearAllSignals();
}

void CameraCalcTest::_testAdjustedFootprint(void)
{
    double adjustedFootprintFrontal = _cameraCalc->adjustedFootprintFrontal()->rawValue().toDouble();
    double adjustedFootprintSide = _cameraCalc->adjustedFootprintSide()->rawValue().toDouble();
    _cameraCalc->valueSetIsDistance()->setRawValue(true);
    changeFactValue(_cameraCalc->distanceToSurface());
    QVERIFY(adjustedFootprintFrontal != _cameraCalc->adjustedFootprintFrontal()->rawValue().toDouble());
    QVERIFY(adjustedFootprintSide != _cameraCalc->adjustedFootprintSide()->rawValue().toDouble());

    adjustedFootprintFrontal = _cameraCalc->adjustedFootprintFrontal()->rawValue().toDouble();
    adjustedFootprintSide = _cameraCalc->adjustedFootprintSide()->rawValue().toDouble();
    _cameraCalc->valueSetIsDistance()->setRawValue(false);
    changeFactValue(_cameraCalc->imageDensity());
    QVERIFY(adjustedFootprintFrontal != _cameraCalc->adjustedFootprintFrontal()->rawValue().toDouble());
    QVERIFY(adjustedFootprintSide != _cameraCalc->adjustedFootprintSide()->rawValue().toDouble());

    adjustedFootprintFrontal = _cameraCalc->adjustedFootprintFrontal()->rawValue().toDouble();
    _cameraCalc->valueSetIsDistance()->setRawValue(true);
    changeFactValue(_cameraCalc->frontalOverlap());
    QVERIFY(adjustedFootprintFrontal != _cameraCalc->adjustedFootprintFrontal()->rawValue().toDouble());

    adjustedFootprintSide = _cameraCalc->adjustedFootprintSide()->rawValue().toDouble();
    _cameraCalc->valueSetIsDistance()->setRawValue(false);
    changeFactValue(_cameraCalc->sideOverlap());
    QVERIFY(adjustedFootprintSide != _cameraCalc->adjustedFootprintSide()->rawValue().toDouble());
}

void CameraCalcTest::_testAltDensityRecalc(void)
{
    _cameraCalc->valueSetIsDistance()->setRawValue(true);
    double imageDensity = _cameraCalc->imageDensity()->rawValue().toDouble();
    _cameraCalc->distanceToSurface()->setRawValue(_cameraCalc->distanceToSurface()->rawValue().toDouble() + 1);
    QVERIFY(imageDensity != _cameraCalc->imageDensity()->rawValue().toDouble());

    _cameraCalc->valueSetIsDistance()->setRawValue(false);
    double distanceToSurface = _cameraCalc->distanceToSurface()->rawValue().toDouble();
    _cameraCalc->imageDensity()->setRawValue(_cameraCalc->imageDensity()->rawValue().toDouble() + 1);
    QVERIFY(distanceToSurface != _cameraCalc->distanceToSurface()->rawValue().toDouble());
}
