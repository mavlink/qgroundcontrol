/*=====================================================================

QGroundControl Open Source Ground Control Station

(c) 2009, 2015 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

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
 *   @brief QGC Video Item
 *   @author Gus Grubba <mavlink@grubba.com>
 */

#ifndef VIDEO_ITEM_H
#define VIDEO_ITEM_H

#include <QtQuick/QQuickItem>
#include "VideoSurface.h"

class VideoItem : public QQuickItem
{
    Q_OBJECT
    Q_DISABLE_COPY(VideoItem)
    Q_PROPERTY(VideoSurface* surface READ surface WRITE setSurface)

public:
    explicit VideoItem(QQuickItem *parent = 0);
    virtual ~VideoItem();

    VideoSurface *surface() const;
    void setSurface(VideoSurface *surface);

protected:
#if defined(QGC_GST_STREAMING)
    /*! Reimplemented from QQuickItem. */
    virtual QSGNode* updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *updatePaintNodeData);
#endif

private:
#if defined(QGC_GST_STREAMING)
    struct Private;
    Private* const _data;
#endif
};

#endif // VIDEO_ITEM_H
