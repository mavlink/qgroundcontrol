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
    : state(WebImage::UNINITIALIZED)
    , sourceURL("")
    , image(0)
    , lastReference(0)
    , _is3D(false)
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
    heightModel.clear();
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
WebImage::getImageData(void) const
{
    return image->scanLine(0);
}

const QVector< QVector<int32_t> >&
WebImage::getHeightModel(void) const
{
    return heightModel;
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

bool
WebImage::setData(const QString& imageFilename, const QString& heightFilename)
{
    QFile heightFile(heightFilename);

    QImage tempImage;
    if (tempImage.load(imageFilename) && heightFile.open(QIODevice::ReadOnly))
    {
        if (image.isNull())
        {
            image.reset(new QImage);
        }
        *image = QGLWidget::convertToGLFormat(tempImage);

        QDataStream heightDataStream(&heightFile);

        // read in width and height values for height map
        char header[8];
        heightDataStream.readRawData(header, 8);

        int32_t height = *(reinterpret_cast<int32_t *>(header));
        int32_t width = *(reinterpret_cast<int32_t *>(header + 4));

        char buffer[height * width * sizeof(int32_t)];
        heightDataStream.readRawData(buffer, height * width * sizeof(int32_t));

        heightModel.clear();
        for (int32_t i = 0; i < height; ++i)
        {
            QVector<int32_t> scanline;
            for (int32_t j = 0; j < width; ++j)
            {    
                int32_t n = *(reinterpret_cast<int32_t *>(buffer
                                                          + (i * height + j)
                                                          * sizeof(int32_t)));
                scanline.push_back(n);
            }
            heightModel.push_back(scanline);
        }

        heightFile.close();

        _is3D = true;

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

bool
WebImage::is3D(void) const
{
    return _is3D;
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
