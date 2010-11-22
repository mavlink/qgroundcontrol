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
 *   @brief Definition of the class QMap3D.
 *
 *   @author James Goppert <james.goppert@gmail.com>
 *
 */

#include "QMap3D.h"
#include <osgEarthUtil/EarthManipulator>
#include <stdexcept>

QMap3D::QMap3D(QWidget * parent, const char * name, WindowFlags f) :
        QWidget(parent,f)
{
    setupUi(this);
    osg::ref_ptr<osg::Node> map = osgDB::readNodeFile("data/yahoo_heightfield.earth");
    if (!map) throw std::runtime_error("unable to load file");
    graphicsView->updateCamera();
    graphicsView->setCameraManipulator(new osgEarthUtil::EarthManipulator);
    graphicsView->setSceneData(map);
    show();
}

QMap3D::~QMap3D()
{
}
