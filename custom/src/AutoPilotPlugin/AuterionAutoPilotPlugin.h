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
#include "Vehicle.h"

/// Auterion overrides from standard PX4AutoPilotPlugin implementation
class AuterionAutoPilotPlugin : public PX4AutoPilotPlugin
{
    Q_OBJECT
public:
    AuterionAutoPilotPlugin(Vehicle* vehicle, QObject* parent);
    const QVariantList& vehicleComponents() override;
private:
    QVariantList _components;

};
