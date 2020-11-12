/****************************************************************************
 *
 * (c) 2009-2019 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 * @file
 *   @brief Custom Autopilot Plugin
 *   @author Gus Grubba <gus@auterion.com>
 */

#pragma once

#include "PX4AutoPilotPlugin.h"
#include "Vehicle.h"

class CustomAutoPilotPlugin : public PX4AutoPilotPlugin
{
    Q_OBJECT
public:
    CustomAutoPilotPlugin(Vehicle* vehicle, QObject* parent);

    const QVariantList& vehicleComponents() final;

private slots:
    void         _advancedChanged        (bool advanced);

private:
    QVariantList _components;

};
