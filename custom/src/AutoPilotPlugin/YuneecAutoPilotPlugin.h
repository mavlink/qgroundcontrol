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

class GimbalComponent;
class ChannelComponent;
class HealthComponent;
class YuneecSafetyComponent;

/// Yuneec overrides from standard PX4AutoPilotPlugin implementation
class YuneecAutoPilotPlugin : public PX4AutoPilotPlugin
{
    Q_OBJECT

public:
    YuneecAutoPilotPlugin(Vehicle* vehicle, QObject* parent);

    // Overrides from PX4AutoPilotPlugin
    const QVariantList& vehicleComponents(void) final;

private slots:
    void    _advancedChanged    (bool advanced);

private:
    GimbalComponent*        _gimbalComponent;
    ChannelComponent*       _channelComponent;
    HealthComponent*        _healthComponent;
    QVariantList            _components;
    YuneecSafetyComponent*  _yuneecSafetyComponent;
};
