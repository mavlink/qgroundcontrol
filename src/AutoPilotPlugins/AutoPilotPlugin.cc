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
#include "QGCUASParamManagerInterface.h"

AutoPilotPlugin::AutoPilotPlugin(UASInterface* uas, QObject* parent) :
    QObject(parent),
    _uas(uas),
    _pluginReady(false)
{
    Q_ASSERT(_uas);
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

bool AutoPilotPlugin::parameterExists(const QString& name)
{
	return _getParameterLoader()->parameterExists(FactSystem::defaultComponentId, name);
}

Fact* AutoPilotPlugin::getParameterFact(const QString& name)
{
	return _getParameterLoader()->getFact(FactSystem::defaultComponentId, name);
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
	Q_ASSERT(_uas);
	
	_uas->getParamManager()->writeOnboardParamsToStream(stream, _uas->getUASName());
}

void AutoPilotPlugin::readParametersFromStream(QTextStream &stream)
{
	Q_ASSERT(_uas);
	
	Fact* autoSaveFact = NULL;
	int previousAutoSave = 0;
	
	if (parameterExists("COM_AUTOS_PAR")) {
		autoSaveFact = getParameterFact("COM_AUTOS_PAR");
		previousAutoSave = autoSaveFact->value().toInt();
		autoSaveFact->setValue(1);
	}
	
	_uas->getParamManager()->readPendingParamsFromStream(stream);
	
	if (autoSaveFact) {
		autoSaveFact->setValue(previousAutoSave);
	}
}
