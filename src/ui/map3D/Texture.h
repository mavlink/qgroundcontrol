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

#ifndef TEXTURE_H
#define TEXTURE_H

#include <inttypes.h>
#include <osg/ref_ptr>
#include <osg/Geometry>
#include <osg/Texture2D>
#include <QSharedPointer>

#include "WebImage.h"

class Texture
{
public:
    explicit Texture(quint64 id);

    const QString& getSourceURL(void) const;

    void setId(quint64 id);

    void sync(const WebImagePtr& image);

    osg::ref_ptr<osg::Geometry> draw(double x1, double y1, double x2, double y2,
                                     double z,
                                     bool smoothInterpolation) const;
    osg::ref_ptr<osg::Geometry> draw(double x1, double y1, double x2, double y2,
                                     double x3, double y3, double x4, double y4,
                                     double z,
                                     bool smoothInterpolation) const;

private:
    enum State
    {
        UNINITIALIZED = 0,
        REQUESTED = 1,
        READY = 2
    };

    State mState;
    QString mSourceURL;
    quint64 mId;
    osg::ref_ptr<osg::Texture2D> mTexture2D;
    osg::ref_ptr<osg::Geometry> mGeometry;
};

typedef QSharedPointer<Texture> TexturePtr;

#endif // TEXTURE_H
