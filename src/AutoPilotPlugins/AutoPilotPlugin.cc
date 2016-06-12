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

#include "AutoPilotPlugin.h"
#include "QGCApplication.h"
#include "ParameterLoader.h"
#include "UAS.h"
#include "FirmwarePlugin.h"

AutoPilotPlugin::AutoPilotPlugin(Vehicle* vehicle, QObject* parent)
    : QObject(parent)
    , _vehicle(vehicle)
    , _firmwarePlugin(vehicle->firmwarePlugin())
    , _parametersReady(false)
    , _missingParameters(false)
	, _setupComplete(false)
{
    Q_ASSERT(vehicle);
	
	connect(_vehicle->uas(), &UASInterface::disconnected, this, &AutoPilotPlugin::_uasDisconnected);

	connect(this, &AutoPilotPlugin::parametersReadyChanged, this, &AutoPilotPlugin::_parametersReadyChanged);
}

AutoPilotPlugin::~AutoPilotPlugin()
{
    
}

void AutoPilotPlugin::_uasDisconnected(void)
{
	_parametersReady = false;
	emit parametersReadyChanged(_parametersReady);
}

void AutoPilotPlugin::_parametersReadyChanged(bool parametersReady)
{
	if (parametersReady) {
		_recalcSetupComplete();
		if (!_setupComplete) {
            qgcApp()->showMessage("One or more vehicle components require setup prior to flight.");
			
			// Take the user to Vehicle Summary
            qgcApp()->showSetupView();
			qgcApp()->processEvents(QEventLoop::ExcludeUserInputEvents);
		}
	}
}

void AutoPilotPlugin::_recalcSetupComplete(void)
{
	bool newSetupComplete = true;
	
	foreach(const QVariant componentVariant, vehicleComponents()) {
		VehicleComponent* component = qobject_cast<VehicleComponent*>(qvariant_cast<QObject *>(componentVariant));
		Q_ASSERT(component);
		
		if (!component->setupComplete()) {
			newSetupComplete = false;
			break;
		}
	}
	
	if (_setupComplete != newSetupComplete) {
		_setupComplete = newSetupComplete;
		emit setupCompleteChanged(_setupComplete);
	}
}

bool AutoPilotPlugin::setupComplete(void)
{
	Q_ASSERT(_parametersReady);
	return _setupComplete;
}

void AutoPilotPlugin::resetAllParametersToDefaults(void)
{
    mavlink_message_t msg;
    MAVLinkProtocol* mavlink = qgcApp()->toolbox()->mavlinkProtocol();

    mavlink_msg_command_long_pack(mavlink->getSystemId(),
                                  mavlink->getComponentId(),
                                  &msg,
                                  _vehicle->id(),                   // Target systeem
                                  _vehicle->defaultComponentId(),   // Target component
                                  MAV_CMD_PREFLIGHT_STORAGE,
                                  0,                                // Confirmation
                                  2,                                // 2 = Reset params to default
                                  -1,                               // -1 = No change to mission storage
                                  0,                                // 0 = Ignore
                                  0, 0, 0, 0);                      // Unused
    _vehicle->sendMessageOnPriorityLink(msg);
}

void AutoPilotPlugin::refreshAllParameters(unsigned char componentID)
{
    _vehicle->getParameterLoader()->refreshAllParameters((uint8_t)componentID);
}

void AutoPilotPlugin::refreshParameter(int componentId, const QString& name)
{
    _vehicle->getParameterLoader()->refreshParameter(componentId, name);
}

void AutoPilotPlugin::refreshParametersPrefix(int componentId, const QString& namePrefix)
{
    _vehicle->getParameterLoader()->refreshParametersPrefix(componentId, namePrefix);
}

bool AutoPilotPlugin::parameterExists(int componentId, const QString& name)
{
    return _vehicle->getParameterLoader()->parameterExists(componentId, name);
}

Fact* AutoPilotPlugin::getParameterFact(int componentId, const QString& name)
{
    return _vehicle->getParameterLoader()->getFact(componentId, name);
}

bool AutoPilotPlugin::factExists(FactSystem::Provider_t provider, int componentId, const QString& name)
{
    switch (provider) {
        case FactSystem::ParameterProvider:
            return _vehicle->getParameterLoader()->parameterExists(componentId, name);
            
        // Other providers will go here once they come online
    }
    
    Q_ASSERT(false);
    return false;
}

Fact* AutoPilotPlugin::getFact(FactSystem::Provider_t provider, int componentId, const QString& name)
{
    switch (provider) {
        case FactSystem::ParameterProvider:
            return _vehicle->getParameterLoader()->getFact(componentId, name);
            
        // Other providers will go here once they come online
    }
    
    Q_ASSERT(false);
    return NULL;
}

QStringList AutoPilotPlugin::parameterNames(int componentId)
{
    return _vehicle->getParameterLoader()->parameterNames(componentId);
}

const QMap<int, QMap<QString, QStringList> >& AutoPilotPlugin::getGroupMap(void)
{
    return _vehicle->getParameterLoader()->getGroupMap();
}

void AutoPilotPlugin::writeParametersToStream(QTextStream &stream)
{
    _vehicle->getParameterLoader()->writeParametersToStream(stream);
}

QString AutoPilotPlugin::readParametersFromStream(QTextStream &stream)
{
    return _vehicle->getParameterLoader()->readParametersFromStream(stream);
}
