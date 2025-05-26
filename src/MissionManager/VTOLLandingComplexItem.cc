/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "VTOLLandingComplexItem.h"
#include "JsonHelper.h"
#include "MissionController.h"
#include "PlanMasterController.h"
#include "FlightPathSegment.h"
#include "MissionItem.h"
#include "SettingsManager.h"
#include "PlanViewSettings.h"
#include "QGC.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QJsonArray>

QGC_LOGGING_CATEGORY(VTOLLandingComplexItemLog, "VTOLLandingComplexItemLog")

const QString VTOLLandingComplexItem::name(VTOLLandingComplexItem::tr("VTOL Landing"));

VTOLLandingComplexItem::VTOLLandingComplexItem(PlanMasterController* masterController, bool flyView)
    : LandingComplexItem        (masterController, flyView)
    , _metaDataMap              (FactMetaData::createMapFromJsonFile(QStringLiteral(":/json/VTOLLandingPattern.FactMetaData.json"), this))
    , _landingDistanceFact      (settingsGroup, _metaDataMap[finalApproachToLandDistanceName])
    , _finalApproachAltitudeFact(settingsGroup, _metaDataMap[finalApproachAltitudeName])
    , _useDoChangeSpeedFact     (settingsGroup, _metaDataMap[useDoChangeSpeedName])
    , _finalApproachSpeedFact   (settingsGroup, _metaDataMap[finalApproachSpeedName])
    , _loiterRadiusFact         (settingsGroup, _metaDataMap[loiterRadiusName])
    , _loiterClockwiseFact      (settingsGroup, _metaDataMap[loiterClockwiseName])
    , _landingHeadingFact       (settingsGroup, _metaDataMap[landingHeadingName])
    , _landingAltitudeFact      (settingsGroup, _metaDataMap[landingAltitudeName])
    , _useLoiterToAltFact       (settingsGroup, _metaDataMap[useLoiterToAltName])
    , _stopTakingPhotosFact     (settingsGroup, _metaDataMap[stopTakingPhotosName])
    , _stopTakingVideoFact      (settingsGroup, _metaDataMap[stopTakingVideoName])
{
    _editorQml      = "qrc:/qml/VTOLLandingPatternEditor.qml";
    _isIncomplete   = false;

    _init();

    // We adjust landing distance meta data to Plan View settings unless there was a custom build override
    if (QGC::fuzzyCompare(_landingDistanceFact.rawValue().toDouble(), _landingDistanceFact.rawDefaultValue().toDouble())) {
        Fact* vtolTransitionDistanceFact = SettingsManager::instance()->planViewSettings()->vtolTransitionDistance();
        double vtolTransitionDistance = vtolTransitionDistanceFact->rawValue().toDouble();
        _landingDistanceFact.metaData()->setRawDefaultValue(vtolTransitionDistance);
        _landingDistanceFact.setRawValue(vtolTransitionDistance);
        _landingDistanceFact.metaData()->setRawMin(vtolTransitionDistanceFact->metaData()->rawMin());
    }

    _recalcFromHeadingAndDistanceChange();

    setDirty(false);
}

QString VTOLLandingComplexItem::patternName() const {
    if (_masterController->missionController()->isFirstLandingComplexItem(this)) {
        return name;
    } else {
        return "Alternate Landing";
    }
}


void VTOLLandingComplexItem::save(QJsonArray&  missionItems)
{
    QJsonObject saveObject = _save();

    saveObject[JsonHelper::jsonVersionKey] =                    1;
    saveObject[VisualMissionItem::jsonTypeKey] =                VisualMissionItem::jsonTypeComplexItemValue;
    saveObject[ComplexMissionItem::jsonComplexItemTypeKey] =    jsonComplexItemTypeValue;

    missionItems.append(saveObject);
}

bool VTOLLandingComplexItem::load(const QJsonObject& complexObject, int sequenceNumber, QString& errorString)
{
    QList<JsonHelper::KeyValidateInfo> keyInfoList = {
        { JsonHelper::jsonVersionKey, QJsonValue::Double, true },
    };
    if (!JsonHelper::validateKeys(complexObject, keyInfoList, errorString)) {
        return false;
    }

    int version = complexObject[JsonHelper::jsonVersionKey].toInt();
    if (version != 1) {
        errorString = tr("%1 complex item version %2 not supported").arg(jsonComplexItemTypeValue).arg(version);
        _ignoreRecalcSignals = false;
        return false;
    }

    return _load(complexObject, sequenceNumber, jsonComplexItemTypeValue, false /* useDeprecatedRelAltKeys */, errorString);
}

MissionItem* VTOLLandingComplexItem::_createLandItem(int seqNum, bool altRel, double lat, double lon, double alt, QObject* parent)
{
    return new MissionItem(seqNum,
                           MAV_CMD_NAV_VTOL_LAND,
                           altRel ? MAV_FRAME_GLOBAL_RELATIVE_ALT : MAV_FRAME_GLOBAL,
                           0.0, 0.0, 0.0,
                           qQNaN(),         // Yaw - not specified
                           lat, lon, alt,
                           true,            // autoContinue
                           false,           // isCurrentItem
                           parent);

}

void VTOLLandingComplexItem::_calcGlideSlope(void)
{
    // No glide slope calc for VTOL
}

bool VTOLLandingComplexItem::_isValidLandItem(const MissionItem& missionItem)
{
    if (missionItem.command() != MAV_CMD_NAV_VTOL_LAND ||
            !(missionItem.frame() == MAV_FRAME_GLOBAL_RELATIVE_ALT || missionItem.frame() == MAV_FRAME_GLOBAL) ||
            missionItem.param1() != 0 || missionItem.param2() != 0 || missionItem.param3() != 0 || !qIsNaN(missionItem.param4())) {
        return false;
    } else {
        return true;
    }
}

bool VTOLLandingComplexItem::scanForItems(QmlObjectListModel* visualItems, bool flyView, PlanMasterController* masterController)
{
    return _scanForItems(visualItems, flyView, masterController, _isValidLandItem, _createItem);
}

// Never call this method directly. If you want to update the flight segments you emit _updateFlightPathSegmentsSignal()
void VTOLLandingComplexItem::_updateFlightPathSegmentsDontCallDirectly(void)
{
    if (_cTerrainCollisionSegments != 0) {
        _cTerrainCollisionSegments = 0;
        emit terrainCollisionChanged(false);
    }

    _flightPathSegments.beginReset();
    _flightPathSegments.clearAndDeleteContents();
    if (useLoiterToAlt()->rawValue().toBool()) {
        _appendFlightPathSegment(FlightPathSegment::SegmentTypeGeneric, finalApproachCoordinate(), amslEntryAlt(), loiterTangentCoordinate(),  amslEntryAlt()); // Best we can do to simulate loiter circle terrain profile
        _appendFlightPathSegment(FlightPathSegment::SegmentTypeGeneric, loiterTangentCoordinate(), amslEntryAlt(), landingCoordinate(),        amslEntryAlt());
    } else {
        _appendFlightPathSegment(FlightPathSegment::SegmentTypeGeneric, finalApproachCoordinate(), amslEntryAlt(), landingCoordinate(),        amslEntryAlt());
    }
    _appendFlightPathSegment(FlightPathSegment::SegmentTypeLand, landingCoordinate(), amslEntryAlt(), landingCoordinate(), amslExitAlt());
    _flightPathSegments.endReset();

    if (_cTerrainCollisionSegments != 0) {
        emit terrainCollisionChanged(true);
    }

    _masterController->missionController()->recalcTerrainProfile();
}
