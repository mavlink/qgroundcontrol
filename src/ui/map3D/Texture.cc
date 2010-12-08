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
 *   @brief Definition of the class Texture.
 *
 *   @author Lionel Heng <hengli@student.ethz.ch>
 *
 */

#include <osg/LineWidth>

#include "Texture.h"

Texture::Texture(unsigned int _id)
    : id(_id)
    , texture2D(new osg::Texture2D)
    , geometry(new osg::Geometry)
{
    texture2D->setFilter(osg::Texture::MIN_FILTER, osg::Texture::NEAREST);
    texture2D->setFilter(osg::Texture::MAG_FILTER, osg::Texture::NEAREST);

    texture2D->setDataVariance(osg::Object::DYNAMIC);
    texture2D->setResizeNonPowerOfTwoHint(false);

    osg::ref_ptr<osg::Image> image = new osg::Image;
    texture2D->setImage(image);

    osg::ref_ptr<osg::Vec2Array> vertices(new osg::Vec2Array(4));
    geometry->setVertexArray(vertices);

    osg::ref_ptr<osg::Vec2Array> textureCoords = new osg::Vec2Array;
    textureCoords->push_back(osg::Vec2(0.0f, 1.0f));
    textureCoords->push_back(osg::Vec2(1.0f, 1.0f));
    textureCoords->push_back(osg::Vec2(1.0f, 0.0f));
    textureCoords->push_back(osg::Vec2(0.0f, 0.0f));
    geometry->setTexCoordArray(id, textureCoords);

    geometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::LINES,
                                                  0, 4));

    osg::ref_ptr<osg::Vec4Array> colors(new osg::Vec4Array);
    colors->push_back(osg::Vec4(0.0f, 0.0f, 1.0f, 1.0f));
    geometry->setColorArray(colors);
    geometry->setColorBinding(osg::Geometry::BIND_OVERALL);

    geometry->setUseDisplayList(false);
}

const QString&
Texture::getSourceURL(void) const
{
    return sourceURL;
}

void
Texture::sync(const WebImagePtr& image)
{
    state = static_cast<State>(image->getState());

    if (image->getState() != WebImage::UNINITIALIZED &&
        sourceURL != image->getSourceURL())
    {
        sourceURL = image->getSourceURL();
    }

    if (image->getState() == WebImage::READY && image->getSyncFlag())
    {
        image->setSyncFlag(false);

        if (texture2D->getImage() != NULL)
        {
            texture2D->getImage()->setImage(image->getWidth(),
                                            image->getHeight(),
                                            1,
                                            GL_RGB,
                                            GL_RGB,
                                            GL_UNSIGNED_BYTE,
                                            image->getImageData(),
                                            osg::Image::NO_DELETE);
            texture2D->getImage()->dirty();
        }
    }
}

osg::ref_ptr<osg::Geometry>
Texture::draw(float x1, float y1, float x2, float y2,
              bool smoothInterpolation) const
{
    return draw(x1, y1, x2, y1, x2, y2, x1, y2, smoothInterpolation);
}

osg::ref_ptr<osg::Geometry>
Texture::draw(float x1, float y1, float x2, float y2,
              float x3, float y3, float x4, float y4,
              bool smoothInterpolation) const
{
    osg::Vec2Array* vertices =
            static_cast<osg::Vec2Array*>(geometry->getVertexArray());
    (*vertices)[0].set(x1, y1);
    (*vertices)[1].set(x2, y2);
    (*vertices)[2].set(x3, y3);
    (*vertices)[3].set(x4, y4);

    osg::DrawArrays* drawarrays =
            static_cast<osg::DrawArrays*>(geometry->getPrimitiveSet(0));
    osg::Vec4Array* colors =
            static_cast<osg::Vec4Array*>(geometry->getColorArray());

    if (state == REQUESTED)
    {
        drawarrays->set(osg::PrimitiveSet::LINES, 0, 4);
        (*colors)[0].set(0.0f, 0.0f, 1.0f, 1.0f);
        
        geometry->getOrCreateStateSet()->
                setTextureAttributeAndModes(id, texture2D, osg::StateAttribute::OFF);

        return geometry;
    }

    if (smoothInterpolation)
    {
        texture2D->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
        texture2D->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);
    }
    else
    {
        texture2D->setFilter(osg::Texture::MIN_FILTER, osg::Texture::NEAREST);
        texture2D->setFilter(osg::Texture::MAG_FILTER, osg::Texture::NEAREST);
    }

    drawarrays->set(osg::PrimitiveSet::POLYGON, 0, 4);
    (*colors)[0].set(1.0f, 1.0f, 1.0f, 1.0f);

    geometry->getOrCreateStateSet()->
            setTextureAttributeAndModes(id, texture2D, osg::StateAttribute::ON);

    return geometry;
}
