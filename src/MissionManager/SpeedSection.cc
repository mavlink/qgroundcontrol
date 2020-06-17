/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "SpeedSection.h"
#include "JsonHelper.h"
#include "FirmwarePlugin.h"
#include "SimpleMissionItem.h"
#include "PlanMasterController.h"

const char* SpeedSection::_flightSpeedName = "FlightSpeed";

QMap<QString, FactMetaData*> SpeedSection::_metaDataMap;

SpeedSection::SpeedSection(PlanMasterController* masterController, QObject* parent)
    : Section               (masterController, parent)
    , _available            (false)
    , _dirty                (false)
    , _specifyFlightSpeed   (false)
    , _flightSpeedFact      (0, _flightSpeedName,   FactMetaData::valueTypeDouble)
{
    if (_metaDataMap.isEmpty()) {
        _metaDataMap = FactMetaData::createMapFromJsonFile(QStringLiteral(":/json/SpeedSection.FactMetaData.json"), nullptr /* metaDataParent */);
    }

    double flightSpeed = 0;
    if (_masterController->controllerVehicle()->multiRotor()) {
        flightSpeed = _masterController->controllerVehicle()->defaultHoverSpeed();
    } else {
        flightSpeed = _masterController->controllerVehicle()->defaultCruiseSpeed();
    }

    _metaDataMap[_flightSpeedName]->setRawDefaultValue(flightSpeed);
    _flightSpeedFact.setMetaData(_metaDataMap[_flightSpeedName]);
    _flightSpeedFact.setRawValue(flightSpeed);

    connect(this,               &SpeedSection::specifyFlightSpeedChanged,   this, &SpeedSection::settingsSpecifiedChanged);
    connect(&_flightSpeedFact,  &Fact::valueChanged,                        this, &SpeedSection::_flightSpeedChanged);

    connect(this,               &SpeedSection::specifyFlightSpeedChanged,   this, &SpeedSection::_updateSpecifiedFlightSpeed);
    connect(&_flightSpeedFact,  &Fact::valueChanged,                        this, &SpeedSection::_updateSpecifiedFlightSpeed);
}

bool SpeedSection::settingsSpecified(void) const
{
    return _specifyFlightSpeed;
}

void SpeedSection::setAvailable(bool available)
{
    if (available != _available) {
        if (available && (_masterController->controllerVehicle()->multiRotor() || _masterController->controllerVehicle()->fixedWing())) {
            _available = available;
            emit availableChanged(available);
        }
    }
}

void SpeedSection::setDirty(bool dirty)
{
    if (_dirty != dirty) {
        _dirty = dirty;
        emit dirtyChanged(_dirty);
    }
}

void SpeedSection::setSpecifyFlightSpeed(bool specifyFlightSpeed)
{
    if (specifyFlightSpeed != _specifyFlightSpeed) {
        _specifyFlightSpeed = specifyFlightSpeed;
        emit specifyFlightSpeedChanged(specifyFlightSpeed);
        setDirty(true);
        emit itemCountChanged(itemCount());
    }
}

int SpeedSection::itemCount(void) const
{
    return _specifyFlightSpeed ? 1: 0;
}

void SpeedSection::appendSectionItems(QList<MissionItem*>& items, QObject* missionItemParent, int& seqNum)
{
    // IMPORTANT NOTE: If anything changes here you must also change SpeedSection::scanForSettings

    if (_specifyFlightSpeed) {
        MissionItem* item = new MissionItem(seqNum++,
                                            MAV_CMD_DO_CHANGE_SPEED,
                                            MAV_FRAME_MISSION,
                                            _masterController->controllerVehicle()->multiRotor() ? 1 /* groundspeed */ : 0 /* airspeed */,    // Change airspeed or groundspeed
                                            _flightSpeedFact.rawValue().toDouble(),
                                            -1,                                                                 // No throttle change
                                            0,                                                                  // Absolute speed change
                                            0, 0, 0,                                                            // param 5-7 not used
                                            true,                                                               // autoContinue
                                            false,                                                              // isCurrentItem
                                            missionItemParent);
        items.append(item);
    }
}

bool SpeedSection::scanForSection(QmlObjectListModel* visualItems, int scanIndex)
{
    if (!_available || scanIndex >= visualItems->count()) {
        return false;
    }

    SimpleMissionItem* item = visualItems->value<SimpleMissionItem*>(scanIndex);
    if (!item) {
        // We hit a complex item, there can't be a speed setting
        return false;
    }
    MissionItem& missionItem = item->missionItem();

    // See SpeedSection::appendMissionItems for specs on what consitutes a known speed setting

    if (missionItem.command() == MAV_CMD_DO_CHANGE_SPEED && missionItem.param3() == -1 && missionItem.param4() == 0 && missionItem.param5() == 0 && missionItem.param6() == 0 && missionItem.param7() == 0) {
        if (_masterController->controllerVehicle()->multiRotor() && missionItem.param1() != 1) {
            return false;
        } else if (_masterController->controllerVehicle()->fixedWing() && missionItem.param1() != 0) {
            return false;
        }
        visualItems->removeAt(scanIndex)->deleteLater();
        _flightSpeedFact.setRawValue(missionItem.param2());
        setSpecifyFlightSpeed(true);
        return true;
    }

    return false;
}


double SpeedSection::specifiedFlightSpeed(void) const
{
    return _specifyFlightSpeed ? _flightSpeedFact.rawValue().toDouble() : std::numeric_limits<double>::quiet_NaN();
}

void SpeedSection::_updateSpecifiedFlightSpeed(void)
{
    if (_specifyFlightSpeed) {
        emit specifiedFlightSpeedChanged(specifiedFlightSpeed());
    }
}

void SpeedSection::_flightSpeedChanged(void)
{
    // We only set the dirty bit if specify flight speed it set. This allows us to change defaults for flight speed
    // without affecting dirty.
    if (_specifyFlightSpeed) {
        setDirty(true);
    }
}
