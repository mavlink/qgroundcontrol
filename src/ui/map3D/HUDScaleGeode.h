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
 *   @brief Definition of the class HUDScaleGeode.
 *
 *   @author Lionel Heng <hengli@student.ethz.ch>
 *
 */

#ifndef HUDSCALEGEODE_H
#define HUDSCALEGEODE_H

#include <osg/Geode>
#include <osgText/Text>
#include <QString>

class HUDScaleGeode : public osg::Geode
{
public:
    HUDScaleGeode();

    void init(osg::ref_ptr<osgText::Font>& font);
    void update(int windowHeight, float cameraFov, float cameraDistance,
                bool darkBackground);

private:
    osg::ref_ptr<osgText::Text> text;
};

#endif // HUDSCALEGEODE_H
