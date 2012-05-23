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

#include "ObstacleGroupNode.h"

#include <limits>
#include <osg/PositionAttitudeTransform>
#include <osg/ShapeDrawable>

#include "gpl.h"
#include "Imagery.h"

ObstacleGroupNode::ObstacleGroupNode()
{

}

void
ObstacleGroupNode::init(void)
{

}

void
ObstacleGroupNode::clear(void)
{
    if (getNumChildren() > 0)
    {
        removeChild(0, getNumChildren());
    }
}

void
ObstacleGroupNode::update(double robotX, double robotY, double robotZ,
                          const px::ObstacleList& obstacleList)
{
    clear();

    osg::ref_ptr<osg::Geode> geode = new osg::Geode;

    // find minimum and maximum height
    float zMin = std::numeric_limits<float>::max();
    float zMax = std::numeric_limits<float>::min();
    for (int i = 0; i < obstacleList.obstacles_size(); ++i)
    {
        const px::Obstacle& obs = obstacleList.obstacles(i);

        float z = robotZ - obs.z();

        if (zMin > z)
        {
            zMin = z;
        }
        if (zMax < z)
        {
            zMax = z;
        }
    }

    for (int i = 0; i < obstacleList.obstacles_size(); ++i)
    {
        const px::Obstacle& obs = obstacleList.obstacles(i);

        osg::Vec3d obsPos(obs.y() - robotY, obs.x() - robotX, robotZ - obs.z());

        osg::ref_ptr<osg::Box> box =
            new osg::Box(obsPos, obs.width(), obs.width(), obs.height());
        osg::ref_ptr<osg::ShapeDrawable> sd = new osg::ShapeDrawable(box);

        int idx = (obsPos.z() - zMin) / (zMax - zMin) * 127.0f;
        float r, g, b;
        qgc::colormap("jet", idx, r, g, b);

        sd->getOrCreateStateSet()->setMode(GL_BLEND, osg::StateAttribute::ON);
        sd->setColor(osg::Vec4(r, g, b, 1.0f));
        sd->setUseDisplayList(false);

        geode->addDrawable(sd);
    }

    addChild(geode);
}

