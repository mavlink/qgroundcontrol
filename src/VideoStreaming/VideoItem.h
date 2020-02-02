/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/**
 * @file
 *   @brief QGC Video Item
 *   @author Gus Grubba <gus@auterion.com>
 */

#pragma once

#include <QtQuick/QQuickItem>
#include "VideoSurface.h"

class QSGGeometry;

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
	QSGGeometry* _createDefaultGeometry(QRectF& rectBound);
	
    struct Private;
    Private* const _data;
#endif
};

