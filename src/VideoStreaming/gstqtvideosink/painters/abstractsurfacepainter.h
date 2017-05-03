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

#ifndef ABSTRACTSURFACEPAINTER_H
#define ABSTRACTSURFACEPAINTER_H

#include "../utils/bufferformat.h"
#include <QRectF>

class QPainter;

/** Common interface for all the painters */
class AbstractSurfacePainter
{
public:
    virtual ~AbstractSurfacePainter() {}

    virtual bool supportsFormat(GstVideoFormat format) const = 0;

    virtual void init(const BufferFormat & format) = 0;
    virtual void cleanup() = 0;

    virtual void paint(quint8 *data, const BufferFormat & frameFormat,
                       QPainter *painter, const PaintAreas & areas) = 0;

    virtual void updateColors(int brightness, int contrast, int hue, int saturation) = 0;
};

#endif
