/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/// @file
///     @author Don Gagne <don@thegagnes.com>

#include "GenericAutoPilotPlugin.h"

GenericAutoPilotPlugin::GenericAutoPilotPlugin(Vehicle* vehicle, QObject* parent) :
    AutoPilotPlugin(vehicle, parent)
{
    Q_ASSERT(vehicle);
}

const QVariantList& GenericAutoPilotPlugin::vehicleComponents(void)
{
    static QVariantList emptyList;
    
    return emptyList;
}

/// This will perform various checks prior to signalling that the plug in ready
void GenericAutoPilotPlugin::_parametersReadyPreChecks(bool missingParameters)
{
    _parametersReady = true;
    _missingParameters = missingParameters;
    emit missingParametersChanged(_missingParameters);
    emit parametersReadyChanged(_parametersReady);
}
