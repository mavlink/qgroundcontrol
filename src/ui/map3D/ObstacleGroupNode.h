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
 *   @brief Definition of the class ObstacleGroupNode.
 *
 *   @author Lionel Heng <hengli@inf.ethz.ch>
 *
 */

#ifndef OBSTACLEGROUPNODE_H
#define OBSTACLEGROUPNODE_H

#include <osg/Group>

#include "UASInterface.h"

class ObstacleGroupNode : public osg::Group
{
public:
    ObstacleGroupNode();

    void init(void);
    void clear(void);
    void update(double robotX, double robotY, double robotZ,
                const px::ObstacleList& obstacleList);
};

#endif // OBSTACLEGROUPNODE_H
