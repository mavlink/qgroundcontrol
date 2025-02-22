/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "GenericAutoPilotPlugin.h"

GenericAutoPilotPlugin::GenericAutoPilotPlugin(Vehicle *vehicle, QObject *parent)
    : AutoPilotPlugin(vehicle, parent)
{

}

const QVariantList& GenericAutoPilotPlugin::vehicleComponents()
{
    static QVariantList emptyList;

    return emptyList;
}
