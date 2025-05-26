/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#pragma once

#include "AutoPilotPlugin.h"

/// This is the generic implementation of the AutoPilotPlugin class for mavs
/// we do not have a specific AutoPilotPlugin implementation.
class GenericAutoPilotPlugin : public AutoPilotPlugin
{
    Q_OBJECT

public:
    explicit GenericAutoPilotPlugin(Vehicle *vehicle, QObject *parent = nullptr);

    const QVariantList &vehicleComponents() final;
    QString prerequisiteSetup(VehicleComponent *component) const final { Q_UNUSED(component); return QString(); }
};
