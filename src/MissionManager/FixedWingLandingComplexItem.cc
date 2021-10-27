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
#include "SimpleMissionItem.h"
#include "PlanMasterController.h"
#include "FlightPathSegment.h"

#include <QPolygonF>

QGC_LOGGING_CATEGORY(FixedWingLandingComplexItemLog, "FixedWingLandingComplexItemLog")

const QString FixedWingLandingComplexItem::name(FixedWingLandingComplexItem::tr("Fixed Wing Landing"));

const char* FixedWingLandingComplexItem::settingsGroup                      = "FixedWingLanding";
const char* FixedWingLandingComplexItem::jsonComplexItemTypeValue           = "fwLandingPattern";

const char* FixedWingLandingComplexItem::glideSlopeName                     = "GlideSlope";
const char* FixedWingLandingComplexItem::valueSetIsDistanceName             = "ValueSetIsDistance";

const char* FixedWingLandingComplexItem::_jsonValueSetIsDistanceKey         = "valueSetIsDistance";

FixedWingLandingComplexItem::FixedWingLandingComplexItem(PlanMasterController* masterController, bool flyView)
    : LandingComplexItem        (masterController, flyView)
    , _metaDataMap              (FactMetaData::createMapFromJsonFile(QStringLiteral(":/json/FWLandingPattern.FactMetaData.json"), this))
    , _landingDistanceFact      (settingsGroup, _metaDataMap[finalApproachToLandDistanceName])
    , _finalApproachAltitudeFact(settingsGroup, _metaDataMap[finalApproachAltitudeName])
    , _loiterRadiusFact         (settingsGroup, _metaDataMap[loiterRadiusName])
    , _loiterClockwiseFact      (settingsGroup, _metaDataMap[loiterClockwiseName])
    , _landingHeadingFact       (settingsGroup, _metaDataMap[landingHeadingName])
    , _landingAltitudeFact      (settingsGroup, _metaDataMap[landingAltitudeName])
    , _glideSlopeFact           (settingsGroup, _metaDataMap[glideSlopeName])
    , _useLoiterToAltFact       (settingsGroup, _metaDataMap[useLoiterToAltName])
    , _stopTakingPhotosFact     (settingsGroup, _metaDataMap[stopTakingPhotosName])
    , _stopTakingVideoFact      (settingsGroup, _metaDataMap[stopTakingVideoName])
    , _valueSetIsDistanceFact   (settingsGroup, _metaDataMap[valueSetIsDistanceName])
{
    _editorQml      = "qrc:/qml/FWLandingPatternEditor.qml";
    _isIncomplete   = false;

    _init();

    connect(&_glideSlopeFact,           &Fact::valueChanged, this, &FixedWingLandingComplexItem::_glideSlopeChanged);
    connect(&_valueSetIsDistanceFact,   &Fact::valueChanged, this, &FixedWingLandingComplexItem::_setDirty);

    if (_valueSetIsDistanceFact.rawValue().toBool()) {
        _recalcFromHeadingAndDistanceChange();
    } else {
        _glideSlopeChanged();
    }
    setDirty(false);
}

void FixedWingLandingComplexItem::save(QJsonArray&  missionItems)
{
    QJsonObject saveObject = _save();

    saveObject[JsonHelper::jsonVersionKey]                  = 2;
    saveObject[VisualMissionItem::jsonTypeKey]              = VisualMissionItem::jsonTypeComplexItemValue;
    saveObject[ComplexMissionItem::jsonComplexItemTypeKey]  = jsonComplexItemTypeValue;
    saveObject[_jsonValueSetIsDistanceKey]                  = _valueSetIsDistanceFact.rawValue().toBool();

    missionItems.append(saveObject);
}

bool FixedWingLandingComplexItem::load(const QJsonObject& complexObject, int sequenceNumber, QString& errorString)
{
    QList<JsonHelper::KeyValidateInfo> keyInfoList = {
        { JsonHelper::jsonVersionKey, QJsonValue::Double, true },
    };
    if (!JsonHelper::validateKeys(complexObject, keyInfoList, errorString)) {
        return false;
    }

    int version = complexObject[JsonHelper::jsonVersionKey].toInt();
    if (version == 1) {
        _valueSetIsDistanceFact.setRawValue(true);
    } else if (version == 2) {
        QList<JsonHelper::KeyValidateInfo> v2KeyInfoList = {
            { _jsonValueSetIsDistanceKey,   QJsonValue::Bool,  true },
        };
        if (!JsonHelper::validateKeys(complexObject, v2KeyInfoList, errorString)) {
            _ignoreRecalcSignals = false;
            return false;
        }

        _valueSetIsDistanceFact.setRawValue(complexObject[_jsonValueSetIsDistanceKey].toBool());
    } else {
        errorString = tr("%1 complex item version %2 not supported").arg(jsonComplexItemTypeValue).arg(version);
        _ignoreRecalcSignals = false;
        return false;
    }

    return _load(complexObject, sequenceNumber, jsonComplexItemTypeValue, version == 1 /* useDeprecatedRelAltKeys */, errorString);
}

MissionItem* FixedWingLandingComplexItem::_createLandItem(int seqNum, bool altRel, double lat, double lon, double alt, QObject* parent)
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

void FixedWingLandingComplexItem::_glideSlopeChanged(void)
{
    if (!_ignoreRecalcSignals) {
        double landingAltDifference = _finalApproachAltitudeFact.rawValue().toDouble() - _landingAltitudeFact.rawValue().toDouble();
        double glideSlope = _glideSlopeFact.rawValue().toDouble();
        _landingDistanceFact.setRawValue(landingAltDifference / qTan(qDegreesToRadians(glideSlope)));
    }
}

void FixedWingLandingComplexItem::_calcGlideSlope(void)
{
    double landingAltDifference = _finalApproachAltitudeFact.rawValue().toDouble() - _landingAltitudeFact.rawValue().toDouble();
    double landingDistance = _landingDistanceFact.rawValue().toDouble();

    _glideSlopeFact.setRawValue(qRadiansToDegrees(qAtan(landingAltDifference / landingDistance)));
}

void FixedWingLandingComplexItem::moveLandingPosition(const QGeoCoordinate& coordinate)
{
    double savedHeading = landingHeading()->rawValue().toDouble();
    double savedDistance = landingDistance()->rawValue().toDouble();

    setLandingCoordinate(coordinate);
    landingHeading()->setRawValue(savedHeading);
    landingDistance()->setRawValue(savedDistance);
}

bool FixedWingLandingComplexItem::_isValidLandItem(const MissionItem& missionItem)
{
    if (missionItem.command() != MAV_CMD_NAV_LAND ||
            !(missionItem.frame() == MAV_FRAME_GLOBAL_RELATIVE_ALT || missionItem.frame() == MAV_FRAME_GLOBAL) ||
            missionItem.param1() != 0 || missionItem.param2() != 0 || missionItem.param3() != 0 || missionItem.param4() != 0) {
        return false;
    } else {
        return true;
    }
}

bool FixedWingLandingComplexItem::scanForItem(QmlObjectListModel* visualItems, bool flyView, PlanMasterController* masterController)
{
    return _scanForItem(visualItems, flyView, masterController, _isValidLandItem, _createItem);
}

// Never call this method directly. If you want to update the flight segments you emit _updateFlightPathSegmentsSignal()
void FixedWingLandingComplexItem::_updateFlightPathSegmentsDontCallDirectly(void)
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
