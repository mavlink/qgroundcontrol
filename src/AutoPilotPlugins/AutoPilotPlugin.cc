/*=====================================================================
 
 QGroundControl Open Source Ground Control Station
 
 (c) 2009 - 2015 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 
 This file is part of the QGROUNDCONTROL project
 
 QGROUNDCONTROL is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 QGROUNDCONTROL is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.
 
 ======================================================================*/

/// @file
///     @author Don Gagne <don@thegagnes.com>

#include "AutoPilotPlugin.h"
#include "QGCApplication.h"
#include "QGCMessageBox.h"
#include "MainWindow.h"
#include "ParameterLoader.h"

AutoPilotPlugin::AutoPilotPlugin(UASInterface* uas, QObject* parent) :
    QObject(parent),
    _uas(uas),
    _pluginReady(false),
	_setupComplete(false)
{
    Q_ASSERT(_uas);
	
	connect(_uas, &UASInterface::disconnected, this, &AutoPilotPlugin::_uasDisconnected);
    connect(_uas, &UASInterface::armingChanged, this, &AutoPilotPlugin::armedChanged);

	connect(this, &AutoPilotPlugin::pluginReadyChanged, this, &AutoPilotPlugin::_pluginReadyChanged);
}

AutoPilotPlugin::~AutoPilotPlugin()
{
    
}

void AutoPilotPlugin::_uasDisconnected(void)
{
	_pluginReady = false;
	emit pluginReadyChanged(_pluginReady);
}

void AutoPilotPlugin::_pluginReadyChanged(bool pluginReady)
{
	if (pluginReady) {
		_recalcSetupComplete();
		if (!_setupComplete) {
			QGCMessageBox::warning("Setup", "One or more vehicle components require setup prior to flight.");
			
			// Take the user to Vehicle Summary
			MainWindow* mainWindow = MainWindow::instance();
			Q_ASSERT(mainWindow);
			mainWindow->getMainToolBar()->onSetupView();
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
	Q_ASSERT(_pluginReady);
	return _setupComplete;
}

void AutoPilotPlugin::resetAllParametersToDefaults(void)
{
    mavlink_message_t msg;
    MAVLinkProtocol* mavlink = MAVLinkProtocol::instance();

    mavlink_msg_command_long_pack(mavlink->getSystemId(), mavlink->getComponentId(), &msg, _uas->getUASID(), 0, MAV_CMD_PREFLIGHT_STORAGE, 0, 2, -1, 0, 0, 0, 0, 0);
    _uas->sendMessage(msg);
}

void AutoPilotPlugin::refreshAllParameters(void)
{
	_getParameterLoader()->refreshAllParameters();
}

void AutoPilotPlugin::refreshParameter(int componentId, const QString& name)
{
	_getParameterLoader()->refreshParameter(componentId, name);
}

void AutoPilotPlugin::refreshParametersPrefix(int componentId, const QString& namePrefix)
{
	_getParameterLoader()->refreshParametersPrefix(componentId, namePrefix);
}

bool AutoPilotPlugin::parameterExists(int componentId, const QString& name)
{
    return _getParameterLoader()->parameterExists(componentId, name);
}

Fact* AutoPilotPlugin::getParameterFact(int componentId, const QString& name)
{
    return _getParameterLoader()->getFact(componentId, name);
}

bool AutoPilotPlugin::factExists(FactSystem::Provider_t provider, int componentId, const QString& name)
{
    switch (provider) {
        case FactSystem::ParameterProvider:
            return _getParameterLoader()->parameterExists(componentId, name);
            
        // Other providers will go here once they come online
    }
    
    Q_ASSERT(false);
    return false;
}

Fact* AutoPilotPlugin::getFact(FactSystem::Provider_t provider, int componentId, const QString& name)
{
    switch (provider) {
        case FactSystem::ParameterProvider:
            return _getParameterLoader()->getFact(componentId, name);
            
        // Other providers will go here once they come online
    }
    
    Q_ASSERT(false);
    return NULL;
}

QStringList AutoPilotPlugin::parameterNames(void)
{
	return _getParameterLoader()->parameterNames();
}

const QMap<int, QMap<QString, QStringList> >& AutoPilotPlugin::getGroupMap(void)
{
    return _getParameterLoader()->getGroupMap();
}

void AutoPilotPlugin::writeParametersToStream(QTextStream &stream)
{
	_getParameterLoader()->writeParametersToStream(stream, _uas->getUASName());
}

QString AutoPilotPlugin::readParametersFromStream(QTextStream &stream)
{
	return _getParameterLoader()->readParametersFromStream(stream);
}

bool AutoPilotPlugin::armed(void)
{
    return _uas->isArmed();
}
