/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "VTOLLandingComplexItem.h"
#include "JsonHelper.h"
#include "MissionController.h"
#include "QGCGeo.h"
#include "SimpleMissionItem.h"
#include "PlanMasterController.h"
#include "FlightPathSegment.h"

#include <QPolygonF>

QGC_LOGGING_CATEGORY(VTOLLandingComplexItemLog, "VTOLLandingComplexItemLog")

const QString VTOLLandingComplexItem::name(tr("VTOL Landing"));

const char* VTOLLandingComplexItem::settingsGroup =            "VTOLLanding";
const char* VTOLLandingComplexItem::jsonComplexItemTypeValue = "vtolLandingPattern";

const char* VTOLLandingComplexItem::loiterToLandDistanceName = "LandingDistance";
const char* VTOLLandingComplexItem::landingHeadingName =       "LandingHeading";
const char* VTOLLandingComplexItem::loiterAltitudeName =       "LoiterAltitude";
const char* VTOLLandingComplexItem::loiterRadiusName =         "LoiterRadius";
const char* VTOLLandingComplexItem::landingAltitudeName =      "LandingAltitude";
const char* VTOLLandingComplexItem::stopTakingPhotosName =     "StopTakingPhotos";
const char* VTOLLandingComplexItem::stopTakingVideoName =      "StopTakingVideo";

const char* VTOLLandingComplexItem::_jsonLoiterCoordinateKey =         "loiterCoordinate";
const char* VTOLLandingComplexItem::_jsonLoiterRadiusKey =             "loiterRadius";
const char* VTOLLandingComplexItem::_jsonLoiterClockwiseKey =          "loiterClockwise";
const char* VTOLLandingComplexItem::_jsonLandingCoordinateKey =        "landCoordinate";
const char* VTOLLandingComplexItem::_jsonAltitudesAreRelativeKey =     "altitudesAreRelative";
const char* VTOLLandingComplexItem::_jsonStopTakingPhotosKey =         "stopTakingPhotos";
const char* VTOLLandingComplexItem::_jsonStopTakingVideoKey =          "stopVideoPhotos";

VTOLLandingComplexItem::VTOLLandingComplexItem(PlanMasterController* masterController, bool flyView, QObject* parent)
    : LandingComplexItem        (masterController, flyView, parent)
    , _metaDataMap              (FactMetaData::createMapFromJsonFile(QStringLiteral(":/json/VTOLLandingPattern.FactMetaData.json"), this))
    , _landingDistanceFact      (settingsGroup, _metaDataMap[loiterToLandDistanceName])
    , _loiterAltitudeFact       (settingsGroup, _metaDataMap[loiterAltitudeName])
    , _loiterRadiusFact         (settingsGroup, _metaDataMap[loiterRadiusName])
    , _landingHeadingFact       (settingsGroup, _metaDataMap[landingHeadingName])
    , _landingAltitudeFact      (settingsGroup, _metaDataMap[landingAltitudeName])
    , _stopTakingPhotosFact     (settingsGroup, _metaDataMap[stopTakingPhotosName])
    , _stopTakingVideoFact      (settingsGroup, _metaDataMap[stopTakingVideoName])
{
    _editorQml = "qrc:/qml/VTOLLandingPatternEditor.qml";
    _isIncomplete = false;

    QGeoCoordinate homePositionCoordinate = masterController->missionController()->plannedHomePosition();
    if (homePositionCoordinate.isValid()) {
        setLandingCoordinate(homePositionCoordinate);
    }

    connect(&_loiterAltitudeFact,       &Fact::valueChanged,                                    this, &VTOLLandingComplexItem::_updateLoiterCoodinateAltitudeFromFact);
    connect(&_landingAltitudeFact,      &Fact::valueChanged,                                    this, &VTOLLandingComplexItem::_updateLandingCoodinateAltitudeFromFact);

    connect(&_landingDistanceFact,      &Fact::valueChanged,                                    this, &VTOLLandingComplexItem::_recalcFromHeadingAndDistanceChange);
    connect(&_landingHeadingFact,       &Fact::valueChanged,                                    this, &VTOLLandingComplexItem::_recalcFromHeadingAndDistanceChange);

    connect(&_loiterRadiusFact,         &Fact::valueChanged,                                    this, &VTOLLandingComplexItem::_recalcFromRadiusChange);
    connect(this,                       &VTOLLandingComplexItem::loiterClockwiseChanged,        this, &VTOLLandingComplexItem::_recalcFromRadiusChange);

    connect(this,                       &VTOLLandingComplexItem::loiterCoordinateChanged,       this, &VTOLLandingComplexItem::_recalcFromCoordinateChange);
    connect(this,                       &VTOLLandingComplexItem::landingCoordinateChanged,      this, &VTOLLandingComplexItem::_recalcFromCoordinateChange);

    connect(this,                       &VTOLLandingComplexItem::altitudesAreRelativeChanged,   this, &VTOLLandingComplexItem::_amslEntryAltChanged);
    connect(this,                       &VTOLLandingComplexItem::altitudesAreRelativeChanged,   this, &VTOLLandingComplexItem::_amslExitAltChanged);
    connect(_missionController,         &MissionController::plannedHomePositionChanged,         this, &VTOLLandingComplexItem::_amslEntryAltChanged);
    connect(_missionController,         &MissionController::plannedHomePositionChanged,         this, &VTOLLandingComplexItem::_amslExitAltChanged);
    connect(&_loiterAltitudeFact,       &Fact::valueChanged,                                    this, &VTOLLandingComplexItem::_amslEntryAltChanged);
    connect(&_landingAltitudeFact,      &Fact::valueChanged,                                    this, &VTOLLandingComplexItem::_amslExitAltChanged);
    connect(this,                       &VTOLLandingComplexItem::amslEntryAltChanged,           this, &VTOLLandingComplexItem::maxAMSLAltitudeChanged);
    connect(this,                       &VTOLLandingComplexItem::amslExitAltChanged,            this, &VTOLLandingComplexItem::minAMSLAltitudeChanged);

    connect(&_stopTakingPhotosFact,     &Fact::valueChanged,                                    this, &VTOLLandingComplexItem::_signalLastSequenceNumberChanged);
    connect(&_stopTakingVideoFact,      &Fact::valueChanged,                                    this, &VTOLLandingComplexItem::_signalLastSequenceNumberChanged);

    connect(&_loiterAltitudeFact,       &Fact::valueChanged,                                    this, &VTOLLandingComplexItem::_setDirty);
    connect(&_landingAltitudeFact,      &Fact::valueChanged,                                    this, &VTOLLandingComplexItem::_setDirty);
    connect(&_landingDistanceFact,      &Fact::valueChanged,                                    this, &VTOLLandingComplexItem::_setDirty);
    connect(&_landingHeadingFact,       &Fact::valueChanged,                                    this, &VTOLLandingComplexItem::_setDirty);
    connect(&_loiterRadiusFact,         &Fact::valueChanged,                                    this, &VTOLLandingComplexItem::_setDirty);
    connect(&_stopTakingPhotosFact,     &Fact::valueChanged,                                    this, &VTOLLandingComplexItem::_setDirty);
    connect(&_stopTakingVideoFact,      &Fact::valueChanged,                                    this, &VTOLLandingComplexItem::_setDirty);
    connect(this,                       &VTOLLandingComplexItem::loiterCoordinateChanged,       this, &VTOLLandingComplexItem::_setDirty);
    connect(this,                       &VTOLLandingComplexItem::landingCoordinateChanged,      this, &VTOLLandingComplexItem::_setDirty);
    connect(this,                       &VTOLLandingComplexItem::loiterClockwiseChanged,        this, &VTOLLandingComplexItem::_setDirty);
    connect(this,                       &VTOLLandingComplexItem::altitudesAreRelativeChanged,   this, &VTOLLandingComplexItem::_setDirty);

    connect(this,                       &VTOLLandingComplexItem::landingCoordSetChanged,        this, &VTOLLandingComplexItem::readyForSaveStateChanged);
    connect(this,                       &VTOLLandingComplexItem::wizardModeChanged,             this, &VTOLLandingComplexItem::readyForSaveStateChanged);

    connect(this,                       &VTOLLandingComplexItem::loiterCoordinateChanged,       this, &VTOLLandingComplexItem::complexDistanceChanged);
    connect(this,                       &VTOLLandingComplexItem::loiterTangentCoordinateChanged,this, &VTOLLandingComplexItem::complexDistanceChanged);
    connect(this,                       &VTOLLandingComplexItem::landingCoordinateChanged,      this, &VTOLLandingComplexItem::complexDistanceChanged);

    connect(this,                       &VTOLLandingComplexItem::loiterTangentCoordinateChanged,this, &VTOLLandingComplexItem::_updateFlightPathSegmentsSignal);
    connect(this,                       &VTOLLandingComplexItem::landingCoordinateChanged,      this, &VTOLLandingComplexItem::_updateFlightPathSegmentsSignal);
    connect(&_loiterAltitudeFact,       &Fact::valueChanged,                                    this, &VTOLLandingComplexItem::_updateFlightPathSegmentsSignal);
    connect(&_landingAltitudeFact,      &Fact::valueChanged,                                    this, &VTOLLandingComplexItem::_updateFlightPathSegmentsSignal);
    connect(this,                       &VTOLLandingComplexItem::altitudesAreRelativeChanged,   this, &VTOLLandingComplexItem::_updateFlightPathSegmentsSignal);
    connect(_missionController,         &MissionController::plannedHomePositionChanged,         this, &VTOLLandingComplexItem::_updateFlightPathSegmentsSignal);

    // The follow is used to compress multiple recalc calls in a row to into a single call.
    connect(this, &VTOLLandingComplexItem::_updateFlightPathSegmentsSignal, this, &VTOLLandingComplexItem::_updateFlightPathSegmentsDontCallDirectly,   Qt::QueuedConnection);
    qgcApp()->addCompressedSignal(QMetaMethod::fromSignal(&VTOLLandingComplexItem::_updateFlightPathSegmentsSignal));

    if (_masterController->controllerVehicle()->apmFirmware()) {
        // ArduPilot does not support camera commands
        _stopTakingVideoFact.setRawValue(false);
        _stopTakingPhotosFact.setRawValue(false);
    }

    _recalcFromHeadingAndDistanceChange();

    setDirty(false);
}

int VTOLLandingComplexItem::lastSequenceNumber(void) const
{
    // Fixed items are:
    //  land start, loiter, land
    // Optional items are:
    //  stop photos/video
    return _sequenceNumber + 2 + (_stopTakingPhotosFact.rawValue().toBool() ? 2 : 0) + (_stopTakingVideoFact.rawValue().toBool() ? 1 : 0);
}

void VTOLLandingComplexItem::setDirty(bool dirty)
{
    if (_dirty != dirty) {
        _dirty = dirty;
        emit dirtyChanged(_dirty);
    }
}

void VTOLLandingComplexItem::save(QJsonArray&  missionItems)
{
    QJsonObject saveObject;

    saveObject[JsonHelper::jsonVersionKey] =                    1;
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

    missionItems.append(saveObject);
}

void VTOLLandingComplexItem::setSequenceNumber(int sequenceNumber)
{
    if (_sequenceNumber != sequenceNumber) {
        _sequenceNumber = sequenceNumber;
        emit sequenceNumberChanged(sequenceNumber);
        emit lastSequenceNumberChanged(lastSequenceNumber());
    }
}

bool VTOLLandingComplexItem::load(const QJsonObject& complexObject, int sequenceNumber, QString& errorString)
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
        QList<JsonHelper::KeyValidateInfo> v2KeyInfoList = {
            { _jsonAltitudesAreRelativeKey, QJsonValue::Bool,  true },
        };
        if (!JsonHelper::validateKeys(complexObject, v2KeyInfoList, errorString)) {
            _ignoreRecalcSignals = false;
            return false;
        }

        _altitudesAreRelative = complexObject[_jsonAltitudesAreRelativeKey].toBool();
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

    _landingCoordSet = true;

    _ignoreRecalcSignals = false;

    _recalcFromCoordinateChange();
    emit coordinateChanged(this->coordinate());    // This will kick off terrain query

    return true;
}

double VTOLLandingComplexItem::greatestDistanceTo(const QGeoCoordinate &other) const
{
    return qMax(_loiterCoordinate.distanceTo(other),_landingCoordinate.distanceTo(other));
}

bool VTOLLandingComplexItem::specifiesCoordinate(void) const
{
    return true;
}

MissionItem* VTOLLandingComplexItem::createDoLandStartItem(int seqNum, QObject* parent)
{
    return new MissionItem(seqNum,                              // sequence number
                           MAV_CMD_DO_LAND_START,               // MAV_CMD
                           MAV_FRAME_MISSION,                   // MAV_FRAME
                           0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,   // param 1-7
                           true,                                // autoContinue
                           false,                               // isCurrentItem
                           parent);
}

MissionItem* VTOLLandingComplexItem::createLoiterToAltItem(int seqNum, bool altRel, double loiterRadius, double lat, double lon, double alt, QObject* parent)
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

MissionItem* VTOLLandingComplexItem::createLandItem(int seqNum, bool altRel, double lat, double lon, double alt, QObject* parent)
{
    return new MissionItem(seqNum,
                           MAV_CMD_NAV_VTOL_LAND,
                           altRel ? MAV_FRAME_GLOBAL_RELATIVE_ALT : MAV_FRAME_GLOBAL,
                           0.0, 0.0, 0.0, 0.0,
                           lat, lon, alt,
                           true,                               // autoContinue
                           false,                              // isCurrentItem
                           parent);

}

void VTOLLandingComplexItem::appendMissionItems(QList<MissionItem*>& items, QObject* missionItemParent)
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

bool VTOLLandingComplexItem::scanForItem(QmlObjectListModel* visualItems, bool flyView, PlanMasterController* masterController)
{
    qCDebug(VTOLLandingComplexItemLog) << "VTOLLandingComplexItem::scanForItem count" << visualItems->count();

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

    VTOLLandingComplexItem* complexItem = new VTOLLandingComplexItem(masterController, flyView, visualItems);

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

    complexItem->_landingCoordSet = true;

    complexItem->_ignoreRecalcSignals = false;
    complexItem->_recalcFromCoordinateChange();
    complexItem->setDirty(false);

    visualItems->append(complexItem);

    return true;
}

double VTOLLandingComplexItem::_mathematicAngleToHeading(double angle)
{
    double heading = (angle - 90) * -1;
    if (heading < 0) {
        heading += 360;
    }

    return heading;
}

double VTOLLandingComplexItem::_headingToMathematicAngle(double heading)
{
    return heading - 90 * -1;
}

void VTOLLandingComplexItem::_recalcFromRadiusChange(void)
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

void VTOLLandingComplexItem::_recalcFromHeadingAndDistanceChange(void)
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
        _ignoreRecalcSignals = false;
    }
}

void VTOLLandingComplexItem::_recalcFromCoordinateChange(void)
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
        _ignoreRecalcSignals = false;
    }
}

void VTOLLandingComplexItem::_updateLoiterCoodinateAltitudeFromFact(void)
{
    _loiterCoordinate.setAltitude(_loiterAltitudeFact.rawValue().toDouble());
    emit loiterCoordinateChanged(_loiterCoordinate);
    emit coordinateChanged(_loiterCoordinate);
}

void VTOLLandingComplexItem::_updateLandingCoodinateAltitudeFromFact(void)
{
    _landingCoordinate.setAltitude(_landingAltitudeFact.rawValue().toDouble());
    emit landingCoordinateChanged(_landingCoordinate);
}

void VTOLLandingComplexItem::_setDirty(void)
{
    setDirty(true);
}

void VTOLLandingComplexItem::applyNewAltitude(double newAltitude)
{
    _loiterAltitudeFact.setRawValue(newAltitude);
}

void VTOLLandingComplexItem::_signalLastSequenceNumberChanged(void)
{
    emit lastSequenceNumberChanged(lastSequenceNumber());
}

VTOLLandingComplexItem::ReadyForSaveState VTOLLandingComplexItem::readyForSaveState(void) const
{
    return _landingCoordSet && !_wizardMode ? ReadyForSave : NotReadyForSaveData;
}


double VTOLLandingComplexItem::amslEntryAlt(void) const
{
    return _loiterAltitudeFact.rawValue().toDouble() + (_altitudesAreRelative ? _missionController->plannedHomePosition().altitude() : 0);
}

double VTOLLandingComplexItem::amslExitAlt(void) const
{
    return _landingAltitudeFact.rawValue().toDouble() + (_altitudesAreRelative ? _missionController->plannedHomePosition().altitude() : 0);

}
