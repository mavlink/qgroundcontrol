/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "PX4AutoPilotPlugin.h"
#include "YuneecSensorsComponent.h"
#include "YuneecSafetyComponent.h"
#include "Vehicle.h"

/// Yuneec overrides from standard PX4AutoPilotPlugin implementation
class YuneecAutoPilotPlugin : public PX4AutoPilotPlugin
{
    Q_OBJECT

public:
    YuneecAutoPilotPlugin(Vehicle* vehicle, QObject* parent);

    // Overrides from PX4AutoPilotPlugin
    const QVariantList& vehicleComponents(void) final;

private:
    QVariantList            _components;
    YuneecSensorsComponent* _sensorsComponent;
    YuneecSafetyComponent*  _safetyComponent;
};
