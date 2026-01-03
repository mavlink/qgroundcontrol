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
