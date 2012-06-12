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
 *   @brief Definition of the class PixhawkCheetahNode.
 *
 *   @author Lionel Heng <hengli@student.ethz.ch>
 *
 */

#ifndef PIXHAWKCHEETAHNODE_H_
#define PIXHAWKCHEETAHNODE_H_

#include <osg/Geode>
#include <QColor>

/**
 * @brief The PixhawkCheetahNode class.
 * Generates an OpenSceneGraph node which renders a Pixhawk Cheetah MAV.
 **/

class PixhawkCheetahNode
{
public:
    /**
     * Constructor.
     */
    PixhawkCheetahNode();

    /**
     * Get a single instance of the node.
     */
    static osg::ref_ptr<osg::Node> instance(void);

    /**
     * @brief Creates an OpenSceneGraph node which renders a Pixhawk Cheetah MAV.
     * @param red Red intensity of the MAV model.
     * @param green Green intensity of the MAV model.
     * @param blue Blue intensity of the MAV model.
     *
     * @return A smart pointer to the node.
     **/
    static osg::ref_ptr<osg::Node> create(const QColor& color);

private:
    static osg::ref_ptr<osg::Node> mInstance;
};

#endif
