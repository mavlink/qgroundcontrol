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

#include "bufferformat.h"
#include <QByteArray>

BufferFormat BufferFormat::fromCaps(GstCaps *caps)
{
    BufferFormat result;
    if (caps && gst_video_info_from_caps(&(result.d->videoInfo), caps)) {
        return result;
    } else {
        return BufferFormat();
    }
}

GstCaps* BufferFormat::newCaps(GstVideoFormat format, const QSize & size,
                               const Fraction & framerate, const Fraction & pixelAspectRatio)
{
    GstVideoInfo videoInfo;
    gst_video_info_init(&videoInfo);
    gst_video_info_set_format(&videoInfo, format, size.width(), size.height());

    videoInfo.fps_n = framerate.numerator;
    videoInfo.fps_d = framerate.denominator;

    videoInfo.par_n = pixelAspectRatio.numerator;
    videoInfo.par_d = pixelAspectRatio.denominator;

    return gst_video_info_to_caps(&videoInfo);
}

int BufferFormat::bytesPerLine(int component) const
{
    return GST_VIDEO_INFO_PLANE_STRIDE(&(d->videoInfo), component);
}

bool operator==(BufferFormat a, BufferFormat b)
{
    return a.d == b.d;
}

bool operator!=(BufferFormat a, BufferFormat b)
{
    return a.d != b.d;
}
