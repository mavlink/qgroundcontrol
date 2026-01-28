#include "FWLandingPatternTest.h"
#include "CameraSectionTest.h"
#include "MultiSignalSpy.h"
#include "FixedWingLandingComplexItem.h"

#include <QtTest/QTest>
#include <QtCore/QJsonArray>

void FWLandingPatternTest::init()
{
    VisualMissionItemTest::init();

    _fwItem = new FixedWingLandingComplexItem(_masterController, false /* flyView */);
    _createSpy(_fwItem, &_viSpy);

    // Start in a clean state
    QVERIFY(!_fwItem->dirty());
    _fwItem->setLandingCoordinate(QGeoCoordinate(47, -122));
    _fwItem->setDirty(false);
    QVERIFY(!_fwItem->dirty());
    _viSpy->clearAllSignals();

    _validStopVideoItem =       CameraSectionTest::createValidStopTimeItem(_masterController);
    _validStopDistanceItem =    CameraSectionTest::createValidStopTimeItem(_masterController);
    _validStopTimeItem =        CameraSectionTest::createValidStopTimeItem(_masterController);
}

void FWLandingPatternTest::cleanup()
{
    delete _viSpy;
    _viSpy = nullptr;

    VisualMissionItemTest::cleanup();

    // These items go away when _masterController goes away
    _fwItem                 = nullptr;
    _validStopVideoItem     = nullptr;
    _validStopDistanceItem  = nullptr;
    _validStopTimeItem  = nullptr;
}


void FWLandingPatternTest::_testDefaults()
{
    QCOMPARE(_fwItem->stopTakingPhotos()->rawValue().toBool(), true);
    QCOMPARE(_fwItem->stopTakingVideo()->rawValue().toBool(), true);
}

void FWLandingPatternTest::_testDirty()
{
    _fwItem->setDirty(true);
    QVERIFY(_fwItem->dirty());
    QVERIFY(_viSpy->checkOnlySignalByMask(_viSpy->mask("dirtyChanged")));
    QVERIFY(_viSpy->pullBoolFromSignal("dirtyChanged"));
    _fwItem->setDirty(false);
    _viSpy->clearAllSignals();

    // These facts should set dirty when changed
    QList<Fact*> rgFacts;
    rgFacts << _fwItem->glideSlope()
            << _fwItem->valueSetIsDistance();
    for(Fact* fact: rgFacts) {
        qDebug() << fact->name();
        QVERIFY(!_fwItem->dirty());
        changeFactValue(fact);
        QVERIFY(_viSpy->checkSignalByMask(_viSpy->mask("dirtyChanged")));
        QVERIFY(_viSpy->pullBoolFromSignal("dirtyChanged"));
        _fwItem->setDirty(false);
        _viSpy->clearAllSignals();
    }
    rgFacts.clear();
}

void FWLandingPatternTest::_testSaveLoad()
{
    QJsonArray items;

    _fwItem->save(items);

    QString errorString;
    FixedWingLandingComplexItem* newItem = new FixedWingLandingComplexItem(_masterController, false /* flyView */);
    bool success =newItem->load(items[0].toObject(), 10, errorString);
    if (!success) {
        qDebug() << errorString;
    }
    QVERIFY(success);
    QGC_VERIFY_EMPTY(errorString);
    _validateItem(newItem);
    newItem->deleteLater();
}

void FWLandingPatternTest::_validateItem(FixedWingLandingComplexItem* newItem)
{
    VERIFY_NOT_NULL(newItem);

    QCOMPARE(newItem->glideSlope()->rawValue().toInt(),             _fwItem->glideSlope()->rawValue().toInt());
    QCOMPARE(newItem->valueSetIsDistance()->rawValue().toBool(),    _fwItem->valueSetIsDistance()->rawValue().toBool());
}
