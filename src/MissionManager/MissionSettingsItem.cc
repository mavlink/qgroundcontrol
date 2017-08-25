/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "MissionSettingsItem.h"
#include "JsonHelper.h"
#include "MissionController.h"
#include "QGCGeo.h"
#include "QGroundControlQmlGlobal.h"
#include "SimpleMissionItem.h"
#include "SettingsManager.h"
#include "AppSettings.h"
#include "MissionCommandUIInfo.h"

#include <QPolygonF>

QGC_LOGGING_CATEGORY(MissionSettingsComplexItemLog, "MissionSettingsComplexItemLog")

const char* MissionSettingsItem::jsonComplexItemTypeValue = "MissionSettings";

const char* MissionSettingsItem::_plannedHomePositionAltitudeName = "PlannedHomePositionAltitude";

QMap<QString, FactMetaData*> MissionSettingsItem::_metaDataMap;

MissionSettingsItem::MissionSettingsItem(Vehicle* vehicle, QObject* parent)
    : ComplexMissionItem                (vehicle, parent)
    , _plannedHomePositionAltitudeFact  (0, _plannedHomePositionAltitudeName,   FactMetaData::valueTypeDouble)
    , _plannedHomePositionFromVehicle   (false)
    , _missionEndRTL                    (false)
    , _cameraSection                    (vehicle)
    , _speedSection                     (vehicle)
    , _sequenceNumber                   (0)
    , _dirty                            (false)
{
    _editorQml = "qrc:/qml/MissionSettingsEditor.qml";

    if (_metaDataMap.isEmpty()) {
        _metaDataMap = FactMetaData::createMapFromJsonFile(QStringLiteral(":/json/MissionSettings.FactMetaData.json"), NULL /* metaDataParent */);
    }

    _plannedHomePositionAltitudeFact.setMetaData    (_metaDataMap[_plannedHomePositionAltitudeName]);
    _plannedHomePositionAltitudeFact.setRawValue    (_plannedHomePositionAltitudeFact.rawDefaultValue());
    setHomePositionSpecialCase(true);

    _cameraSection.setAvailable(true);
    _speedSection.setAvailable(true);

    connect(this,               &MissionSettingsItem::specifyMissionFlightSpeedChanged, this, &MissionSettingsItem::_setDirtyAndUpdateLastSequenceNumber);
    connect(this,               &MissionSettingsItem::missionEndRTLChanged,             this, &MissionSettingsItem::_setDirtyAndUpdateLastSequenceNumber);
    connect(&_cameraSection,    &CameraSection::itemCountChanged,                       this, &MissionSettingsItem::_setDirtyAndUpdateLastSequenceNumber);
    connect(&_speedSection,     &CameraSection::itemCountChanged,                       this, &MissionSettingsItem::_setDirtyAndUpdateLastSequenceNumber);

    connect(this,               &MissionSettingsItem::terrainAltitudeChanged,           this, &MissionSettingsItem::_setHomeAltFromTerrain);

    connect(&_plannedHomePositionAltitudeFact,  &Fact::valueChanged,                    this, &MissionSettingsItem::_setDirty);
    connect(&_plannedHomePositionAltitudeFact,  &Fact::valueChanged,                    this, &MissionSettingsItem::_updateAltitudeInCoordinate);

    connect(&_cameraSection,    &CameraSection::dirtyChanged,   this, &MissionSettingsItem::_sectionDirtyChanged);
    connect(&_speedSection,     &SpeedSection::dirtyChanged,    this, &MissionSettingsItem::_sectionDirtyChanged);

    connect(&_cameraSection,    &CameraSection::specifiedGimbalYawChanged,  this, &MissionSettingsItem::specifiedGimbalYawChanged);
    connect(&_speedSection,     &SpeedSection::specifiedFlightSpeedChanged, this, &MissionSettingsItem::specifiedFlightSpeedChanged);
}

int MissionSettingsItem::lastSequenceNumber(void) const
{
    int lastSequenceNumber = _sequenceNumber;

    lastSequenceNumber += _cameraSection.itemCount();
    lastSequenceNumber += _speedSection.itemCount();

    return lastSequenceNumber;
}

void MissionSettingsItem::setDirty(bool dirty)
{
    if (_dirty != dirty) {
        _dirty = dirty;
        if (!dirty) {
            _cameraSection.setDirty(false);
            _speedSection.setDirty(false);
        }
        emit dirtyChanged(_dirty);
    }
}

void MissionSettingsItem::save(QJsonArray&  missionItems)
{
    QList<MissionItem*> items;

    appendMissionItems(items, this);

    // First item show be planned home position, we are not responsible for save/load
    // Remaining items we just output as is
    for (int i=1; i<items.count(); i++) {
        MissionItem* item = items[i];
        QJsonObject saveObject;
        item->save(saveObject);
        missionItems.append(saveObject);
        item->deleteLater();
    }
}

void MissionSettingsItem::setSequenceNumber(int sequenceNumber)
{
    if (_sequenceNumber != sequenceNumber) {
        _sequenceNumber = sequenceNumber;
        emit sequenceNumberChanged(sequenceNumber);
        emit lastSequenceNumberChanged(lastSequenceNumber());
    }
}

bool MissionSettingsItem::load(const QJsonObject& complexObject, int sequenceNumber, QString& errorString)
{
    Q_UNUSED(complexObject);
    Q_UNUSED(sequenceNumber);
    Q_UNUSED(errorString);

    return true;
}

double MissionSettingsItem::greatestDistanceTo(const QGeoCoordinate &other) const
{
    Q_UNUSED(other);
    return 0;
}

bool MissionSettingsItem::specifiesCoordinate(void) const
{
    return true;
}

void MissionSettingsItem::appendMissionItems(QList<MissionItem*>& items, QObject* missionItemParent)
{
    int seqNum = _sequenceNumber;

    // IMPORTANT NOTE: If anything changes here you must also change MissionSettingsItem::scanForMissionSettings

    // Planned home position
    MissionItem* item = new MissionItem(seqNum++,
                                        MAV_CMD_NAV_WAYPOINT,
                                        MAV_FRAME_GLOBAL,
                                        0,                      // Hold time
                                        0,                      // Acceptance radius
                                        0,                      // Not sure?
                                        0,                      // Yaw
                                        coordinate().latitude(),
                                        coordinate().longitude(),
                                        _plannedHomePositionAltitudeFact.rawValue().toDouble(),
                                        true,                   // autoContinue
                                        false,                  // isCurrentItem
                                        missionItemParent);
    items.append(item);

    _cameraSection.appendSectionItems(items, missionItemParent, seqNum);
    _speedSection.appendSectionItems(items, missionItemParent, seqNum);
}

bool MissionSettingsItem::addMissionEndAction(QList<MissionItem*>& items, int seqNum, QObject* missionItemParent)
{
    MissionItem* item = NULL;

    // IMPORTANT NOTE: If anything changes here you must also change MissionSettingsItem::scanForMissionSettings

    if (_missionEndRTL) {
        qCDebug(MissionSettingsComplexItemLog) << "Appending end action RTL seqNum" << seqNum;
        item = new MissionItem(seqNum,
                               MAV_CMD_NAV_RETURN_TO_LAUNCH,
                               MAV_FRAME_MISSION,
                               0, 0, 0, 0, 0, 0, 0,        // param 1-7 not used
                               true,                       // autoContinue
                               false,                      // isCurrentItem
                               missionItemParent);
        items.append(item);
        return true;
    } else {
        return false;
    }
}

bool MissionSettingsItem::scanForMissionSettings(QmlObjectListModel* visualItems, int scanIndex)
{
    bool foundSpeedSection = false;
    bool foundCameraSection = false;

    qCDebug(MissionSettingsComplexItemLog) << "MissionSettingsItem::scanForMissionSettings count:scanIndex" << visualItems->count() << scanIndex;

    // Scan through the initial mission items for possible mission settings
    foundCameraSection = _cameraSection.scanForSection(visualItems, scanIndex);
    foundSpeedSection = _speedSection.scanForSection(visualItems, scanIndex);

    // Look at the end of the mission for end actions

    int lastIndex = visualItems->count() - 1;
    SimpleMissionItem* item = visualItems->value<SimpleMissionItem*>(lastIndex);
    if (item) {
        MissionItem& missionItem = item->missionItem();

        if (missionItem.command() == MAV_CMD_NAV_RETURN_TO_LAUNCH &&
                missionItem.param1() == 0 && missionItem.param2() == 0 && missionItem.param3() == 0 && missionItem.param4() == 0 && missionItem.param5() == 0 && missionItem.param6() == 0 && missionItem.param7() == 0) {
            qCDebug(MissionSettingsComplexItemLog) << "Scan: Found end action RTL";
            _missionEndRTL = true;
            visualItems->removeAt(lastIndex)->deleteLater();
        }
    }

    return foundSpeedSection || foundCameraSection;
}

double MissionSettingsItem::complexDistance(void) const
{
    return 0;
}

void MissionSettingsItem::_setDirty(void)
{
    setDirty(true);
}

void MissionSettingsItem::setHomePositionFromVehicle(const QGeoCoordinate& coordinate)
{
    _plannedHomePositionFromVehicle = true;
    setCoordinate(coordinate);
}

void MissionSettingsItem::setCoordinate(const QGeoCoordinate& coordinate)
{
    if (_plannedHomePositionCoordinate != coordinate) {
        // ArduPilot tends to send crap home positions at initial vehicel boot, discard them
        if (coordinate.isValid() && (coordinate.latitude() != 0 || coordinate.longitude() != 0)) {
            qDebug() << "Setting home position" << coordinate;
            _plannedHomePositionCoordinate = coordinate;
            emit coordinateChanged(coordinate);
            emit exitCoordinateChanged(coordinate);
            _plannedHomePositionAltitudeFact.setRawValue(coordinate.altitude());
        }
    }
}

void MissionSettingsItem::_setDirtyAndUpdateLastSequenceNumber(void)
{
    emit lastSequenceNumberChanged(lastSequenceNumber());
    setDirty(true);
}

void MissionSettingsItem::_sectionDirtyChanged(bool dirty)
{
    if (dirty) {
        setDirty(true);
    }
}

double MissionSettingsItem::specifiedGimbalYaw(void)
{
    return _cameraSection.specifyGimbal() ? _cameraSection.gimbalYaw()->rawValue().toDouble() : std::numeric_limits<double>::quiet_NaN();
}

void MissionSettingsItem::_updateAltitudeInCoordinate(QVariant value)
{
    double newAltitude = value.toDouble();

    if (!qFuzzyCompare(_plannedHomePositionCoordinate.altitude(), newAltitude)) {

        _plannedHomePositionCoordinate.setAltitude(newAltitude);
        emit coordinateChanged(_plannedHomePositionCoordinate);
        emit exitCoordinateChanged(_plannedHomePositionCoordinate);
    }
}

double MissionSettingsItem::specifiedFlightSpeed(void)
{
    if (_speedSection.specifyFlightSpeed()) {
        return _speedSection.flightSpeed()->rawValue().toDouble();
    } else {
        return std::numeric_limits<double>::quiet_NaN();
    }
}

void MissionSettingsItem::setMissionEndRTL(bool missionEndRTL)
{
    if (missionEndRTL != _missionEndRTL) {
        _missionEndRTL = missionEndRTL;
        emit missionEndRTLChanged(missionEndRTL);
    }
}

void MissionSettingsItem::_setHomeAltFromTerrain(double terrainAltitude)
{
    if (!_plannedHomePositionFromVehicle) {
        _plannedHomePositionAltitudeFact.setRawValue(terrainAltitude);
    }
}
