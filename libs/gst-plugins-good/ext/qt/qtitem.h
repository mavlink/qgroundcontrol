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

#ifndef __QT_ITEM_H__
#define __QT_ITEM_H__

#include <gst/gst.h>
#include <gst/gl/gl.h>

#include "gstqtgl.h"
#include <QtCore/QMutex>
#include <QtQuick/QQuickItem>
#include <QtGui/QOpenGLContext>
#include <QtGui/QOpenGLFunctions>

typedef struct _QtGLVideoItemPrivate QtGLVideoItemPrivate;

class QtGLVideoItem;

class QtGLVideoItemInterface : public QObject
{
    Q_OBJECT
public:
    QtGLVideoItemInterface (QtGLVideoItem *w) : qt_item (w), lock() {};

    void invalidateRef();

    void setBuffer (GstBuffer * buffer);
    gboolean setCaps (GstCaps *caps);
    gboolean initWinSys ();
    GstGLContext *getQtContext();
    GstGLContext *getContext();
    GstGLDisplay *getDisplay();
    QtGLVideoItem *videoItem () { return qt_item; };

    void setDAR(gint, gint);
    void getDAR(gint *, gint *);
    void setForceAspectRatio(bool);
    bool getForceAspectRatio();
private:
    QtGLVideoItem *qt_item;
    QMutex lock;
};

class InitializeSceneGraph;

class QtGLVideoItem : public QQuickItem, protected QOpenGLFunctions
{
    Q_OBJECT

    Q_PROPERTY(bool itemInitialized
               READ itemInitialized
               NOTIFY itemInitializedChanged)

public:
    QtGLVideoItem();
    ~QtGLVideoItem();

    void setDAR(gint, gint);
    void getDAR(gint *, gint *);
    void setForceAspectRatio(bool);
    bool getForceAspectRatio();
    bool itemInitialized();

    QSharedPointer<QtGLVideoItemInterface> getInterface() { return proxy; };
    /* private for C interface ... */
    QtGLVideoItemPrivate *priv;

Q_SIGNALS:
    void itemInitializedChanged();

private Q_SLOTS:
    void handleWindowChanged(QQuickWindow * win);
    void onSceneGraphInitialized();
    void onSceneGraphInvalidated();

protected:
    QSGNode * updatePaintNode (QSGNode * oldNode, UpdatePaintNodeData * updatePaintNodeData);

private:

    friend class InitializeSceneGraph;
    void setViewportSize(const QSize &size);
    void shareContext();

    QSize m_viewportSize;
    bool m_openGlContextInitialized;

    QSharedPointer<QtGLVideoItemInterface> proxy;
};

#endif /* __QT_ITEM_H__ */
