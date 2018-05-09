/*
    Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies). <qt-info@nokia.com>
    Copyright (C) 2011-2012 Collabora Ltd. <info@collabora.com>

    This library is free software; you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License version 2.1
    as published by the Free Software Foundation.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/**
 * @file
 *   @brief Extracted from QtGstreamer to avoid overly complex dependency
 *   @author Gus Grubba <mavlink@grubba.com>
 */

#include "genericsurfacepainter.h"
#include <QPainter>

GenericSurfacePainter::GenericSurfacePainter()
    : m_imageFormat(QImage::Format_Invalid)
{
}

//static
QSet<GstVideoFormat> GenericSurfacePainter::supportedPixelFormats()
{
    return QSet<GstVideoFormat>()
#if Q_BYTE_ORDER == Q_BIG_ENDIAN
        << GST_VIDEO_FORMAT_ARGB
        << GST_VIDEO_FORMAT_xRGB
#else
        << GST_VIDEO_FORMAT_BGRA
        << GST_VIDEO_FORMAT_BGRx
#endif
        << GST_VIDEO_FORMAT_RGB
        << GST_VIDEO_FORMAT_RGB16
        ;
}

void GenericSurfacePainter::init(const BufferFormat &format)
{
    switch (format.videoFormat()) {
    // QImage is shitty and reads integers instead of bytes,
    // thus it is affected by the host's endianness
#if Q_BYTE_ORDER == Q_BIG_ENDIAN
    case GST_VIDEO_FORMAT_ARGB:
#else
    case GST_VIDEO_FORMAT_BGRA:
#endif
        m_imageFormat = QImage::Format_ARGB32;
        break;
#if Q_BYTE_ORDER == Q_BIG_ENDIAN
    case GST_VIDEO_FORMAT_xRGB:
#else
    case GST_VIDEO_FORMAT_BGRx:
#endif
        m_imageFormat = QImage::Format_RGB32;
        break;
    //16-bit RGB formats use host's endianness in GStreamer
    //FIXME-0.11 do endianness checks like above if semantics have changed
    case GST_VIDEO_FORMAT_RGB16:
        m_imageFormat = QImage::Format_RGB16;
        break;
    //This is not affected by endianness
    case GST_VIDEO_FORMAT_RGB:
        m_imageFormat = QImage::Format_RGB888;
        break;
    default:
        throw QString("Unsupported format");
    }
}

void GenericSurfacePainter::cleanup()
{
    m_imageFormat = QImage::Format_Invalid;
}

void GenericSurfacePainter::paint(quint8 *data,
        const BufferFormat & frameFormat,
        QPainter *painter,
        const PaintAreas & areas)
{
    Q_ASSERT(m_imageFormat != QImage::Format_Invalid);

    QImage image(
        data,
        frameFormat.frameSize().width(),
        frameFormat.frameSize().height(),
        frameFormat.bytesPerLine(),
        m_imageFormat);

    QRectF sourceRect = areas.sourceRect;
    sourceRect.setX(sourceRect.x() * frameFormat.frameSize().width());
    sourceRect.setY(sourceRect.y() * frameFormat.frameSize().height());
    sourceRect.setWidth(sourceRect.width() * frameFormat.frameSize().width());
    sourceRect.setHeight(sourceRect.height() * frameFormat.frameSize().height());

    painter->fillRect(areas.blackArea1, Qt::black);
    painter->drawImage(areas.videoArea, image, sourceRect);
    painter->fillRect(areas.blackArea2, Qt::black);
}

void GenericSurfacePainter::updateColors(int, int, int, int)
{
}
