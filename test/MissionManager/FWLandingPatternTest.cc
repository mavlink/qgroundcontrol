#include "FWLandingPatternTest.h"

#include <QtCore/QJsonArray>

#include "CameraSectionTest.h"
#include "FixedWingLandingComplexItem.h"
#include "Fixtures/TestFixtures.h"
#include "MultiSignalSpy.h"

using namespace TestFixtures;

void FWLandingPatternTest::init()
{
    VisualMissionItemTest::init();
    _fwItem = new FixedWingLandingComplexItem(planController(), false /* flyView */);
    _createSpy(_fwItem, &_viSpy);
    // Start in a clean state
    QVERIFY(!_fwItem->dirty());
    _fwItem->setLandingCoordinate(Coord::seattle());
    _fwItem->setDirty(false);
    QVERIFY(!_fwItem->dirty());
    _viSpy->clearAllSignals();
    _validStopVideoItem = CameraSectionTest::createValidStopTimeItem(planController());
    _validStopDistanceItem = CameraSectionTest::createValidStopTimeItem(planController());
    _validStopTimeItem = CameraSectionTest::createValidStopTimeItem(planController());
}

void FWLandingPatternTest::cleanup()
{
    delete _viSpy;
    _viSpy = nullptr;
    VisualMissionItemTest::cleanup();
    // These items go away when planController() goes away
    _fwItem = nullptr;
    _validStopVideoItem = nullptr;
    _validStopDistanceItem = nullptr;
    _validStopTimeItem = nullptr;
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
    QVERIFY(_viSpy->onlyEmittedOnce("dirtyChanged"));
    QVERIFY(_viSpy->argument<bool>("dirtyChanged"));
    _fwItem->setDirty(false);
    _viSpy->clearAllSignals();
    // These facts should set dirty when changed
    QList<Fact*> rgFacts;
    rgFacts << _fwItem->glideSlope() << _fwItem->valueSetIsDistance();
    for (Fact* fact : rgFacts) {
        qDebug() << fact->name();
        QVERIFY(!_fwItem->dirty());
        changeFactValue(fact);
        QVERIFY(_viSpy->emittedOnce("dirtyChanged"));
        QVERIFY(_viSpy->argument<bool>("dirtyChanged"));
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
    FixedWingLandingComplexItem* newItem = new FixedWingLandingComplexItem(planController(), false /* flyView */);
    bool success = newItem->load(items[0].toObject(), 10, errorString);
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
    QCOMPARE(newItem->glideSlope()->rawValue().toInt(), _fwItem->glideSlope()->rawValue().toInt());
    QCOMPARE(newItem->valueSetIsDistance()->rawValue().toBool(), _fwItem->valueSetIsDistance()->rawValue().toBool());
}

UT_REGISTER_TEST(FWLandingPatternTest, TestLabel::Unit, TestLabel::MissionManager)
