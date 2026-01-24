#include "CameraCalcTest.h"
#include "CameraCalc.h"
#include "PlanMasterController.h"
#include "MultiSignalSpy.h"

#include <QtTest/QTest>

void CameraCalcTest::init()
{
    OfflineTest::init();

    _masterController = new PlanMasterController(this);
    _controllerVehicle = _masterController->controllerVehicle();
    _cameraCalc = new CameraCalc(_masterController, "CameraCalcUnitTest" /* settingsGroup */, this);
    _cameraCalc->setCameraBrand(CameraCalc::canonicalCustomCameraName());
    _cameraCalc->setDirty(false);

    _multiSpy = new MultiSignalSpy();
    QVERIFY(_multiSpy->init(_cameraCalc));
}

void CameraCalcTest::cleanup()
{
    delete _multiSpy;
    delete _cameraCalc;
    delete _masterController;

    _multiSpy           = nullptr;
    _cameraCalc         = nullptr;
    _masterController   = nullptr;
    _controllerVehicle  = nullptr;

    OfflineTest::cleanup();
}

void CameraCalcTest::_testDirty()
{
    const char* dirtyChangedSignal  = "dirtyChanged";
    auto        dirtyChangedMask    = _multiSpy->mask(dirtyChangedSignal);

    QVERIFY(!_cameraCalc->dirty());
    _cameraCalc->setDirty(false);
    QVERIFY(!_cameraCalc->dirty());
    QVERIFY_NO_SIGNALS(*_multiSpy);

    _cameraCalc->setDirty(true);
    QVERIFY(_cameraCalc->dirty());
    QVERIFY(_multiSpy->checkOnlySignalByMask(dirtyChangedMask));
    QVERIFY(_multiSpy->pullBoolFromSignal(dirtyChangedSignal));
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


    _cameraCalc->setDistanceMode(_cameraCalc->distanceMode() == QGroundControlQmlGlobal::AltitudeModeRelative ? QGroundControlQmlGlobal::AltitudeModeAbsolute : QGroundControlQmlGlobal::AltitudeModeRelative);
    QVERIFY(_cameraCalc->dirty());
    _multiSpy->clearAllSignals();

    _cameraCalc->setCameraBrand(CameraCalc::canonicalManualCameraName());
    QVERIFY(_cameraCalc->dirty());
    _multiSpy->clearAllSignals();
}

void CameraCalcTest::_testAdjustedFootprint()
{
    double adjustedFootprintFrontal = _cameraCalc->adjustedFootprintFrontal()->rawValue().toDouble();
    double adjustedFootprintSide = _cameraCalc->adjustedFootprintSide()->rawValue().toDouble();
    _cameraCalc->valueSetIsDistance()->setRawValue(true);
    changeFactValue(_cameraCalc->distanceToSurface());
    QCOMPARE_NE(_cameraCalc->adjustedFootprintFrontal()->rawValue().toDouble(), adjustedFootprintFrontal);
    QCOMPARE_NE(_cameraCalc->adjustedFootprintSide()->rawValue().toDouble(), adjustedFootprintSide);

    adjustedFootprintFrontal = _cameraCalc->adjustedFootprintFrontal()->rawValue().toDouble();
    adjustedFootprintSide = _cameraCalc->adjustedFootprintSide()->rawValue().toDouble();
    _cameraCalc->valueSetIsDistance()->setRawValue(false);
    changeFactValue(_cameraCalc->imageDensity());
    QCOMPARE_NE(_cameraCalc->adjustedFootprintFrontal()->rawValue().toDouble(), adjustedFootprintFrontal);
    QCOMPARE_NE(_cameraCalc->adjustedFootprintSide()->rawValue().toDouble(), adjustedFootprintSide);

    adjustedFootprintFrontal = _cameraCalc->adjustedFootprintFrontal()->rawValue().toDouble();
    _cameraCalc->valueSetIsDistance()->setRawValue(true);
    changeFactValue(_cameraCalc->frontalOverlap());
    QCOMPARE_NE(_cameraCalc->adjustedFootprintFrontal()->rawValue().toDouble(), adjustedFootprintFrontal);

    adjustedFootprintSide = _cameraCalc->adjustedFootprintSide()->rawValue().toDouble();
    _cameraCalc->valueSetIsDistance()->setRawValue(false);
    changeFactValue(_cameraCalc->sideOverlap());
    QCOMPARE_NE(_cameraCalc->adjustedFootprintSide()->rawValue().toDouble(), adjustedFootprintSide);
}

void CameraCalcTest::_testAltDensityRecalc()
{
    _cameraCalc->valueSetIsDistance()->setRawValue(true);
    double imageDensity = _cameraCalc->imageDensity()->rawValue().toDouble();
    _cameraCalc->distanceToSurface()->setRawValue(_cameraCalc->distanceToSurface()->rawValue().toDouble() + 1);
    QCOMPARE_NE(_cameraCalc->imageDensity()->rawValue().toDouble(), imageDensity);

    _cameraCalc->valueSetIsDistance()->setRawValue(false);
    double distanceToSurface = _cameraCalc->distanceToSurface()->rawValue().toDouble();
    _cameraCalc->imageDensity()->setRawValue(_cameraCalc->imageDensity()->rawValue().toDouble() + 1);
    QCOMPARE_NE(_cameraCalc->distanceToSurface()->rawValue().toDouble(), distanceToSurface);
}
