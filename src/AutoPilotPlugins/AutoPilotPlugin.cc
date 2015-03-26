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

AutoPilotPlugin::AutoPilotPlugin(UASInterface* uas, QObject* parent) :
    QObject(parent),
    _uas(uas),
    _pluginReady(false)
{
    Q_ASSERT(_uas);
}

void AutoPilotPlugin::refreshAllParameters(void)
{
    Q_ASSERT(_uas);
    QGCUASParamManagerInterface* paramMgr = _uas->getParamManager();
    Q_ASSERT(paramMgr);
    paramMgr->requestParameterList();
}

void AutoPilotPlugin::refreshParameter(const QString& param)
{
    Q_ASSERT(_uas);
    QGCUASParamManagerInterface* paramMgr = _uas->getParamManager();
    Q_ASSERT(paramMgr);
    
    QList<int> compIdList = paramMgr->getComponentForParam(param);
    Q_ASSERT(compIdList.count() > 0);
    paramMgr->requestParameterUpdate(compIdList[0], param);
}

void AutoPilotPlugin::refreshParametersPrefix(const QString& paramPrefix)
{
    foreach(QVariant varFact, parameters()) {
        Fact* fact = qvariant_cast<Fact*>(varFact);
        Q_ASSERT(fact);
        if (fact->name().startsWith(paramPrefix)) {
            refreshParameter(fact->name());
        }
    }
}

bool AutoPilotPlugin::factExists(const QString& param)
{
    return parameters().contains(param);
}

Fact* AutoPilotPlugin::getFact(const QString& name)
{
    return parameters()[name].value<Fact*>();
}
