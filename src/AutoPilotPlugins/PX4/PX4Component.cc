/*=====================================================================
 
 QGroundControl Open Source Ground Control Station
 
 (c) 2009 - 2014 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 
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

#include "PX4Component.h"

PX4Component::PX4Component(UASInterface* uas, AutoPilotPlugin* autopilot, QObject* parent) :
    VehicleComponent(uas, autopilot, parent)
{
    bool fSuccess = connect(_paramMgr, SIGNAL(parameterUpdated(int, QString, QVariant)), this, SLOT(_parameterUpdated(int, QString, QVariant)));
    Q_ASSERT(fSuccess);
    Q_UNUSED(fSuccess);
}

void PX4Component::_parameterUpdated(int compId, QString paramName, QVariant value)
{
    Q_UNUSED(value);
    
    if (compId == _paramMgr->getDefaultComponentId()) {
        QStringList triggerList = setupCompleteChangedTriggerList();
        foreach(QString triggerParam, triggerList) {
            if (paramName == triggerParam) {
                emit setupCompleteChanged(setupComplete());
                return;
            }
        }
    }
}
