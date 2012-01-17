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

#include "ImageWindowGeode.h"

ImageWindowGeode::ImageWindowGeode()
    : border(5)
{

}

void
ImageWindowGeode::init(const QString& caption, const osg::Vec4& backgroundColor,
                       osg::ref_ptr<osg::Image>& image,
                       osg::ref_ptr<osgText::Font>& font)
{
    // image
    osg::ref_ptr<osg::Geometry> imageGeometry = new osg::Geometry;
    imageVertices = new osg::Vec3Array(4);

    osg::ref_ptr<osg::Vec2Array> textureCoords = new osg::Vec2Array;
    textureCoords->push_back(osg::Vec2(0.0f, 1.0f));
    textureCoords->push_back(osg::Vec2(1.0f, 1.0f));
    textureCoords->push_back(osg::Vec2(1.0f, 0.0f));
    textureCoords->push_back(osg::Vec2(0.0f, 0.0f));

    osg::ref_ptr<osg::Vec4Array> imageColors(new osg::Vec4Array);
    imageColors->push_back(osg::Vec4(1.0f, 1.0f, 1.0f, 1.0f));
    imageGeometry->setColorArray(imageColors);
    imageGeometry->setColorBinding(osg::Geometry::BIND_OVERALL);

    imageGeometry->setVertexArray(imageVertices);
    imageGeometry->setTexCoordArray(0, textureCoords);

    imageGeometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::POLYGON,
                                   0, imageVertices->size()));

    osg::ref_ptr<osg::Texture2D> texture = new osg::Texture2D;
    texture->setDataVariance(osg::Object::DYNAMIC);
    texture->setImage(image);
    texture->setResizeNonPowerOfTwoHint(false);

    imageGeometry->getOrCreateStateSet()->
    setTextureAttributeAndModes(0, texture, osg::StateAttribute::ON);
    imageGeometry->setUseDisplayList(false);

    // background
    osg::ref_ptr<osg::Geometry> backgroundGeometry = new osg::Geometry;
    backgroundVertices = new osg::Vec3Array(4);
    backgroundGeometry->setVertexArray(backgroundVertices);
    backgroundGeometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::POLYGON,
                                        0, backgroundVertices->size()));
    osg::ref_ptr<osg::Vec4Array> backgroundColors(new osg::Vec4Array);
    backgroundColors->push_back(backgroundColor);
    backgroundGeometry->setColorArray(backgroundColors);
    backgroundGeometry->setColorBinding(osg::Geometry::BIND_OVERALL);
    backgroundGeometry->setUseDisplayList(false);

    // caption
    text = new osgText::Text;
    text->setText(caption.toStdString().c_str());
    text->setCharacterSize(11);
    text->setFont(font);
    text->setAxisAlignment(osgText::Text::SCREEN);
    text->setColor(osg::Vec4(1.0f, 1.0f, 1.0f, 1.0f));

    addDrawable(imageGeometry);
    addDrawable(backgroundGeometry);
    addDrawable(text);

    setAttributes(0, 0, 0, 0);
}

void
ImageWindowGeode::setAttributes(int x, int y, int width, int height)
{
    int imageWidth = width - border * 2;
    int imageHeight = height - border * 2 - 15;
    int imageXPosition = x + border;
    int imageYPosition = y + border;

    imageVertices->at(0) = osg::Vec3(imageXPosition, imageYPosition, 0);
    imageVertices->at(1) = osg::Vec3(imageXPosition + imageWidth, imageYPosition, 0);
    imageVertices->at(2) = osg::Vec3(imageXPosition + imageWidth, imageYPosition + imageHeight, 0);
    imageVertices->at(3) = osg::Vec3(imageXPosition, imageYPosition + imageHeight, 0);

    text->setPosition(osg::Vec3(imageXPosition, imageYPosition + imageHeight + 5, 0));

    backgroundVertices->at(0) = osg::Vec3(x, y, -1);
    backgroundVertices->at(1) = osg::Vec3(x + width, y, -1);
    backgroundVertices->at(2) = osg::Vec3(x + width, y + height, -1);
    backgroundVertices->at(3) = osg::Vec3(x, y + height, -1);
}
