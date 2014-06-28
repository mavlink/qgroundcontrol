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

#include "MockUAS.h"

QString MockUAS::_bogusStaticString;

MockUAS::MockUAS(void) :
    _systemType(MAV_TYPE_QUADROTOR),
    _systemId(1),
    _mavlinkPlugin(NULL)
{
    
}

void MockUAS::setMockParametersAndSignal(MockQGCUASParamManager::ParamMap_t& map)
{
    _paramManager.setMockParameters(map);
    
    QMapIterator<QString, QVariant> i(map);
    while (i.hasNext()) {
        i.next();
        emit parameterChanged(_systemId, 0, i.key(), i.value());
    }
}

void MockUAS::sendMessage(mavlink_message_t message)
{
    if (!_mavlinkPlugin) {
        Q_ASSERT(false);
    }
    
    _mavlinkPlugin->sendMessage(message);
}