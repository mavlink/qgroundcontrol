///*=====================================================================
//
//QGroundControl Open Source Ground Control Station
//
//(c) 2009, 2010 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
//
//This file is part of the QGROUNDCONTROL project
//
//    QGROUNDCONTROL is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    QGROUNDCONTROL is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.
//
//======================================================================*/

/**
 * @file
 *   @brief Definition of the class ImageWindowGeode.
 *
 *   @author Lionel Heng <hengli@student.ethz.ch>
 *
 */

#ifndef IMAGEWINDOWGEODE_H
#define IMAGEWINDOWGEODE_H

#include <osg/Geode>
#include <osg/Geometry>
#include <osgText/Text>
#include <QString>

class ImageWindowGeode : public osg::Geode
{
public:
    ImageWindowGeode();
    void init(const QString& caption, const osg::Vec4& backgroundColor,
              osg::ref_ptr<osg::Image>& image,
              osg::ref_ptr<osgText::Font>& font);

    void setAttributes(int x, int y, int width, int height);

private:
    int border;

    osg::ref_ptr<osg::Vec3Array> imageVertices;
    osg::ref_ptr<osg::Vec3Array> backgroundVertices;
    osg::ref_ptr<osgText::Text> text;
};

#endif // IMAGEWINDOWGEODE_H
