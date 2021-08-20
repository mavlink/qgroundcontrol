/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "LandingComplexItemTest.h"
#include "QGCApplication.h"
#include "MissionCommandTree.h"
#include "MissionCommandUIInfo.h"
#include "CameraSectionTest.h"
#include "JsonHelper.h"

const char* SimpleLandingComplexItem::settingsGroup             = "SimpleLandingComplexItemUnitTest";
const char* SimpleLandingComplexItem::jsonComplexItemTypeValue  = "utSimpleLandingPattern";

LandingComplexItemTest::LandingComplexItemTest(void)
{    
    rgSignals[finalApproachCoordinateChangedIndex]  = SIGNAL(finalApproachCoordinateChanged(QGeoCoordinate));
    rgSignals[loiterTangentCoordinateChangedIndex]  = SIGNAL(loiterTangentCoordinateChanged(QGeoCoordinate));
    rgSignals[landingCoordinateChangedIndex]        = SIGNAL(landingCoordinateChanged(QGeoCoordinate));
    rgSignals[landingCoordSetChangedIndex]          = SIGNAL(landingCoordSetChanged(bool));
    rgSignals[altitudesAreRelativeChangedIndex]     = SIGNAL(altitudesAreRelativeChanged(bool));
    rgSignals[_updateFlightPathSegmentsSignalIndex] = SIGNAL(_updateFlightPathSegmentsSignal());
}

void LandingComplexItemTest::init(void)
{
    VisualMissionItemTest::init();

    _item = new SimpleLandingComplexItem(_masterController, false /* flyView */);

    // Start in a clean state
    QVERIFY(!_item->dirty());
    _item->setLandingCoordinate(QGeoCoordinate(47, -122));
    _item->setDirty(false);
    QVERIFY(!_item->dirty());

    VisualMissionItemTest::_createSpy(_item, &_viMultiSpy);

    _multiSpy = new MultiSignalSpy();
    QCOMPARE(_multiSpy->init(_item, rgSignals, cSignals), true);

    _validStopVideoItem     = CameraSectionTest::createValidStopTimeItem(_masterController);
    _validStopDistanceItem  = CameraSectionTest::createValidStopTimeItem(_masterController);
    _validStopTimeItem      = CameraSectionTest::createValidStopTimeItem(_masterController);
}

void LandingComplexItemTest::cleanup(void)
{
    delete _multiSpy;
    _multiSpy = nullptr;

    VisualMissionItemTest::cleanup();

    // These items go away when _masterController is deleted
    _item                   = nullptr;
    _validStopVideoItem     = nullptr;
    _validStopDistanceItem  = nullptr;
    _validStopTimeItem      = nullptr;
}

void LandingComplexItemTest::_testDirty(void)
{
    QVERIFY(!_item->dirty());
    _item->setDirty(true);
    QVERIFY(_item->dirty());
    QVERIFY(_viMultiSpy->checkOnlySignalByMask(dirtyChangedMask));
    QVERIFY(_viMultiSpy->pullBoolFromSignalIndex(dirtyChangedIndex));
    _item->setDirty(false);
    _viMultiSpy->clearAllSignals();

    // These facts should set dirty when changed
    QList<Fact*> rgFacts;
    rgFacts << _item->finalApproachAltitude()
            << _item->landingHeading()
            << _item->loiterRadius()
            << _item->loiterClockwise()
            << _item->landingAltitude()
            << _item->landingDistance()
            << _item->useLoiterToAlt()
            << _item->stopTakingPhotos()
            << _item->stopTakingVideo();
    for(Fact* fact: rgFacts) {
        qDebug() << fact->name();
        QVERIFY(!_item->dirty());
        changeFactValue(fact);
        QVERIFY(_viMultiSpy->checkSignalByMask(dirtyChangedMask));
        QVERIFY(_viMultiSpy->pullBoolFromSignalIndex(dirtyChangedIndex));
        _item->setDirty(false);
        _viMultiSpy->clearAllSignals();
    }

    // These bool properties should set dirty when changed
    QList<const char*> rgBoolNames;
    rgBoolNames << "altitudesAreRelative";
    const QMetaObject* metaObject = _item->metaObject();
    for(const char* boolName: rgBoolNames) {
        qDebug() << boolName;
        QVERIFY(!_item->dirty());
        QMetaProperty boolProp = metaObject->property(metaObject->indexOfProperty(boolName));
        QVERIFY(boolProp.write(_item, !boolProp.read(_item).toBool()));
        QVERIFY(_viMultiSpy->checkSignalByMask(dirtyChangedMask));
        QVERIFY(_viMultiSpy->pullBoolFromSignalIndex(dirtyChangedIndex));
        _item->setDirty(false);
        _viMultiSpy->clearAllSignals();
    }

    // These coordinates should set dirty when changed

    QVERIFY(!_item->dirty());
    _item->setFinalApproachCoordinate(changeCoordinateValue(_item->finalApproachCoordinate()));
    QVERIFY(_viMultiSpy->checkSignalByMask(dirtyChangedMask));
    QVERIFY(_viMultiSpy->pullBoolFromSignalIndex(dirtyChangedIndex));
    _item->setDirty(false);
    _viMultiSpy->clearAllSignals();

    QVERIFY(!_item->dirty());
    _item->setLandingCoordinate(changeCoordinateValue(_item->landingCoordinate()));
    QVERIFY(_viMultiSpy->checkSignalByMask(dirtyChangedMask));
    QVERIFY(_viMultiSpy->pullBoolFromSignalIndex(dirtyChangedIndex));
    _item->setDirty(false);
    _viMultiSpy->clearAllSignals();
}

void LandingComplexItemTest::_testItemCount(void)
{
    QList<MissionItem*> items;

    struct TestCase_s {
        bool stopTakingPhotos;
        bool stopTakingVideo;
    } rgTestCases[] = {
        { false, false },
        { false, true },
        { true, false },
        { true, true },
    };

    for (size_t i=0; i<sizeof(rgTestCases)/sizeof(rgTestCases[0]); i++) {
        TestCase_s& testCase = rgTestCases[i];

        _item->stopTakingPhotos()->setRawValue(testCase.stopTakingPhotos);
        _item->stopTakingVideo()->setRawValue(testCase.stopTakingVideo);
        _item->appendMissionItems(items, this);
        QCOMPARE(items.count(), 3 + (testCase.stopTakingPhotos * CameraSection::stopTakingPhotosCommandCount()) + (testCase.stopTakingVideo * CameraSection::stopTakingVideoCommandCount()));
        QCOMPARE(items.count() - 1, _item->lastSequenceNumber());
        items.clear();
    }
}

void LandingComplexItemTest::_testAppendSectionItems(void)
{
    QList<MissionItem*> rgMissionItems;

    struct TestCase_s {
        bool stopTakingPhotos;
        bool stopTakingVideo;
        bool useLoiterToAlt;
    } rgTestCases[] = {
        { false, false, false },
        { false, false, true },
        { false, true, false },
        { false, true, true },
        { true, false, false },
        { true, false, true },
        { true, true, false },
        { true, true, true },
    };

    for (size_t i=0; i<sizeof(rgTestCases)/sizeof(rgTestCases[0]); i++) {
        TestCase_s& testCase = rgTestCases[i];

        qDebug() << "stopTakingPhotos" << testCase.stopTakingPhotos << "stopTakingVideo" << testCase.stopTakingVideo << "useLoiterToAlt" << testCase.useLoiterToAlt;
        _item->stopTakingPhotos()->setRawValue(testCase.stopTakingPhotos);
        _item->stopTakingVideo()->setRawValue(testCase.stopTakingVideo);
        _item->useLoiterToAlt()->setRawValue(testCase.useLoiterToAlt);
        _item->appendMissionItems(rgMissionItems, this);

        // First item should be DO_LAND_START always
        QCOMPARE(rgMissionItems[0]->command(), MAV_CMD_DO_LAND_START);

        // Next come stop photo/video

        // Convert to simple visual items for verification
        QmlObjectListModel* simpleItems = new QmlObjectListModel(this);
        for (MissionItem* item: rgMissionItems) {
            SimpleMissionItem* simpleItem = new SimpleMissionItem(_masterController, false /* flyView */, false /* forLoad */);
            simpleItem->missionItem() = *item;
            simpleItems->append(simpleItem);
        }

        // Validate stop commands
        QCOMPARE(CameraSection::scanStopTakingPhotos(simpleItems, 1, false /* removeScannedItems */), testCase.stopTakingPhotos);
        QCOMPARE(CameraSection::scanStopTakingVideo(simpleItems, 1 + (testCase.stopTakingPhotos ? CameraSection::stopTakingPhotosCommandCount() : 0), false /* removeScannedItems */), testCase.stopTakingVideo);

        // Lastly is final approach item and land
        int finalApproachIndex = 1 + (testCase.stopTakingPhotos ? CameraSection::stopTakingPhotosCommandCount() : 0) + (testCase.stopTakingVideo ? CameraSection::stopTakingVideoCommandCount() : 0);
        QCOMPARE(rgMissionItems[finalApproachIndex]->command(), testCase.useLoiterToAlt ? MAV_CMD_NAV_LOITER_TO_ALT : MAV_CMD_NAV_WAYPOINT);
        qDebug() << rgMissionItems[finalApproachIndex+1]->command();
        QCOMPARE(rgMissionItems[finalApproachIndex+1]->command(), MAV_CMD_NAV_LAND);

        simpleItems->deleteLater();
        rgMissionItems.clear();
    }
}

void LandingComplexItemTest::_testScanForItems(void)
{
    QList<MissionItem*> rgMissionItems;

    struct TestCase_s {
        bool stopTakingPhotos;
        bool stopTakingVideo;
        bool useLoiterToAlt;
    } rgTestCases[] = {
        { false, false, false },
        { false, false, true },
        { false, true, false },
        { false, true, true },
        { true, false, false },
        { true, false, true },
        { true, true, false },
        { true, true, true },
    };

    for (size_t i=0; i<sizeof(rgTestCases)/sizeof(rgTestCases[0]); i++) {
        TestCase_s& testCase = rgTestCases[i];

        qDebug() << "stopTakingPhotos" << testCase.stopTakingPhotos << "stopTakingVideo" << testCase.stopTakingVideo << "useLoiterToAlt" << testCase.useLoiterToAlt;
        _item->stopTakingPhotos()->setRawValue(testCase.stopTakingPhotos);
        _item->stopTakingVideo()->setRawValue(testCase.stopTakingVideo);
        _item->useLoiterToAlt()->setRawValue(testCase.useLoiterToAlt);
        _item->appendMissionItems(rgMissionItems, this);

        // Convert to simple visual items for _scan
        QmlObjectListModel* visualItems = new QmlObjectListModel(this);
        for (MissionItem* item: rgMissionItems) {
            SimpleMissionItem* simpleItem = new SimpleMissionItem(_masterController, false /* flyView */, false /* forLoad */);
            simpleItem->missionItem() = *item;
            visualItems->append(simpleItem);
        }

        QVERIFY(LandingComplexItem::_scanForItem(visualItems, false /* flyView */, _masterController, &SimpleLandingComplexItem::_isValidLandItem, &SimpleLandingComplexItem::_createItem));
        QCOMPARE(visualItems->count(), 1);
        SimpleLandingComplexItem* scannedItem = visualItems->value<SimpleLandingComplexItem*>(0);
        QVERIFY(scannedItem);
        _validateItem(scannedItem, _item);

        visualItems->deleteLater();
        rgMissionItems.clear();
    }
}

void LandingComplexItemTest::_testSaveLoad(void)
{
    QString     errorString;

    QJsonObject saveObject = _item->_save();

    saveObject[JsonHelper::jsonVersionKey]                  = 1;
    saveObject[VisualMissionItem::jsonTypeKey]              = VisualMissionItem::jsonTypeComplexItemValue;
    saveObject[ComplexMissionItem::jsonComplexItemTypeKey]  = SimpleLandingComplexItem::jsonComplexItemTypeValue;

    // Test useDeprecatedRelAltKeys = false
    SimpleLandingComplexItem* newItem = new SimpleLandingComplexItem(_masterController, false /* flyView */);
    bool loadSuccess = newItem->_load(saveObject, 0 /* sequenceNumber */, SimpleLandingComplexItem::jsonComplexItemTypeValue, false /* useDeprecatedRelAltKeys */, errorString);
    if (!loadSuccess) {
        qDebug() << "_load failed" << errorString;
    }
    QVERIFY(loadSuccess);
    QVERIFY(errorString.isEmpty());
    _validateItem(newItem, _item);
    newItem->deleteLater();

    // Test useDeprecatedRelAltKeys = true
    bool relAlt = saveObject[LandingComplexItem::_jsonAltitudesAreRelativeKey].toBool();
    saveObject[LandingComplexItem::_jsonDeprecatedLoiterAltitudeRelativeKey] = relAlt;
    saveObject[LandingComplexItem::_jsonDeprecatedLandingAltitudeRelativeKey] = relAlt;
    saveObject.remove(LandingComplexItem::_jsonAltitudesAreRelativeKey);
    newItem = new SimpleLandingComplexItem(_masterController, false /* flyView */);
    loadSuccess = newItem->_load(saveObject, 0 /* sequenceNumber */, SimpleLandingComplexItem::jsonComplexItemTypeValue, true /* useDeprecatedRelAltKeys */, errorString);
    if (!loadSuccess) {
        qDebug() << "_load failed" << errorString;
    }
    QVERIFY(loadSuccess);
    QVERIFY(errorString.isEmpty());
    _validateItem(newItem, _item);
    newItem->deleteLater();

    // Test for _jsonDeprecatedLoiterCoordinateKey support
    saveObject                                                          = _item->_save();
    saveObject[JsonHelper::jsonVersionKey]                              = 1;
    saveObject[VisualMissionItem::jsonTypeKey]                          = VisualMissionItem::jsonTypeComplexItemValue;
    saveObject[ComplexMissionItem::jsonComplexItemTypeKey]              = SimpleLandingComplexItem::jsonComplexItemTypeValue;
    saveObject[LandingComplexItem::_jsonDeprecatedLoiterCoordinateKey]  = saveObject[LandingComplexItem::_jsonFinalApproachCoordinateKey];
    saveObject.remove(LandingComplexItem::_jsonFinalApproachCoordinateKey);
    newItem = new SimpleLandingComplexItem(_masterController, false /* flyView */);
    loadSuccess = newItem->_load(saveObject, 0 /* sequenceNumber */, SimpleLandingComplexItem::jsonComplexItemTypeValue, false /* useDeprecatedRelAltKeys */, errorString);
    if (!loadSuccess) {
        qDebug() << "_load failed" << errorString;
    }
    QVERIFY(loadSuccess);
    QVERIFY(errorString.isEmpty());
    _validateItem(newItem, _item);
    newItem->deleteLater();
}

void LandingComplexItemTest::_validateItem(LandingComplexItem* actualItem, LandingComplexItem* expectedItem)
{
    QVERIFY(actualItem);

    QCOMPARE(actualItem->stopTakingPhotos()->rawValue().toBool(),       expectedItem->stopTakingPhotos()->rawValue().toBool());
    QCOMPARE(actualItem->stopTakingVideo()->rawValue().toBool(),        expectedItem->stopTakingVideo()->rawValue().toBool());
    QCOMPARE(actualItem->useLoiterToAlt()->rawValue().toBool(),         expectedItem->useLoiterToAlt()->rawValue().toBool());
    QCOMPARE(actualItem->finalApproachAltitude()->rawValue().toInt(),   expectedItem->finalApproachAltitude()->rawValue().toInt());
    QCOMPARE(actualItem->landingAltitude()->rawValue().toInt(),         expectedItem->landingAltitude()->rawValue().toInt());
    QCOMPARE(actualItem->landingHeading()->rawValue().toInt(),          expectedItem->landingHeading()->rawValue().toInt());
    QCOMPARE(actualItem->landingDistance()->rawValue().toInt(),         expectedItem->landingDistance()->rawValue().toInt());
    QCOMPARE(actualItem->altitudesAreRelative(),                        expectedItem->altitudesAreRelative());
    QCOMPARE(actualItem->landingCoordSet(),                             expectedItem->landingCoordSet());

    QVERIFY(fuzzyCompareLatLon(actualItem->finalApproachCoordinate(),   expectedItem->finalApproachCoordinate()));
    QVERIFY(fuzzyCompareLatLon(actualItem->landingCoordinate(),         expectedItem->landingCoordinate()));
    if (actualItem->useLoiterToAlt()->rawValue().toBool()) {
        QVERIFY(fuzzyCompareLatLon(actualItem->loiterTangentCoordinate(), expectedItem->loiterTangentCoordinate()));
        QCOMPARE(actualItem->loiterRadius()->rawValue().toInt(),        expectedItem->loiterRadius()->rawValue().toInt());
        QCOMPARE(actualItem->loiterClockwise()->rawValue().toBool(),    expectedItem->loiterClockwise()->rawValue().toBool());
    }
}

SimpleLandingComplexItem::SimpleLandingComplexItem(PlanMasterController* masterController, bool flyView)
    : LandingComplexItem        (masterController, flyView)
    , _metaDataMap              (FactMetaData::createMapFromJsonFile(QStringLiteral(":/json/VTOLLandingPattern.FactMetaData.json"), this))
    , _landingDistanceFact      (settingsGroup, _metaDataMap[finalApproachToLandDistanceName])
    , _finalApproachAltitudeFact(settingsGroup, _metaDataMap[finalApproachAltitudeName])
    , _loiterRadiusFact         (settingsGroup, _metaDataMap[loiterRadiusName])
    , _loiterClockwiseFact      (settingsGroup, _metaDataMap[loiterClockwiseName])
    , _landingHeadingFact       (settingsGroup, _metaDataMap[landingHeadingName])
    , _landingAltitudeFact      (settingsGroup, _metaDataMap[landingAltitudeName])
    , _useLoiterToAltFact       (settingsGroup, _metaDataMap[useLoiterToAltName])
    , _stopTakingPhotosFact     (settingsGroup, _metaDataMap[stopTakingPhotosName])
    , _stopTakingVideoFact      (settingsGroup, _metaDataMap[stopTakingVideoName])
{
    _isIncomplete   = false;

    _init();
    _recalcFromHeadingAndDistanceChange();

    setDirty(false);
}

MissionItem* SimpleLandingComplexItem::_createLandItem(int seqNum, bool altRel, double lat, double lon, double alt, QObject* parent)
{
    return new MissionItem(seqNum,
                           MAV_CMD_NAV_LAND,
                           altRel ? MAV_FRAME_GLOBAL_RELATIVE_ALT : MAV_FRAME_GLOBAL,
                           0.0, 0.0, 0.0, 0.0,
                           lat, lon, alt,
                           true,                               // autoContinue
                           false,                              // isCurrentItem
                           parent);

}

bool SimpleLandingComplexItem::_isValidLandItem(const MissionItem& missionItem)
{
    if (missionItem.command() != MAV_CMD_NAV_LAND ||
            !(missionItem.frame() == MAV_FRAME_GLOBAL_RELATIVE_ALT || missionItem.frame() == MAV_FRAME_GLOBAL) ||
            missionItem.param1() != 0 || missionItem.param2() != 0 || missionItem.param3() != 0 || missionItem.param4() != 0 ||
            qIsNaN(missionItem.param5()) || qIsNaN(missionItem.param6()) || qIsNaN(missionItem.param7())) {
        return false;
    } else {
        return true;
    }
}

// Never call this method directly. If you want to update the flight segments you emit _updateFlightPathSegmentsSignal()
void SimpleLandingComplexItem::_updateFlightPathSegmentsDontCallDirectly(void)
{
    if (_cTerrainCollisionSegments != 0) {
        _cTerrainCollisionSegments = 0;
        emit terrainCollisionChanged(false);
    }

    _flightPathSegments.beginReset();
    _flightPathSegments.clearAndDeleteContents();
    if (useLoiterToAlt()->rawValue().toBool()) {
        _appendFlightPathSegment(FlightPathSegment::SegmentTypeGeneric, finalApproachCoordinate(), amslEntryAlt(), loiterTangentCoordinate(),  amslEntryAlt()); // Best we can do to simulate loiter circle terrain profile
        _appendFlightPathSegment(FlightPathSegment::SegmentTypeLand, loiterTangentCoordinate(), amslEntryAlt(), landingCoordinate(),        amslExitAlt());
    } else {
        _appendFlightPathSegment(FlightPathSegment::SegmentTypeLand, finalApproachCoordinate(), amslEntryAlt(), landingCoordinate(),        amslExitAlt());
    }
    _flightPathSegments.endReset();

    if (_cTerrainCollisionSegments != 0) {
        emit terrainCollisionChanged(true);
    }

    _masterController->missionController()->recalcTerrainProfile();
}
