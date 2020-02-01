/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "FixedWingLandingComplexItem.h"
#include "JsonHelper.h"
#include "MissionController.h"
#include "QGCGeo.h"
#include "QGroundControlQmlGlobal.h"
#include "SimpleMissionItem.h"

#include <QPolygonF>

QGC_LOGGING_CATEGORY(FixedWingLandingComplexItemLog, "FixedWingLandingComplexItemLog")

const char* FixedWingLandingComplexItem::settingsGroup =            "FixedWingLanding";
const char* FixedWingLandingComplexItem::jsonComplexItemTypeValue = "fwLandingPattern";

const char* FixedWingLandingComplexItem::loiterToLandDistanceName = "LandingDistance";
const char* FixedWingLandingComplexItem::landingHeadingName =       "LandingHeading";
const char* FixedWingLandingComplexItem::loiterAltitudeName =       "LoiterAltitude";
const char* FixedWingLandingComplexItem::loiterRadiusName =         "LoiterRadius";
const char* FixedWingLandingComplexItem::landingAltitudeName =      "LandingAltitude";
const char* FixedWingLandingComplexItem::glideSlopeName =           "GlideSlope";
const char* FixedWingLandingComplexItem::stopTakingPhotosName =     "StopTakingPhotos";
const char* FixedWingLandingComplexItem::stopTakingVideoName =      "StopTakingVideo";
const char* FixedWingLandingComplexItem::valueSetIsDistanceName =   "ValueSetIsDistance";

const char* FixedWingLandingComplexItem::_jsonLoiterCoordinateKey =         "loiterCoordinate";
const char* FixedWingLandingComplexItem::_jsonLoiterRadiusKey =             "loiterRadius";
const char* FixedWingLandingComplexItem::_jsonLoiterClockwiseKey =          "loiterClockwise";
const char* FixedWingLandingComplexItem::_jsonLandingCoordinateKey =        "landCoordinate";
const char* FixedWingLandingComplexItem::_jsonValueSetIsDistanceKey =       "valueSetIsDistance";
const char* FixedWingLandingComplexItem::_jsonAltitudesAreRelativeKey =     "altitudesAreRelative";
const char* FixedWingLandingComplexItem::_jsonStopTakingPhotosKey =         "stopTakingPhotos";
const char* FixedWingLandingComplexItem::_jsonStopTakingVideoKey =          "stopVideoPhotos";

// Usage deprecated
const char* FixedWingLandingComplexItem::_jsonFallRateKey =                 "fallRate";
const char* FixedWingLandingComplexItem::_jsonLandingAltitudeRelativeKey =  "landAltitudeRelative";
const char* FixedWingLandingComplexItem::_jsonLoiterAltitudeRelativeKey =   "loiterAltitudeRelative";

FixedWingLandingComplexItem::FixedWingLandingComplexItem(Vehicle* vehicle, bool flyView, QObject* parent)
    : ComplexMissionItem        (vehicle, flyView, parent)
    , _sequenceNumber           (0)
    , _dirty                    (false)
    , _landingCoordSet          (false)
    , _ignoreRecalcSignals      (false)
    , _loiterDragAngleOnly      (false)
    , _metaDataMap              (FactMetaData::createMapFromJsonFile(QStringLiteral(":/json/FWLandingPattern.FactMetaData.json"), this))
    , _landingDistanceFact      (settingsGroup, _metaDataMap[loiterToLandDistanceName])
    , _loiterAltitudeFact       (settingsGroup, _metaDataMap[loiterAltitudeName])
    , _loiterRadiusFact         (settingsGroup, _metaDataMap[loiterRadiusName])
    , _landingHeadingFact       (settingsGroup, _metaDataMap[landingHeadingName])
    , _landingAltitudeFact      (settingsGroup, _metaDataMap[landingAltitudeName])
    , _glideSlopeFact           (settingsGroup, _metaDataMap[glideSlopeName])
    , _stopTakingPhotosFact     (settingsGroup, _metaDataMap[stopTakingPhotosName])
    , _stopTakingVideoFact      (settingsGroup, _metaDataMap[stopTakingVideoName])
    , _valueSetIsDistanceFact   (settingsGroup, _metaDataMap[valueSetIsDistanceName])
    , _loiterClockwise          (true)
    , _altitudesAreRelative     (true)
{
    _editorQml = "qrc:/qml/FWLandingPatternEditor.qml";
    _isIncomplete = false;

    connect(&_loiterAltitudeFact,       &Fact::valueChanged,                                    this, &FixedWingLandingComplexItem::_updateLoiterCoodinateAltitudeFromFact);
    connect(&_landingAltitudeFact,      &Fact::valueChanged,                                    this, &FixedWingLandingComplexItem::_updateLandingCoodinateAltitudeFromFact);

    connect(&_landingDistanceFact,      &Fact::valueChanged,                                    this, &FixedWingLandingComplexItem::_recalcFromHeadingAndDistanceChange);
    connect(&_landingHeadingFact,       &Fact::valueChanged,                                    this, &FixedWingLandingComplexItem::_recalcFromHeadingAndDistanceChange);

    connect(&_loiterRadiusFact,         &Fact::valueChanged,                                    this, &FixedWingLandingComplexItem::_recalcFromRadiusChange);
    connect(this,                       &FixedWingLandingComplexItem::loiterClockwiseChanged,   this, &FixedWingLandingComplexItem::_recalcFromRadiusChange);

    connect(this,                       &FixedWingLandingComplexItem::loiterCoordinateChanged,  this, &FixedWingLandingComplexItem::_recalcFromCoordinateChange);
    connect(this,                       &FixedWingLandingComplexItem::landingCoordinateChanged, this, &FixedWingLandingComplexItem::_recalcFromCoordinateChange);

    connect(&_glideSlopeFact,           &Fact::valueChanged,                                    this, &FixedWingLandingComplexItem::_glideSlopeChanged);

    connect(&_stopTakingPhotosFact,     &Fact::valueChanged,                                            this, &FixedWingLandingComplexItem::_signalLastSequenceNumberChanged);
    connect(&_stopTakingVideoFact,      &Fact::valueChanged,                                            this, &FixedWingLandingComplexItem::_signalLastSequenceNumberChanged);

    connect(&_loiterAltitudeFact,       &Fact::valueChanged,                                            this, &FixedWingLandingComplexItem::_setDirty);
    connect(&_landingAltitudeFact,      &Fact::valueChanged,                                            this, &FixedWingLandingComplexItem::_setDirty);
    connect(&_landingDistanceFact,      &Fact::valueChanged,                                            this, &FixedWingLandingComplexItem::_setDirty);
    connect(&_landingHeadingFact,       &Fact::valueChanged,                                            this, &FixedWingLandingComplexItem::_setDirty);
    connect(&_loiterRadiusFact,         &Fact::valueChanged,                                            this, &FixedWingLandingComplexItem::_setDirty);
    connect(&_stopTakingPhotosFact,     &Fact::valueChanged,                                            this, &FixedWingLandingComplexItem::_setDirty);
    connect(&_stopTakingVideoFact,      &Fact::valueChanged,                                            this, &FixedWingLandingComplexItem::_setDirty);
    connect(&_valueSetIsDistanceFact,   &Fact::valueChanged,                                            this, &FixedWingLandingComplexItem::_setDirty);
    connect(this,                       &FixedWingLandingComplexItem::loiterCoordinateChanged,          this, &FixedWingLandingComplexItem::_setDirty);
    connect(this,                       &FixedWingLandingComplexItem::landingCoordinateChanged,         this, &FixedWingLandingComplexItem::_setDirty);
    connect(this,                       &FixedWingLandingComplexItem::loiterClockwiseChanged,           this, &FixedWingLandingComplexItem::_setDirty);
    connect(this,                       &FixedWingLandingComplexItem::altitudesAreRelativeChanged,      this, &FixedWingLandingComplexItem::_setDirty);
    connect(this,                       &FixedWingLandingComplexItem::valueSetIsDistanceChanged,        this, &FixedWingLandingComplexItem::_setDirty);

    connect(this,                       &FixedWingLandingComplexItem::altitudesAreRelativeChanged,      this, &FixedWingLandingComplexItem::coordinateHasRelativeAltitudeChanged);
    connect(this,                       &FixedWingLandingComplexItem::altitudesAreRelativeChanged,      this, &FixedWingLandingComplexItem::exitCoordinateHasRelativeAltitudeChanged);

    connect(this,                       &FixedWingLandingComplexItem::landingCoordSetChanged,           this, &FixedWingLandingComplexItem::readyForSaveStateChanged);
    connect(this,                       &FixedWingLandingComplexItem::wizardModeChanged,                this, &FixedWingLandingComplexItem::readyForSaveStateChanged);

    if (vehicle->apmFirmware()) {
        // ArduPilot does not support camera commands
        _stopTakingVideoFact.setRawValue(false);
        _stopTakingPhotosFact.setRawValue(false);
    }

    if (_valueSetIsDistanceFact.rawValue().toBool()) {
        _recalcFromHeadingAndDistanceChange();
    } else {
        _glideSlopeChanged();
    }
    setDirty(false);
}

int FixedWingLandingComplexItem::lastSequenceNumber(void) const
{
    // Fixed items are:
    //  land start, loiter, land
    // Optional items are:
    //  stop photos/video
    return _sequenceNumber + 2 + (_stopTakingPhotosFact.rawValue().toBool() ? 2 : 0) + (_stopTakingVideoFact.rawValue().toBool() ? 1 : 0);
}

void FixedWingLandingComplexItem::setDirty(bool dirty)
{
    if (_dirty != dirty) {
        _dirty = dirty;
        emit dirtyChanged(_dirty);
    }
}

void FixedWingLandingComplexItem::save(QJsonArray&  missionItems)
{
    QJsonObject saveObject;

    saveObject[JsonHelper::jsonVersionKey] =                    2;
    saveObject[VisualMissionItem::jsonTypeKey] =                VisualMissionItem::jsonTypeComplexItemValue;
    saveObject[ComplexMissionItem::jsonComplexItemTypeKey] =    jsonComplexItemTypeValue;

    QGeoCoordinate coordinate;
    QJsonValue jsonCoordinate;

    coordinate = _loiterCoordinate;
    coordinate.setAltitude(_loiterAltitudeFact.rawValue().toDouble());
    JsonHelper::saveGeoCoordinate(coordinate, true /* writeAltitude */, jsonCoordinate);
    saveObject[_jsonLoiterCoordinateKey] = jsonCoordinate;

    coordinate = _landingCoordinate;
    coordinate.setAltitude(_landingAltitudeFact.rawValue().toDouble());
    JsonHelper::saveGeoCoordinate(coordinate, true /* writeAltitude */, jsonCoordinate);
    saveObject[_jsonLandingCoordinateKey] = jsonCoordinate;

    saveObject[_jsonLoiterRadiusKey] =          _loiterRadiusFact.rawValue().toDouble();
    saveObject[_jsonStopTakingPhotosKey] =      _stopTakingPhotosFact.rawValue().toBool();
    saveObject[_jsonStopTakingVideoKey] =       _stopTakingVideoFact.rawValue().toBool();
    saveObject[_jsonLoiterClockwiseKey] =       _loiterClockwise;
    saveObject[_jsonAltitudesAreRelativeKey] =  _altitudesAreRelative;
    saveObject[_jsonValueSetIsDistanceKey] =    _valueSetIsDistanceFact.rawValue().toBool();

    missionItems.append(saveObject);
}

void FixedWingLandingComplexItem::setSequenceNumber(int sequenceNumber)
{
    if (_sequenceNumber != sequenceNumber) {
        _sequenceNumber = sequenceNumber;
        emit sequenceNumberChanged(sequenceNumber);
        emit lastSequenceNumberChanged(lastSequenceNumber());
    }
}

bool FixedWingLandingComplexItem::load(const QJsonObject& complexObject, int sequenceNumber, QString& errorString)
{
    QList<JsonHelper::KeyValidateInfo> keyInfoList = {
        { JsonHelper::jsonVersionKey,                   QJsonValue::Double, true },
        { VisualMissionItem::jsonTypeKey,               QJsonValue::String, true },
        { ComplexMissionItem::jsonComplexItemTypeKey,   QJsonValue::String, true },
        { _jsonLoiterCoordinateKey,                     QJsonValue::Array,  true },
        { _jsonLoiterRadiusKey,                         QJsonValue::Double, true },
        { _jsonLoiterClockwiseKey,                      QJsonValue::Bool,   true },
        { _jsonLandingCoordinateKey,                    QJsonValue::Array,  true },
        { _jsonStopTakingPhotosKey,                     QJsonValue::Bool,   false },
        { _jsonStopTakingVideoKey,                      QJsonValue::Bool,   false },
    };
    if (!JsonHelper::validateKeys(complexObject, keyInfoList, errorString)) {
        return false;
    }

    QString itemType = complexObject[VisualMissionItem::jsonTypeKey].toString();
    QString complexType = complexObject[ComplexMissionItem::jsonComplexItemTypeKey].toString();
    if (itemType != VisualMissionItem::jsonTypeComplexItemValue || complexType != jsonComplexItemTypeValue) {
        errorString = tr("%1 does not support loading this complex mission item type: %2:%3").arg(qgcApp()->applicationName()).arg(itemType).arg(complexType);
        return false;
    }

    setSequenceNumber(sequenceNumber);

    _ignoreRecalcSignals = true;

    int version = complexObject[JsonHelper::jsonVersionKey].toInt();
    if (version == 1) {
        QList<JsonHelper::KeyValidateInfo> v1KeyInfoList = {
            { _jsonLoiterAltitudeRelativeKey,   QJsonValue::Bool,  true },
            { _jsonLandingAltitudeRelativeKey,  QJsonValue::Bool,  true },
        };
        if (!JsonHelper::validateKeys(complexObject, v1KeyInfoList, errorString)) {
            return false;
        }

        bool loiterAltitudeRelative = complexObject[_jsonLoiterAltitudeRelativeKey].toBool();
        bool landingAltitudeRelative = complexObject[_jsonLandingAltitudeRelativeKey].toBool();
        if (loiterAltitudeRelative != landingAltitudeRelative) {
            qgcApp()->showMessage(tr("Fixed Wing Landing Pattern: "
                                     "Setting the loiter and landing altitudes with different settings for altitude relative is no longer supported. "
                                     "Both have been set to altitude relative. Be sure to adjust/check your plan prior to flight."));
            _altitudesAreRelative = true;
        } else {
            _altitudesAreRelative = loiterAltitudeRelative;
        }
        _valueSetIsDistanceFact.setRawValue(true);
    } else if (version == 2) {
        QList<JsonHelper::KeyValidateInfo> v2KeyInfoList = {
            { _jsonAltitudesAreRelativeKey, QJsonValue::Bool,  true },
            { _jsonValueSetIsDistanceKey,   QJsonValue::Bool,  true },
        };
        if (!JsonHelper::validateKeys(complexObject, v2KeyInfoList, errorString)) {
            _ignoreRecalcSignals = false;
            return false;
        }

        _altitudesAreRelative = complexObject[_jsonAltitudesAreRelativeKey].toBool();
        _valueSetIsDistanceFact.setRawValue(complexObject[_jsonValueSetIsDistanceKey].toBool());
    } else {
        errorString = tr("%1 complex item version %2 not supported").arg(jsonComplexItemTypeValue).arg(version);
        _ignoreRecalcSignals = false;
        return false;
    }

    QGeoCoordinate coordinate;
    if (!JsonHelper::loadGeoCoordinate(complexObject[_jsonLoiterCoordinateKey], true /* altitudeRequired */, coordinate, errorString)) {
        return false;
    }
    _loiterCoordinate = coordinate;
    _loiterAltitudeFact.setRawValue(coordinate.altitude());

    if (!JsonHelper::loadGeoCoordinate(complexObject[_jsonLandingCoordinateKey], true /* altitudeRequired */, coordinate, errorString)) {
        return false;
    }
    _landingCoordinate = coordinate;
    _landingAltitudeFact.setRawValue(coordinate.altitude());

    _loiterRadiusFact.setRawValue(complexObject[_jsonLoiterRadiusKey].toDouble());
    _loiterClockwise = complexObject[_jsonLoiterClockwiseKey].toBool();

    if (complexObject.contains(_jsonStopTakingPhotosKey)) {
        _stopTakingPhotosFact.setRawValue(complexObject[_jsonStopTakingPhotosKey].toBool());
    } else {
        _stopTakingPhotosFact.setRawValue(false);
    }
    if (complexObject.contains(_jsonStopTakingVideoKey)) {
        _stopTakingVideoFact.setRawValue(complexObject[_jsonStopTakingVideoKey].toBool());
    } else {
        _stopTakingVideoFact.setRawValue(false);
    }

    _calcGlideSlope();

    _landingCoordSet = true;

    _ignoreRecalcSignals = false;

    _recalcFromCoordinateChange();
    emit coordinateChanged(this->coordinate());    // This will kick off terrain query

    return true;
}

double FixedWingLandingComplexItem::greatestDistanceTo(const QGeoCoordinate &other) const
{
    return qMax(_loiterCoordinate.distanceTo(other),_landingCoordinate.distanceTo(other));
}

bool FixedWingLandingComplexItem::specifiesCoordinate(void) const
{
    return true;
}

MissionItem* FixedWingLandingComplexItem::createDoLandStartItem(int seqNum, QObject* parent)
{
    return new MissionItem(seqNum,                              // sequence number
                           MAV_CMD_DO_LAND_START,               // MAV_CMD
                           MAV_FRAME_MISSION,                   // MAV_FRAME
                           0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,   // param 1-7
                           true,                                // autoContinue
                           false,                               // isCurrentItem
                           parent);
}

MissionItem* FixedWingLandingComplexItem::createLoiterToAltItem(int seqNum, bool altRel, double loiterRadius, double lat, double lon, double alt, QObject* parent)
{
    return new MissionItem(seqNum,
                           MAV_CMD_NAV_LOITER_TO_ALT,
                           altRel ? MAV_FRAME_GLOBAL_RELATIVE_ALT : MAV_FRAME_GLOBAL,
                           1.0,                             // Heading required = true
                           loiterRadius,                    // Loiter radius
                           0.0,                             // param 3 - unused
                           1.0,                             // Exit crosstrack - tangent of loiter to land point
                           lat, lon, alt,
                           true,                            // autoContinue
                           false,                           // isCurrentItem
                           parent);
}

MissionItem* FixedWingLandingComplexItem::createLandItem(int seqNum, bool altRel, double lat, double lon, double alt, QObject* parent)
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

void FixedWingLandingComplexItem::appendMissionItems(QList<MissionItem*>& items, QObject* missionItemParent)
{
    int seqNum = _sequenceNumber;

    // IMPORTANT NOTE: Any changes here must also be taken into account in scanForItem

    MissionItem* item = createDoLandStartItem(seqNum++, missionItemParent);
    items.append(item);


    if (_stopTakingPhotosFact.rawValue().toBool()) {
        CameraSection::appendStopTakingPhotos(items, seqNum, missionItemParent);
    }

    if (_stopTakingVideoFact.rawValue().toBool()) {
        CameraSection::appendStopTakingVideo(items, seqNum, missionItemParent);
    }

    double loiterRadius = _loiterRadiusFact.rawValue().toDouble() * (_loiterClockwise ? 1.0 : -1.0);
    item = createLoiterToAltItem(seqNum++,
                                 _altitudesAreRelative,
                                 loiterRadius,
                                 _loiterCoordinate.latitude(), _loiterCoordinate.longitude(), _loiterAltitudeFact.rawValue().toDouble(),
                                 missionItemParent);
    items.append(item);

    item = createLandItem(seqNum++,
                          _altitudesAreRelative,
                          _landingCoordinate.latitude(), _landingCoordinate.longitude(), _landingAltitudeFact.rawValue().toDouble(),
                          missionItemParent);
    items.append(item);
}

bool FixedWingLandingComplexItem::scanForItem(QmlObjectListModel* visualItems, bool flyView, Vehicle* vehicle)
{
    qCDebug(FixedWingLandingComplexItemLog) << "FixedWingLandingComplexItem::scanForItem count" << visualItems->count();

    if (visualItems->count() < 3) {
        return false;
    }

    // A valid fixed wing landing pattern is comprised of the follow commands in this order at the end of the item list:
    //  MAV_CMD_DO_LAND_START - required
    //  Stop taking photos sequence - optional
    //  Stop taking video sequence - optional
    //  MAV_CMD_NAV_LOITER_TO_ALT - required
    //  MAV_CMD_NAV_LAND - required

    // Start looking for the commands in reverse order from the end of the list

    int scanIndex = visualItems->count() - 1;

    if (scanIndex < 0 || scanIndex > visualItems->count() - 1) {
        return false;
    }
    SimpleMissionItem* item = visualItems->value<SimpleMissionItem*>(scanIndex--);
    if (!item) {
        return false;
    }
    MissionItem& missionItemLand = item->missionItem();
    if (missionItemLand.command() != MAV_CMD_NAV_LAND ||
            !(missionItemLand.frame() == MAV_FRAME_GLOBAL_RELATIVE_ALT || missionItemLand.frame() == MAV_FRAME_GLOBAL) ||
            missionItemLand.param1() != 0 || missionItemLand.param2() != 0 || missionItemLand.param3() != 0 || missionItemLand.param4() != 0) {
        return false;
    }
    MAV_FRAME landPointFrame = missionItemLand.frame();

    if (scanIndex < 0 || scanIndex > visualItems->count() - 1) {
        return false;
    }
    item = visualItems->value<SimpleMissionItem*>(scanIndex);
    if (!item) {
        return false;
    }
    MissionItem& missionItemLoiter = item->missionItem();
    if (missionItemLoiter.command() != MAV_CMD_NAV_LOITER_TO_ALT ||
            missionItemLoiter.frame() != landPointFrame ||
            missionItemLoiter.param1() != 1.0 || missionItemLoiter.param3() != 0 || missionItemLoiter.param4() != 1.0) {
        return false;
    }

    scanIndex -= CameraSection::stopTakingVideoCommandCount();
    bool stopTakingVideo = CameraSection::scanStopTakingVideo(visualItems, scanIndex, false /* removeScannedItems */);
    if (!stopTakingVideo) {
        scanIndex += CameraSection::stopTakingVideoCommandCount();
    }

    scanIndex -= CameraSection::stopTakingPhotosCommandCount();
    bool stopTakingPhotos = CameraSection::scanStopTakingPhotos(visualItems, scanIndex, false /* removeScannedItems */);
    if (!stopTakingPhotos) {
        scanIndex += CameraSection::stopTakingPhotosCommandCount();
    }

    scanIndex--;
    if (scanIndex < 0 || scanIndex > visualItems->count() - 1) {
        return false;
    }
    item = visualItems->value<SimpleMissionItem*>(scanIndex);
    if (!item) {
        return false;
    }
    MissionItem& missionItemDoLandStart = item->missionItem();
    if (missionItemDoLandStart.command() != MAV_CMD_DO_LAND_START ||
            missionItemDoLandStart.frame() != MAV_FRAME_MISSION ||
            missionItemDoLandStart.param1() != 0 || missionItemDoLandStart.param2() != 0 || missionItemDoLandStart.param3() != 0 || missionItemDoLandStart.param4() != 0|| missionItemDoLandStart.param5() != 0|| missionItemDoLandStart.param6() != 0) {
        return false;
    }

    // We made it this far so we do have a Fixed Wing Landing Pattern item at the end of the mission.
    // Since we have scanned it we need to remove the items for it fromt the list
    int deleteCount = 3;
    if (stopTakingPhotos) {
        deleteCount += CameraSection::stopTakingPhotosCommandCount();
    }
    if (stopTakingVideo) {
        deleteCount += CameraSection::stopTakingVideoCommandCount();
    }
    int firstItem = visualItems->count() - deleteCount;
    while (deleteCount--) {
        visualItems->removeAt(firstItem)->deleteLater();
    }

    // Now stuff all the scanned information into the item

    FixedWingLandingComplexItem* complexItem = new FixedWingLandingComplexItem(vehicle, flyView, visualItems);

    complexItem->_ignoreRecalcSignals = true;

    complexItem->_altitudesAreRelative = landPointFrame == MAV_FRAME_GLOBAL_RELATIVE_ALT;
    complexItem->_loiterRadiusFact.setRawValue(qAbs(missionItemLoiter.param2()));
    complexItem->_loiterClockwise = missionItemLoiter.param2() > 0;
    complexItem->setLoiterCoordinate(QGeoCoordinate(missionItemLoiter.param5(), missionItemLoiter.param6()));
    complexItem->_loiterAltitudeFact.setRawValue(missionItemLoiter.param7());

    complexItem->_landingCoordinate.setLatitude(missionItemLand.param5());
    complexItem->_landingCoordinate.setLongitude(missionItemLand.param6());
    complexItem->_landingAltitudeFact.setRawValue(missionItemLand.param7());

    complexItem->_stopTakingPhotosFact.setRawValue(stopTakingPhotos);
    complexItem->_stopTakingVideoFact.setRawValue(stopTakingVideo);

    complexItem->_calcGlideSlope();

    complexItem->_landingCoordSet = true;

    complexItem->_ignoreRecalcSignals = false;
    complexItem->_recalcFromCoordinateChange();
    complexItem->setDirty(false);

    visualItems->append(complexItem);

    return true;
}

double FixedWingLandingComplexItem::complexDistance(void) const
{
    return _loiterCoordinate.distanceTo(_landingCoordinate);
}

void FixedWingLandingComplexItem::setLandingCoordinate(const QGeoCoordinate& coordinate)
{
    if (coordinate != _landingCoordinate) {
        _landingCoordinate = coordinate;
        if (_landingCoordSet) {
            emit exitCoordinateChanged(coordinate);
            emit landingCoordinateChanged(coordinate);
        } else {
            _ignoreRecalcSignals = true;
            emit exitCoordinateChanged(coordinate);
            emit landingCoordinateChanged(coordinate);
            _ignoreRecalcSignals = false;
            _landingCoordSet = true;
            _recalcFromHeadingAndDistanceChange();
            emit landingCoordSetChanged(true);
        }
    }
}

void FixedWingLandingComplexItem::setLoiterCoordinate(const QGeoCoordinate& coordinate)
{
    if (coordinate != _loiterCoordinate) {
        _loiterCoordinate = coordinate;
        emit coordinateChanged(coordinate);
        emit loiterCoordinateChanged(coordinate);
    }
}

double FixedWingLandingComplexItem::_mathematicAngleToHeading(double angle)
{
    double heading = (angle - 90) * -1;
    if (heading < 0) {
        heading += 360;
    }

    return heading;
}

double FixedWingLandingComplexItem::_headingToMathematicAngle(double heading)
{
    return heading - 90 * -1;
}

void FixedWingLandingComplexItem::_recalcFromRadiusChange(void)
{
    // Fixed:
    //      land
    //      loiter tangent
    //      distance
    //      radius
    //      heading
    // Adjusted:
    //      loiter

    if (!_ignoreRecalcSignals) {
        // These are our known values
        double radius  = _loiterRadiusFact.rawValue().toDouble();
        double landToTangentDistance = _landingDistanceFact.rawValue().toDouble();
        double heading = _landingHeadingFact.rawValue().toDouble();

        double landToLoiterDistance = _landingCoordinate.distanceTo(_loiterCoordinate);
        if (landToLoiterDistance < radius) {
            // Degnenerate case: Move tangent to loiter point
            _loiterTangentCoordinate = _loiterCoordinate;

            double heading = _landingCoordinate.azimuthTo(_loiterTangentCoordinate);

            _ignoreRecalcSignals = true;
            _landingHeadingFact.setRawValue(heading);
            emit loiterTangentCoordinateChanged(_loiterTangentCoordinate);
            _ignoreRecalcSignals = false;
        } else {
            double landToLoiterDistance = qSqrt(qPow(radius, 2) + qPow(landToTangentDistance, 2));
            double angleLoiterToTangent = qRadiansToDegrees(qAsin(radius/landToLoiterDistance)) * (_loiterClockwise ? -1 : 1);

            _loiterCoordinate = _landingCoordinate.atDistanceAndAzimuth(landToLoiterDistance, heading + 180 + angleLoiterToTangent);
            _loiterCoordinate.setAltitude(_loiterAltitudeFact.rawValue().toDouble());

            _ignoreRecalcSignals = true;
            emit loiterCoordinateChanged(_loiterCoordinate);
            emit coordinateChanged(_loiterCoordinate);
            _ignoreRecalcSignals = false;
        }
    }
}

void FixedWingLandingComplexItem::_recalcFromHeadingAndDistanceChange(void)
{
    // Fixed:
    //      land
    //      heading
    //      distance
    //      radius
    // Adjusted:
    //      loiter
    //      loiter tangent
    //      glide slope

    if (!_ignoreRecalcSignals && _landingCoordSet) {
        // These are our known values
        double radius = _loiterRadiusFact.rawValue().toDouble();
        double landToTangentDistance = _landingDistanceFact.rawValue().toDouble();
        double heading = _landingHeadingFact.rawValue().toDouble();

        // Calculate loiter tangent coordinate
        _loiterTangentCoordinate = _landingCoordinate.atDistanceAndAzimuth(landToTangentDistance, heading + 180);

        // Calculate the distance and angle to the loiter coordinate
        QGeoCoordinate tangent = _landingCoordinate.atDistanceAndAzimuth(landToTangentDistance, 0);
        QGeoCoordinate loiter = tangent.atDistanceAndAzimuth(radius, 90);
        double loiterDistance = _landingCoordinate.distanceTo(loiter);
        double loiterAzimuth = _landingCoordinate.azimuthTo(loiter) * (_loiterClockwise ? -1 : 1);

        // Use those values to get the new loiter point which takes heading into acount
        _loiterCoordinate = _landingCoordinate.atDistanceAndAzimuth(loiterDistance, heading + 180 + loiterAzimuth);
        _loiterCoordinate.setAltitude(_loiterAltitudeFact.rawValue().toDouble());

        _ignoreRecalcSignals = true;
        emit loiterTangentCoordinateChanged(_loiterTangentCoordinate);
        emit loiterCoordinateChanged(_loiterCoordinate);
        emit coordinateChanged(_loiterCoordinate);
        _calcGlideSlope();
        _ignoreRecalcSignals = false;
    }
}

QPointF FixedWingLandingComplexItem::_rotatePoint(const QPointF& point, const QPointF& origin, double angle)
{
    QPointF rotated;
    double radians = (M_PI / 180.0) * angle;

    rotated.setX(((point.x() - origin.x()) * cos(radians)) - ((point.y() - origin.y()) * sin(radians)) + origin.x());
    rotated.setY(((point.x() - origin.x()) * sin(radians)) + ((point.y() - origin.y()) * cos(radians)) + origin.y());

    return rotated;
}

void FixedWingLandingComplexItem::_recalcFromCoordinateChange(void)
{
    // Fixed:
    //      land
    //      loiter
    //      radius
    // Adjusted:
    //      loiter tangent
    //      heading
    //      distance
    //      glide slope

    if (!_ignoreRecalcSignals && _landingCoordSet) {
        // These are our known values
        double radius = _loiterRadiusFact.rawValue().toDouble();
        double landToLoiterDistance = _landingCoordinate.distanceTo(_loiterCoordinate);
        double landToLoiterHeading = _landingCoordinate.azimuthTo(_loiterCoordinate);

        double landToTangentDistance;
        if (landToLoiterDistance < radius) {
            // Degenerate case, set tangent to loiter coordinate
            _loiterTangentCoordinate = _loiterCoordinate;
            landToTangentDistance = _landingCoordinate.distanceTo(_loiterTangentCoordinate);
        } else {
            double loiterToTangentAngle = qRadiansToDegrees(qAsin(radius/landToLoiterDistance)) * (_loiterClockwise ? 1 : -1);
            landToTangentDistance = qSqrt(qPow(landToLoiterDistance, 2) - qPow(radius, 2));

            _loiterTangentCoordinate = _landingCoordinate.atDistanceAndAzimuth(landToTangentDistance, landToLoiterHeading + loiterToTangentAngle);

        }

        double heading = _loiterTangentCoordinate.azimuthTo(_landingCoordinate);

        _ignoreRecalcSignals = true;
        _landingHeadingFact.setRawValue(heading);
        _landingDistanceFact.setRawValue(landToTangentDistance);
        emit loiterTangentCoordinateChanged(_loiterTangentCoordinate);
        _calcGlideSlope();
        _ignoreRecalcSignals = false;
    }
}

void FixedWingLandingComplexItem::_updateLoiterCoodinateAltitudeFromFact(void)
{
    _loiterCoordinate.setAltitude(_loiterAltitudeFact.rawValue().toDouble());
    emit loiterCoordinateChanged(_loiterCoordinate);
    emit coordinateChanged(_loiterCoordinate);
}

void FixedWingLandingComplexItem::_updateLandingCoodinateAltitudeFromFact(void)
{
    _landingCoordinate.setAltitude(_landingAltitudeFact.rawValue().toDouble());
    emit landingCoordinateChanged(_landingCoordinate);
}

void FixedWingLandingComplexItem::_setDirty(void)
{
    setDirty(true);
}

void FixedWingLandingComplexItem::applyNewAltitude(double newAltitude)
{
    _loiterAltitudeFact.setRawValue(newAltitude);
}

void FixedWingLandingComplexItem::_glideSlopeChanged(void)
{
    if (!_ignoreRecalcSignals) {
        double landingAltDifference = _loiterAltitudeFact.rawValue().toDouble() - _landingAltitudeFact.rawValue().toDouble();
        double glideSlope = _glideSlopeFact.rawValue().toDouble();
        _landingDistanceFact.setRawValue(landingAltDifference / qTan(qDegreesToRadians(glideSlope)));
    }
}

void FixedWingLandingComplexItem::_calcGlideSlope(void)
{
    double landingAltDifference = _loiterAltitudeFact.rawValue().toDouble() - _landingAltitudeFact.rawValue().toDouble();
    double landingDistance = _landingDistanceFact.rawValue().toDouble();

    _glideSlopeFact.setRawValue(qRadiansToDegrees(qAtan(landingAltDifference / landingDistance)));
}

void FixedWingLandingComplexItem::_signalLastSequenceNumberChanged(void)
{
    emit lastSequenceNumberChanged(lastSequenceNumber());
}

FixedWingLandingComplexItem::ReadyForSaveState FixedWingLandingComplexItem::readyForSaveState(void) const
{
    return _landingCoordSet && !_wizardMode ? ReadyForSave : NotReadyForSaveData;
}

void FixedWingLandingComplexItem::setLoiterDragAngleOnly(bool loiterDragAngleOnly)
{
    if (loiterDragAngleOnly != _loiterDragAngleOnly) {
        _loiterDragAngleOnly = loiterDragAngleOnly;
        emit loiterDragAngleOnlyChanged(_loiterDragAngleOnly);
    }
}
