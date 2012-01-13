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

#include "HUDScaleGeode.h"

#include <osg/Geometry>
#include <osg/LineWidth>

HUDScaleGeode::HUDScaleGeode()
{

}

void
HUDScaleGeode::init(osg::ref_ptr<osgText::Font>& font)
{
    osg::ref_ptr<osg::Vec2Array> outlineVertices(new osg::Vec2Array);
    outlineVertices->push_back(osg::Vec2(20.0f, 50.0f));
    outlineVertices->push_back(osg::Vec2(20.0f, 70.0f));
    outlineVertices->push_back(osg::Vec2(20.0f, 60.0f));
    outlineVertices->push_back(osg::Vec2(100.0f, 60.0f));
    outlineVertices->push_back(osg::Vec2(100.0f, 50.0f));
    outlineVertices->push_back(osg::Vec2(100.0f, 70.0f));

    osg::ref_ptr<osg::Geometry> outlineGeometry(new osg::Geometry);
    outlineGeometry->setVertexArray(outlineVertices);

    osg::ref_ptr<osg::Vec4Array> outlineColor(new osg::Vec4Array);
    outlineColor->push_back(osg::Vec4(1.0f, 1.0f, 1.0f, 1.0f));
    outlineGeometry->setColorArray(outlineColor);
    outlineGeometry->setColorBinding(osg::Geometry::BIND_OVERALL);

    outlineGeometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::LINES,
                                     0, 6));

    osg::ref_ptr<osg::LineWidth> outlineWidth(new osg::LineWidth());
    outlineWidth->setWidth(4.0f);
    outlineGeometry->getOrCreateStateSet()->
    setAttributeAndModes(outlineWidth, osg::StateAttribute::ON);

    addDrawable(outlineGeometry);

    osg::ref_ptr<osg::Vec2Array> markerVertices(new osg::Vec2Array);
    markerVertices->push_back(osg::Vec2(20.0f, 50.0f));
    markerVertices->push_back(osg::Vec2(20.0f, 70.0f));
    markerVertices->push_back(osg::Vec2(20.0f, 60.0f));
    markerVertices->push_back(osg::Vec2(100.0f, 60.0f));
    markerVertices->push_back(osg::Vec2(100.0f, 50.0f));
    markerVertices->push_back(osg::Vec2(100.0f, 70.0f));

    osg::ref_ptr<osg::Geometry> markerGeometry(new osg::Geometry);
    markerGeometry->setVertexArray(markerVertices);

    osg::ref_ptr<osg::Vec4Array> markerColor(new osg::Vec4Array);
    markerColor->push_back(osg::Vec4(0.0f, 0.0f, 0.0f, 1.0f));
    markerGeometry->setColorArray(markerColor);
    markerGeometry->setColorBinding(osg::Geometry::BIND_OVERALL);

    markerGeometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::LINES,
                                    0, 6));

    osg::ref_ptr<osg::LineWidth> markerWidth(new osg::LineWidth());
    markerWidth->setWidth(1.5f);
    markerGeometry->getOrCreateStateSet()->
    setAttributeAndModes(markerWidth, osg::StateAttribute::ON);

    addDrawable(markerGeometry);

    text = new osgText::Text;
    text->setCharacterSize(11);
    text->setFont(font);
    text->setAxisAlignment(osgText::Text::SCREEN);
    text->setColor(osg::Vec4(1.0f, 1.0f, 1.0f, 1.0f));
    text->setPosition(osg::Vec3(40.0f, 45.0f, -1.5f));

    addDrawable(text);
}

void
HUDScaleGeode::update(int windowHeight, float cameraFov, float cameraDistance,
                      bool darkBackground)
{
    float f = static_cast<float>(windowHeight) / 2.0f
              / tanf(cameraFov / 180.0f * M_PI / 2.0f);
    float dist = cameraDistance / f * 80.0f;

    if (darkBackground) {
        text->setColor(osg::Vec4(1.0f, 1.0f, 1.0f, 1.0f));
    } else {
        text->setColor(osg::Vec4(0.0f, 0.0f, 0.0f, 1.0f));
    }

    text->setText(QString("%1 m").arg(dist, 0, 'f', 2).toStdString());
}
