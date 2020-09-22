/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "FWLandingPatternTest.h"
#include "QGCApplication.h"
#include "MissionCommandTree.h"
#include "MissionCommandUIInfo.h"
#include "CameraSectionTest.h"

FWLandingPatternTest::FWLandingPatternTest(void)
{
    
}

void FWLandingPatternTest::init(void)
{
    VisualMissionItemTest::init();

    _fwItem = new FixedWingLandingComplexItem(_masterController, false /* flyView */, this);
    _createSpy(_fwItem, &_viSpy);

    // Start in a clean state
    QVERIFY(!_fwItem->dirty());
    _fwItem->setLandingCoordinate(QGeoCoordinate(47, -122));
    _fwItem->setDirty(false);
    QVERIFY(!_fwItem->dirty());
    _viSpy->clearAllSignals();

    _validStopVideoItem =       CameraSectionTest::createValidStopTimeItem(_masterController, this);
    _validStopDistanceItem =    CameraSectionTest::createValidStopTimeItem(_masterController, this);
    _validStopTimeItem =        CameraSectionTest::createValidStopTimeItem(_masterController, this);
}

void FWLandingPatternTest::cleanup(void)
{
    delete _fwItem;
    delete _viSpy;
    delete _validStopVideoItem;
    delete _validStopDistanceItem;
    delete _validStopTimeItem;
    VisualMissionItemTest::cleanup();
}


void FWLandingPatternTest::_testDefaults(void)
{
    QCOMPARE(_fwItem->stopTakingPhotos()->rawValue().toBool(), true);
    QCOMPARE(_fwItem->stopTakingVideo()->rawValue().toBool(), true);
}

void FWLandingPatternTest::_testDirty(void)
{
    _fwItem->setDirty(true);
    QVERIFY(_fwItem->dirty());
    QVERIFY(_viSpy->checkOnlySignalByMask(dirtyChangedMask));
    QVERIFY(_viSpy->pullBoolFromSignalIndex(dirtyChangedIndex));
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
        QVERIFY(_viSpy->checkSignalByMask(dirtyChangedMask));
        QVERIFY(_viSpy->pullBoolFromSignalIndex(dirtyChangedIndex));
        _fwItem->setDirty(false);
        _viSpy->clearAllSignals();
    }
    rgFacts.clear();
}

void FWLandingPatternTest::_testSaveLoad(void)
{
    QJsonArray items;

    _fwItem->save(items);

    QString errorString;
    FixedWingLandingComplexItem* newItem = new FixedWingLandingComplexItem(_masterController, false /* flyView */, this /* parent */);
    bool success =newItem->load(items[0].toObject(), 10, errorString);
    if (!success) {
        qDebug() << errorString;
    }
    QVERIFY(success);
    QVERIFY(errorString.isEmpty());
    _validateItem(newItem);
    newItem->deleteLater();
}

void FWLandingPatternTest::_validateItem(FixedWingLandingComplexItem* newItem)
{
    QVERIFY(newItem);

    QCOMPARE(newItem->glideSlope()->rawValue().toInt(),             _fwItem->glideSlope()->rawValue().toInt());
    QCOMPARE(newItem->valueSetIsDistance()->rawValue().toBool(),    _fwItem->valueSetIsDistance()->rawValue().toBool());
}
