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
 *   @brief Definition of the class WebImage.
 *
 *   @author Lionel Heng <hengli@student.ethz.ch>
 *
 */

#include "WebImage.h"

#include <QGLWidget>

WebImage::WebImage()
    : state(WebImage::UNINITIALIZED)
    , sourceURL("")
    , image(0)
    , lastReference(0)
    , syncFlag(false)
{

}

void
WebImage::clear(void)
{
    image.reset();
    sourceURL.clear();
    state = WebImage::UNINITIALIZED;
    lastReference = 0;
}

WebImage::State
WebImage::getState(void) const
{
    return state;
}

void
WebImage::setState(State state)
{
    this->state = state;
}

const QString&
WebImage::getSourceURL(void) const
{
    return sourceURL;
}

void
WebImage::setSourceURL(const QString& url)
{
    sourceURL = url;
}

const uint8_t*
WebImage::getData(void) const
{
    return image->scanLine(0);
}

bool
WebImage::setData(const QByteArray& data)
{
    QImage tempImage;
    if (tempImage.loadFromData(data))
    {
        if (image.isNull())
        {
            image.reset(new QImage);
        }
        *image = QGLWidget::convertToGLFormat(tempImage);

        return true;
    }
    else
    {
        return false;
    }
}

bool
WebImage::setData(const QString& filename)
{
    QImage tempImage;
    if (tempImage.load(filename))
    {
        if (image.isNull())
        {
            image.reset(new QImage);
        }
        *image = QGLWidget::convertToGLFormat(tempImage);

        return true;
    }
    else
    {
        return false;
    }
}

int32_t
WebImage::getWidth(void) const
{
    return image->width();
}

int32_t
WebImage::getHeight(void) const
{
    return image->height();
}

int32_t
WebImage::getByteCount(void) const
{
    return image->byteCount();
}

uint64_t
WebImage::getLastReference(void) const
{
    return lastReference;
}

void
WebImage::setLastReference(uint64_t value)
{
    lastReference = value;
}

bool
WebImage::getSyncFlag(void) const
{
    return syncFlag;
}

void
WebImage::setSyncFlag(bool onoff)
{
    syncFlag = onoff;
}
