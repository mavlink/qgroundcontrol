/*===================================================================
QGroundControl Open Source Ground Control Station

(c) 2009, 2010 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

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

#include "SimpleMissionItem.h"

SimpleMissionItem::SimpleMissionItem(Vehicle* vehicle, QObject* parent)
    : MissionItem(vehicle, parent)
{

}

SimpleMissionItem::SimpleMissionItem(Vehicle*       vehicle,
                         int             sequenceNumber,
                         MAV_CMD         command,
                         MAV_FRAME       frame,
                         double          param1,
                         double          param2,
                         double          param3,
                         double          param4,
                         double          param5,
                         double          param6,
                         double          param7,
                         bool            autoContinue,
                         bool            isCurrentItem,
                         QObject*        parent)
    : MissionItem(vehicle, sequenceNumber, command, frame, param1, param2, param3, param4, param5, param6, param7, autoContinue, isCurrentItem, parent)
{

}

SimpleMissionItem::SimpleMissionItem(const SimpleMissionItem& other, QObject* parent)
    : MissionItem(other, parent)
{

}

const SimpleMissionItem& SimpleMissionItem::operator=(const SimpleMissionItem& other)
{
    static_cast<MissionItem&>(*this) = other;

    return *this;
}
