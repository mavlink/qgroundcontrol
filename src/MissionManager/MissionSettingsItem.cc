/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "MissionSettingsItem.h"
#include "JsonHelper.h"
#include "MissionController.h"
#include "QGCGeo.h"
#include "SimpleMissionItem.h"
#include "SettingsManager.h"
#include "AppSettings.h"
#include "MissionCommandUIInfo.h"
#include "PlanMasterController.h"

#include <QPolygonF>

QGC_LOGGING_CATEGORY(MissionSettingsItemLog, "MissionSettingsItemLog")

const char* MissionSettingsItem::_plannedHomePositionAltitudeName = "PlannedHomePositionAltitude";

QMap<QString, FactMetaData*> MissionSettingsItem::_metaDataMap;

MissionSettingsItem::MissionSettingsItem(PlanMasterController* masterController, bool flyView)
    : ComplexMissionItem                (masterController, flyView)
    , _managerVehicle                   (masterController->managerVehicle())
    , _plannedHomePositionAltitudeFact  (0, _plannedHomePositionAltitudeName,   FactMetaData::valueTypeDouble)
    , _cameraSection                    (masterController)
    , _speedSection                     (masterController)
{
    _isIncomplete = false;
    _editorQml = "qrc:/qml/MissionSettingsEditor.qml";

    if (_metaDataMap.isEmpty()) {
        _metaDataMap = FactMetaData::createMapFromJsonFile(QStringLiteral(":/json/MissionSettings.FactMetaData.json"), nullptr /* metaDataParent */);
    }

    _plannedHomePositionAltitudeFact.setMetaData    (_metaDataMap[_plannedHomePositionAltitudeName]);
    _plannedHomePositionAltitudeFact.setRawValue    (_plannedHomePositionAltitudeFact.rawDefaultValue());
    setHomePositionSpecialCase(true);

    _cameraSection.setAvailable(true);
    _speedSection.setAvailable(true);

    connect(this,               &MissionSettingsItem::specifyMissionFlightSpeedChanged, this, &MissionSettingsItem::_setDirtyAndUpdateLastSequenceNumber);
    connect(&_cameraSection,    &CameraSection::itemCountChanged,                       this, &MissionSettingsItem::_setDirtyAndUpdateLastSequenceNumber);
    connect(&_speedSection,     &CameraSection::itemCountChanged,                       this, &MissionSettingsItem::_setDirtyAndUpdateLastSequenceNumber);
    connect(this,               &MissionSettingsItem::terrainAltitudeChanged,           this, &MissionSettingsItem::_setHomeAltFromTerrain);
    connect(&_cameraSection,    &CameraSection::dirtyChanged,                           this, &MissionSettingsItem::_sectionDirtyChanged);
    connect(&_speedSection,     &SpeedSection::dirtyChanged,                            this, &MissionSettingsItem::_sectionDirtyChanged);
    connect(&_cameraSection,    &CameraSection::specifiedGimbalYawChanged,              this, &MissionSettingsItem::specifiedGimbalYawChanged);
    connect(&_cameraSection,    &CameraSection::specifiedGimbalPitchChanged,            this, &MissionSettingsItem::specifiedGimbalPitchChanged);
    connect(&_speedSection,     &SpeedSection::specifiedFlightSpeedChanged,             this, &MissionSettingsItem::specifiedFlightSpeedChanged);
    connect(this,               &MissionSettingsItem::coordinateChanged,                this, &MissionSettingsItem::_amslEntryAltChanged);
    connect(this,               &MissionSettingsItem::amslEntryAltChanged,              this, &MissionSettingsItem::amslExitAltChanged);
    connect(this,               &MissionSettingsItem::amslEntryAltChanged,              this, &MissionSettingsItem::minAMSLAltitudeChanged);
    connect(this,               &MissionSettingsItem::amslEntryAltChanged,              this, &MissionSettingsItem::maxAMSLAltitudeChanged);

    connect(&_plannedHomePositionAltitudeFact,  &Fact::rawValueChanged,                 this, &MissionSettingsItem::_updateAltitudeInCoordinate);

    connect(_managerVehicle, &Vehicle::homePositionChanged, this, &MissionSettingsItem::_updateHomePosition);
    _updateHomePosition(_managerVehicle->homePosition());
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

    // First item should be planned home position, we are not responsible for save/load
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

bool MissionSettingsItem::load(const QJsonObject& /*complexObject*/, int /*sequenceNumber*/, QString& /*errorString*/)
{
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

bool MissionSettingsItem::addMissionEndAction(QList<MissionItem*>& /*items*/, int /*seqNum*/, QObject* /*missionItemParent*/)
{
    return false;
}

bool MissionSettingsItem::scanForMissionSettings(QmlObjectListModel* visualItems, int scanIndex)
{
    bool foundSpeedSection = false;
    bool foundCameraSection = false;

    qCDebug(MissionSettingsItemLog) << "MissionSettingsItem::scanForMissionSettings count:scanIndex" << visualItems->count() << scanIndex;

    // Scan through the initial mission items for possible mission settings
    foundCameraSection = _cameraSection.scanForSection(visualItems, scanIndex);
    foundSpeedSection = _speedSection.scanForSection(visualItems, scanIndex);

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

void MissionSettingsItem::_setCoordinateWorker(const QGeoCoordinate& coordinate)
{
    if (_plannedHomePositionCoordinate != coordinate) {
        _plannedHomePositionCoordinate = coordinate;
        emit coordinateChanged(coordinate);
        emit exitCoordinateChanged(coordinate);
        if (_plannedHomePositionFromVehicle) {
            _plannedHomePositionAltitudeFact.setRawValue(coordinate.altitude());
        }
    }
}

void MissionSettingsItem::setHomePositionFromVehicle(Vehicle* vehicle)
{
    // If the user hasn't moved the planned home position manually we use the value from the vehicle
    if (!_plannedHomePositionMovedByUser) {
        QGeoCoordinate coordinate = vehicle->homePosition();
        // ArduPilot tends to send crap home positions at initial vehicle boot, discard them
        if (coordinate.isValid() && (coordinate.latitude() != 0 || coordinate.longitude() != 0)) {
            _plannedHomePositionFromVehicle = true;
            _setCoordinateWorker(coordinate);
        }
    }
}

void MissionSettingsItem::setInitialHomePosition(const QGeoCoordinate& coordinate)
{
    _plannedHomePositionMovedByUser = false;
    _plannedHomePositionFromVehicle = false;
    _setCoordinateWorker(coordinate);
}

void MissionSettingsItem::setInitialHomePositionFromUser(const QGeoCoordinate& coordinate)
{
    _plannedHomePositionMovedByUser = true;
    _plannedHomePositionFromVehicle = false;
    _setCoordinateWorker(coordinate);
}


void MissionSettingsItem::setCoordinate(const QGeoCoordinate& coordinate)
{
    if (coordinate != this->coordinate()) {
        // The user is moving the planned home position manually. Stop tracking vehicle home position.
        _plannedHomePositionMovedByUser = true;
        _plannedHomePositionFromVehicle = false;
        _setCoordinateWorker(coordinate);
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

double MissionSettingsItem::specifiedGimbalPitch(void)
{
    return _cameraSection.specifyGimbal() ? _cameraSection.gimbalPitch()->rawValue().toDouble() : std::numeric_limits<double>::quiet_NaN();
}

void MissionSettingsItem::_updateAltitudeInCoordinate(QVariant value)
{
    double newAltitude = value.toDouble();

    if (!QGC::fuzzyCompare(_plannedHomePositionCoordinate.altitude(), newAltitude)) {
        qCDebug(MissionSettingsItemLog) << "MissionSettingsItem::_updateAltitudeInCoordinate" << newAltitude;
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

void MissionSettingsItem::_setHomeAltFromTerrain(double terrainAltitude)
{
    if (!_plannedHomePositionFromVehicle && !qIsNaN(terrainAltitude)) {
        qCDebug(MissionSettingsItemLog) << "MissionSettingsItem::_setHomeAltFromTerrain" << terrainAltitude;
        _plannedHomePositionAltitudeFact.setRawValue(terrainAltitude);
    }
}

QString MissionSettingsItem::abbreviation(void) const
{
    return _flyView ? tr("L") : tr("Launch");
}

void MissionSettingsItem::_updateHomePosition(const QGeoCoordinate& homePosition)
{
    if (_flyView) {
        setCoordinate(homePosition);
    }
}
