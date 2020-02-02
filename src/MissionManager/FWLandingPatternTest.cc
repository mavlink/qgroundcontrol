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
    : _offlineVehicle           (Q_NULLPTR)
    , _fwItem                   (Q_NULLPTR)
    , _multiSpy                 (Q_NULLPTR)
    , _validStopVideoItem       (Q_NULLPTR)
    , _validStopDistanceItem    (Q_NULLPTR)
    , _validStopTimeItem        (Q_NULLPTR)
{
    
}

void FWLandingPatternTest::init(void)
{
    UnitTest::init();

    rgSignals[dirtyChangedIndex] = SIGNAL(dirtyChanged(bool));

    _offlineVehicle = new Vehicle(MAV_AUTOPILOT_PX4, MAV_TYPE_QUADROTOR, qgcApp()->toolbox()->firmwarePluginManager(), this);
    _fwItem = new FixedWingLandingComplexItem(_offlineVehicle, false /* flyView */, this);
    _multiSpy = new MultiSignalSpy();
    QCOMPARE(_multiSpy->init(_fwItem, rgSignals, cSignals), true);

    // Start in a clean state
    QVERIFY(!_fwItem->dirty());
    _fwItem->setLandingCoordinate(QGeoCoordinate(47, -122));
    _fwItem->setDirty(false);
    QVERIFY(!_fwItem->dirty());
    _multiSpy->clearAllSignals();

    _validStopVideoItem =       CameraSectionTest::createValidStopTimeItem(_offlineVehicle, this);
    _validStopDistanceItem =    CameraSectionTest::createValidStopTimeItem(_offlineVehicle, this);
    _validStopTimeItem =        CameraSectionTest::createValidStopTimeItem(_offlineVehicle, this);
}

void FWLandingPatternTest::cleanup(void)
{
    delete _fwItem;
    delete _offlineVehicle;
    delete _multiSpy;
    delete _validStopVideoItem;
    delete _validStopDistanceItem;
    delete _validStopTimeItem;
    UnitTest::cleanup();
}


void FWLandingPatternTest::_testDefaults(void)
{
    QCOMPARE(_fwItem->stopTakingPhotos()->rawValue().toBool(), true);
    QCOMPARE(_fwItem->stopTakingVideo()->rawValue().toBool(), true);
}

void FWLandingPatternTest::_testItemCount(void)
{
    QList<MissionItem*> items;

    _fwItem->stopTakingPhotos()->setRawValue(true);
    _fwItem->stopTakingVideo()->setRawValue(true);
    _fwItem->appendMissionItems(items, this);
    QCOMPARE(items.count(), 3 + CameraSection::stopTakingPhotosCommandCount() + CameraSection::stopTakingVideoCommandCount());
    QCOMPARE(items.count() - 1, _fwItem->lastSequenceNumber());
    items.clear();

    _fwItem->stopTakingPhotos()->setRawValue(false);
    _fwItem->stopTakingVideo()->setRawValue(false);
    _fwItem->appendMissionItems(items, this);
    QCOMPARE(items.count(), 3);
    QCOMPARE(items.count() - 1, _fwItem->lastSequenceNumber());
    items.clear();
}

void FWLandingPatternTest::_testAppendSectionItems(void)
{
    QList<MissionItem*> rgMissionItems;

    // Create the set of appended mission items
    _fwItem->stopTakingPhotos()->setRawValue(true);
    _fwItem->stopTakingVideo()->setRawValue(true);
    _fwItem->appendMissionItems(rgMissionItems, this);

    // Convert to visual items
    QmlObjectListModel* simpleItems = new QmlObjectListModel(this);

    for (MissionItem* item: rgMissionItems) {
        SimpleMissionItem* simpleItem = new SimpleMissionItem(_offlineVehicle, false /* flyView */, simpleItems);
        simpleItem->missionItem() = *item;
        simpleItems->append(simpleItem);
    }

    // Scan the items back in to verify the same values come back
    // Note that the compares does the best it can with doubles going to floats and back causing inaccuracies beyond a fuzzy compare.
    QVERIFY(FixedWingLandingComplexItem::scanForItem(simpleItems, false /* flyView */, _offlineVehicle));
    QCOMPARE(simpleItems->count(), 1);
    _validateItem(simpleItems->value<FixedWingLandingComplexItem*>(0));

    // Reset
    simpleItems->deleteLater();
    rgMissionItems.clear();

    // Check for no stop camera actions
    _fwItem->stopTakingPhotos()->setRawValue(false);
    _fwItem->stopTakingVideo()->setRawValue(false);
    _fwItem->appendMissionItems(rgMissionItems, this);
    simpleItems = new QmlObjectListModel(this);
    for (MissionItem* item: rgMissionItems) {
        SimpleMissionItem* simpleItem = new SimpleMissionItem(_offlineVehicle, false /* flyView */, simpleItems);
        simpleItem->missionItem() = *item;
        simpleItems->append(simpleItem);
    }
    QVERIFY(FixedWingLandingComplexItem::scanForItem(simpleItems, false /* flyView */, _offlineVehicle));
    QCOMPARE(simpleItems->count(), 1);
    _validateItem(simpleItems->value<FixedWingLandingComplexItem*>(0));

    // Reset
    simpleItems->deleteLater();
    rgMissionItems.clear();
}

void FWLandingPatternTest::_testDirty(void)
{
    _fwItem->setDirty(true);
    QVERIFY(_fwItem->dirty());
    QVERIFY(_multiSpy->checkOnlySignalByMask(dirtyChangedMask));
    QVERIFY(_multiSpy->pullBoolFromSignalIndex(dirtyChangedIndex));
    _fwItem->setDirty(false);
    _multiSpy->clearAllSignals();

    // These facts should set dirty when changed
    QList<Fact*> rgFacts;
    rgFacts << _fwItem->loiterAltitude()
            << _fwItem->landingHeading()
            << _fwItem->loiterRadius()
            << _fwItem->landingAltitude()
            << _fwItem->landingDistance()
            << _fwItem->glideSlope()
            << _fwItem->stopTakingPhotos()
            << _fwItem->stopTakingVideo()
            << _fwItem->valueSetIsDistance();
    for(Fact* fact: rgFacts) {
        qDebug() << fact->name();
        QVERIFY(!_fwItem->dirty());
        if (fact->typeIsBool()) {
            fact->setRawValue(!fact->rawValue().toBool());
        } else {
            fact->setRawValue(fact->rawValue().toDouble() + 1);
        }
        QVERIFY(_multiSpy->checkSignalByMask(dirtyChangedMask));
        QVERIFY(_multiSpy->pullBoolFromSignalIndex(dirtyChangedIndex));
        _fwItem->setDirty(false);
        _multiSpy->clearAllSignals();
    }
    rgFacts.clear();

    // These bool properties should set dirty when changed
    QList<const char*> rgBoolNames;
    rgBoolNames << "loiterClockwise"
                << "altitudesAreRelative";
    const QMetaObject* metaObject = _fwItem->metaObject();
    for(const char* boolName: rgBoolNames) {
        qDebug() << boolName;
        QVERIFY(!_fwItem->dirty());
        QMetaProperty boolProp = metaObject->property(metaObject->indexOfProperty(boolName));
        QVERIFY(boolProp.write(_fwItem, !boolProp.read(_fwItem).toBool()));
        QVERIFY(_multiSpy->checkSignalByMask(dirtyChangedMask));
        QVERIFY(_multiSpy->pullBoolFromSignalIndex(dirtyChangedIndex));
        _fwItem->setDirty(false);
        _multiSpy->clearAllSignals();
    }
    rgFacts.clear();

    // These coordinates should set dirty when changed
    QVERIFY(!_fwItem->dirty());
    _fwItem->setLoiterCoordinate(_fwItem->loiterCoordinate().atDistanceAndAzimuth(1, 0));
    QVERIFY(_multiSpy->checkSignalByMask(dirtyChangedMask));
    QVERIFY(_multiSpy->pullBoolFromSignalIndex(dirtyChangedIndex));
    _fwItem->setDirty(false);
    _multiSpy->clearAllSignals();
    QVERIFY(!_fwItem->dirty());
    _fwItem->setLoiterCoordinate(_fwItem->landingCoordinate().atDistanceAndAzimuth(1, 0));
    QVERIFY(_multiSpy->checkSignalByMask(dirtyChangedMask));
    QVERIFY(_multiSpy->pullBoolFromSignalIndex(dirtyChangedIndex));
    _fwItem->setDirty(false);
    _multiSpy->clearAllSignals();
}

void FWLandingPatternTest::_testSaveLoad(void)
{
    QJsonArray  items;
    _fwItem->save(items);

    QString errorString;
    FixedWingLandingComplexItem* newItem = new FixedWingLandingComplexItem(_offlineVehicle, false /* flyView */, this /* parent */);
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

    QVERIFY(fuzzyCompareLatLon(newItem->loiterCoordinate(),         _fwItem->loiterCoordinate()));
    QVERIFY(fuzzyCompareLatLon(newItem->loiterTangentCoordinate(),  _fwItem->loiterTangentCoordinate()));
    QVERIFY(fuzzyCompareLatLon(newItem->landingCoordinate(),        _fwItem->landingCoordinate()));

    QCOMPARE(newItem->stopTakingPhotos()->rawValue().toBool(),      _fwItem->stopTakingPhotos()->rawValue().toBool());
    QCOMPARE(newItem->stopTakingVideo()->rawValue().toBool(),       _fwItem->stopTakingVideo()->rawValue().toBool());
    QCOMPARE(newItem->loiterAltitude()->rawValue().toInt(),         _fwItem->loiterAltitude()->rawValue().toInt());
    QCOMPARE(newItem->loiterRadius()->rawValue().toInt(),           _fwItem->loiterRadius()->rawValue().toInt());
    QCOMPARE(newItem->landingAltitude()->rawValue().toInt(),        _fwItem->landingAltitude()->rawValue().toInt());
    QCOMPARE(newItem->landingHeading()->rawValue().toInt(),         _fwItem->landingHeading()->rawValue().toInt());
    QCOMPARE(newItem->landingDistance()->rawValue().toInt(),        _fwItem->landingDistance()->rawValue().toInt());
    QCOMPARE(newItem->glideSlope()->rawValue().toInt(),             _fwItem->glideSlope()->rawValue().toInt());
    QCOMPARE(newItem->valueSetIsDistance()->rawValue().toBool(),    _fwItem->valueSetIsDistance()->rawValue().toBool());
    QCOMPARE(newItem->_loiterClockwise,                             _fwItem->_loiterClockwise);
    QCOMPARE(newItem->_altitudesAreRelative,                        _fwItem->_altitudesAreRelative);
    QCOMPARE(newItem->_landingCoordSet,                             _fwItem->_landingCoordSet);
}
