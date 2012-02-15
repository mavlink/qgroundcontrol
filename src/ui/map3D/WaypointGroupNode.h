/*=====================================================================

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

/**
 * @file
 *   @brief Definition of the class WaypointGroupNode.
 *
 *   @author Lionel Heng <hengli@inf.ethz.ch>
 *
 */

#ifndef WAYPOINTGROUPNODE_H
#define WAYPOINTGROUPNODE_H

#include <osg/Group>

#include "UASInterface.h"

class WaypointGroupNode : public osg::Group
{
public:
    WaypointGroupNode(const QColor& color);

    void init(void);
    void update(UASInterface* uas, MAV_FRAME frame);

private:
    void getPosition(Waypoint* wp, double& x, double& y, double& z);

    QColor mColor;
};

#endif // WAYPOINTGROUPNODE_H
