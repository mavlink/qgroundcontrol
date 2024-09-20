/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

/*
 * GStreamer
 * Copyright (C) 2015 Matthew Waters <matthew@centricular.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef __QT6_GL_ITEM_H__
#define __QT6_GL_ITEM_H__

#include <gst/gst.h>
#include <gst/gl/gl.h>

#include "gstqt6gl.h"
#include <QtCore/QMutex>
#include <QtQuick/QQuickItem>
#include <QtGui/QOpenGLContext>
#include <QtGui/QOpenGLFunctions>
#include <QtQuick/QQuickWindow>

typedef struct _Qt6GLVideoItemPrivate Qt6GLVideoItemPrivate;

class Qt6GLVideoItem;

class Qt6GLVideoItemInterface : public QObject
{
    Q_OBJECT
    QML_ELEMENT
public:
    Qt6GLVideoItemInterface (Qt6GLVideoItem *w) : qt_item (w), lock() {};

    void invalidateRef();

    void setSink (GstElement * sink);
    void setBuffer (GstBuffer * buffer);
    gboolean setCaps (GstCaps *caps);
    gboolean initWinSys ();
    GstGLContext *getQtContext();
    GstGLContext *getContext();
    GstGLDisplay *getDisplay();
    Qt6GLVideoItem *videoItem () { return qt_item; };

    void setDAR(gint, gint);
    void getDAR(gint *, gint *);
    void setForceAspectRatio(bool);
    bool getForceAspectRatio();
private:
    Qt6GLVideoItem *qt_item;
    QMutex lock;
};

class Qt6GLVideoItem : public QQuickItem, protected QOpenGLFunctions
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(bool itemInitialized
               READ itemInitialized
               NOTIFY itemInitializedChanged)
    Q_PROPERTY(bool forceAspectRatio
               READ getForceAspectRatio
               WRITE setForceAspectRatio
               NOTIFY forceAspectRatioChanged)

public:
    Qt6GLVideoItem();
    ~Qt6GLVideoItem();

    void setDAR(gint, gint);
    void getDAR(gint *, gint *);
    void setForceAspectRatio(bool);
    bool getForceAspectRatio();
    bool itemInitialized();

    QSharedPointer<Qt6GLVideoItemInterface> getInterface() { return proxy; };
    /* private for C interface ... */
    Qt6GLVideoItemPrivate *priv;

Q_SIGNALS:
    void itemInitializedChanged();
    void forceAspectRatioChanged(bool);

private Q_SLOTS:
    void handleWindowChanged(QQuickWindow * win);
    void onSceneGraphInitialized();
    void onSceneGraphInvalidated();

protected:
    QSGNode * updatePaintNode (QSGNode * oldNode, UpdatePaintNodeData * updatePaintNodeData) override;
    void releaseResources() override;
    void wheelEvent(QWheelEvent *) override;
    void hoverEnterEvent(QHoverEvent *) override;
    void hoverLeaveEvent (QHoverEvent *) override;
    void hoverMoveEvent (QHoverEvent *) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void touchEvent(QTouchEvent*) override;

private:

    void setViewportSize(const QSize &size);
    void shareContext();

    void fitStreamToAllocatedSize(GstVideoRectangle * result);
    QPointF mapPointToStreamSize(QPointF);

    void sendMouseEvent(QMouseEvent * event, gboolean is_press);

    quint32 mousePressedButton;
    bool mouseHovering;

    QSharedPointer<Qt6GLVideoItemInterface> proxy;
};

#endif /* __QT_GL_ITEM_H__ */
