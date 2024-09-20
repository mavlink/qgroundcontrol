/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "APMFollowComponentController.h"
#include "ArduRoverFirmwarePlugin.h"
#include "Vehicle.h"

APMFollowComponentController::APMFollowComponentController(QObject *parent)
    : FactPanelController(parent)
    , _metaDataMap(FactMetaData::createMapFromJsonFile(QStringLiteral(":/json/APMFollowComponent.FactMetaData.json"), this))
    , _angleFact(new SettingsFact(_settingsGroup, _metaDataMap[_angleName], this))
    , _distanceFact(new SettingsFact(_settingsGroup, _metaDataMap[_distanceName], this))
    , _heightFact(new SettingsFact(_settingsGroup, _metaDataMap[_heightName], this))
{
    // qCDebug() << Q_FUNC_INFO << this;
}

APMFollowComponentController::~APMFollowComponentController()
{
    // qCDebug() << Q_FUNC_INFO << this;
}

bool APMFollowComponentController::roverFirmware()
{
    return !!qobject_cast<ArduRoverFirmwarePlugin*>(_vehicle->firmwarePlugin());
}
