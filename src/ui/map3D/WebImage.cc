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

#include <QFile>
#include <QGLWidget>

WebImage::WebImage()
 : mState(WebImage::UNINITIALIZED)
 , mSourceURL("")
 , mImage(0)
 , mLastReference(0)
 , mSyncFlag(false)
{

}

void
WebImage::clear(void)
{
    mImage.reset();
    mSourceURL.clear();
    mState = WebImage::UNINITIALIZED;
    mLastReference = 0;
}

WebImage::State
WebImage::getState(void) const
{
    return mState;
}

void
WebImage::setState(State state)
{
    mState = state;
}

const QString&
WebImage::getSourceURL(void) const
{
    return mSourceURL;
}

void
WebImage::setSourceURL(const QString& url)
{
    mSourceURL = url;
}

uchar*
WebImage::getImageData(void) const
{
    return mImage->scanLine(0);
}

bool
WebImage::setData(const QByteArray& data)
{
    QImage tempImage;
    if (tempImage.loadFromData(data))
    {
        if (mImage.isNull())
        {
            mImage.reset(new QImage);
        }
        *mImage = QGLWidget::convertToGLFormat(tempImage);

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
        if (mImage.isNull())
        {
            mImage.reset(new QImage);
        }
        *mImage = QGLWidget::convertToGLFormat(tempImage);

        return true;
    }
    else
    {
        return false;
    }
}

int
WebImage::getWidth(void) const
{
    return mImage->width();
}

int
WebImage::getHeight(void) const
{
    return mImage->height();
}

int
WebImage::getByteCount(void) const
{
    return mImage->byteCount();
}

quint64
WebImage::getLastReference(void) const
{
    return mLastReference;
}

void
WebImage::setLastReference(quint64 value)
{
    mLastReference = value;
}

bool
WebImage::getSyncFlag(void) const
{
    return mSyncFlag;
}

void
WebImage::setSyncFlag(bool onoff)
{
    mSyncFlag = onoff;
}
