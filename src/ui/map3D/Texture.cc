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
 *   @author Lionel Heng <hengli@inf.ethz.ch>
 *
 */

#include <osg/LineWidth>

#include "Texture.h"

Texture::Texture(quint64 id)
 : mId(id)
 , mTexture2D(new osg::Texture2D)
 , mGeometry(new osg::Geometry)
{
    mTexture2D->setFilter(osg::Texture::MIN_FILTER, osg::Texture::NEAREST);
    mTexture2D->setFilter(osg::Texture::MAG_FILTER, osg::Texture::NEAREST);

    mTexture2D->setDataVariance(osg::Object::DYNAMIC);
    mTexture2D->setResizeNonPowerOfTwoHint(false);

    osg::ref_ptr<osg::Image> image = new osg::Image;
    mTexture2D->setImage(image);

    osg::ref_ptr<osg::Vec3dArray> vertices(new osg::Vec3dArray(4));
    mGeometry->setVertexArray(vertices);

    osg::ref_ptr<osg::Vec2Array> textureCoords = new osg::Vec2Array;
    textureCoords->push_back(osg::Vec2(0.0f, 1.0f));
    textureCoords->push_back(osg::Vec2(1.0f, 1.0f));
    textureCoords->push_back(osg::Vec2(1.0f, 0.0f));
    textureCoords->push_back(osg::Vec2(0.0f, 0.0f));
    mGeometry->setTexCoordArray(0, textureCoords);

    mGeometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::LINES,
                              0, 4));

    osg::ref_ptr<osg::Vec4Array> colors(new osg::Vec4Array);
    colors->push_back(osg::Vec4(0.0f, 0.0f, 1.0f, 1.0f));
    mGeometry->setColorArray(colors);
    mGeometry->setColorBinding(osg::Geometry::BIND_OVERALL);

    mGeometry->setUseDisplayList(false);

    osg::ref_ptr<osg::LineWidth> linewidth(new osg::LineWidth);
    linewidth->setWidth(2.0f);
    mGeometry->getOrCreateStateSet()->
    setAttributeAndModes(linewidth, osg::StateAttribute::ON);
    mGeometry->getOrCreateStateSet()->
    setMode(GL_LIGHTING, osg::StateAttribute::OFF);
}

const QString&
Texture::getSourceURL(void) const
{
    return mSourceURL;
}

void
Texture::setId(quint64 id)
{
    mId = id;
}

void
Texture::sync(const WebImagePtr& image)
{
    mState = static_cast<State>(image->getState());

    if (image->getState() != WebImage::UNINITIALIZED &&
        mSourceURL != image->getSourceURL())
    {
        mSourceURL = image->getSourceURL();
    }

    if (image->getState() == WebImage::READY && image->getSyncFlag())
    {
        image->setSyncFlag(false);

        if (mTexture2D->getImage() != NULL)
        {
            mTexture2D->getImage()->setImage(image->getWidth(),
                                             image->getHeight(),
                                             1,
                                             GL_RGBA,
                                             GL_RGBA,
                                             GL_UNSIGNED_BYTE,
                                             image->getImageData(),
                                             osg::Image::NO_DELETE);
            mTexture2D->getImage()->dirty();
        }
    }
}

osg::ref_ptr<osg::Geometry>
Texture::draw(double x1, double y1, double x2, double y2,
              double z,
              bool smoothInterpolation) const
{
    return draw(x1, y1, x2, y1, x2, y2, x1, y2, z, smoothInterpolation);
}

osg::ref_ptr<osg::Geometry>
Texture::draw(double x1, double y1, double x2, double y2,
              double x3, double y3, double x4, double y4,
              double z,
              bool smoothInterpolation) const
{
    osg::Vec3dArray* vertices =
        static_cast<osg::Vec3dArray*>(mGeometry->getVertexArray());
    (*vertices)[0].set(x1, y1, z);
    (*vertices)[1].set(x2, y2, z);
    (*vertices)[2].set(x3, y3, z);
    (*vertices)[3].set(x4, y4, z);

    osg::DrawArrays* drawarrays =
        static_cast<osg::DrawArrays*>(mGeometry->getPrimitiveSet(0));
    osg::Vec4Array* colors =
        static_cast<osg::Vec4Array*>(mGeometry->getColorArray());

    if (mState == REQUESTED)
    {
        drawarrays->set(osg::PrimitiveSet::LINE_LOOP, 0, 4);
        (*colors)[0].set(0.0f, 0.0f, 1.0f, 1.0f);

        mGeometry->getOrCreateStateSet()->
        setTextureAttributeAndModes(0, mTexture2D, osg::StateAttribute::OFF);

        return mGeometry;
    }

    if (smoothInterpolation)
    {
        mTexture2D->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
        mTexture2D->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);
    }
    else
    {
        mTexture2D->setFilter(osg::Texture::MIN_FILTER, osg::Texture::NEAREST);
        mTexture2D->setFilter(osg::Texture::MAG_FILTER, osg::Texture::NEAREST);
    }

    drawarrays->set(osg::PrimitiveSet::POLYGON, 0, 4);
    (*colors)[0].set(1.0f, 1.0f, 1.0f, 1.0f);

    mGeometry->getOrCreateStateSet()->
        setTextureAttributeAndModes(0, mTexture2D, osg::StateAttribute::ON);

    return mGeometry;
}
