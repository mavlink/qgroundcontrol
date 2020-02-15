/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#ifndef GENERICAUTOPILOT_H
#define GENERICAUTOPILOT_H

#include "AutoPilotPlugin.h"

/// @file
///     @brief This is the generic implementation of the AutoPilotPlugin class for mavs
///             we do not have a specific AutoPilotPlugin implementation.
///     @author Don Gagne <don@thegagnes.com>

class GenericAutoPilotPlugin : public AutoPilotPlugin
{
    Q_OBJECT

public:
    GenericAutoPilotPlugin(Vehicle* vehicle, QObject* parent = nullptr);
    
    // Overrides from AutoPilotPlugin
    const QVariantList& vehicleComponents(void) final;
    QString prerequisiteSetup(VehicleComponent* component) const final;
};

#endif
