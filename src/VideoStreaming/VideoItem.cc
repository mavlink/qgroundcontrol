/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/**
 * @file
 *   @brief QGC Video Item
 *   @author Gus Grubba <mavlink@grubba.com>
 */

#include <QtCore/QPointer>
#include <QtQuick/QSGNode>
#include <QtQuick/QSGFlatColorMaterial>

#include "VideoItem.h"
#if defined(QGC_GST_STREAMING)
#include "VideoSurface_p.h"
#endif

#if defined(QGC_GST_STREAMING)
struct VideoItem::Private
{
    QPointer<VideoSurface> surface;
    bool surfaceDirty;
    QRectF targetArea;
};
#endif

VideoItem::VideoItem(QQuickItem *parent)
    : QQuickItem(parent)
#if defined(QGC_GST_STREAMING)
    , _data(new Private)
#endif
{
#if defined(QGC_GST_STREAMING)
    _data->surfaceDirty = true;
    setFlag(QQuickItem::ItemHasContents, true);
#endif
}

VideoItem::~VideoItem()
{
#if defined(QGC_GST_STREAMING)
    setSurface(0);
    delete _data;
#endif
}

VideoSurface *VideoItem::surface() const
{
#if defined(QGC_GST_STREAMING)
    return _data->surface.data();
#else
    return NULL;
#endif
}

void VideoItem::setSurface(VideoSurface *surface)
{
#if defined(QGC_GST_STREAMING)
    if (_data->surface) {
        _data->surface.data()->_data->items.remove(this);
    }
    _data->surface = surface;
    _data->surfaceDirty = true;
    if (_data->surface) {
        _data->surface.data()->_data->items.insert(this);
    }
#else
    Q_UNUSED(surface)
#endif
}

#if defined(QGC_GST_STREAMING)
QSGNode* VideoItem::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData*)
{
    QRectF r = boundingRect();
    QSGNode* newNode = 0;

    if (_data->surfaceDirty) {
        delete oldNode;
        oldNode = 0;
        _data->surfaceDirty = false;
    }

    if (!_data->surface || _data->surface.data()->_data->videoSink == NULL) {
        if (!oldNode) {
            QSGFlatColorMaterial *material = new QSGFlatColorMaterial;
            material->setColor(Qt::black);
            QSGGeometryNode *node = new QSGGeometryNode;
            node->setMaterial(material);
            node->setFlag(QSGNode::OwnsMaterial);
            node->setFlag(QSGNode::OwnsGeometry);
            newNode = node;
            _data->targetArea = QRectF(); //force geometry to be set
        } else {
            newNode = oldNode;
        }
        if (r != _data->targetArea) {
            QSGGeometry *geometry = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), 4);
            geometry->vertexDataAsPoint2D()[0].set(r.x(), r.y());
            geometry->vertexDataAsPoint2D()[1].set(r.x(), r.height());
            geometry->vertexDataAsPoint2D()[2].set(r.width(), r.y());
            geometry->vertexDataAsPoint2D()[3].set(r.width(), r.height());
            QSGGeometryNode *node = static_cast<QSGGeometryNode*>(newNode);
            node->setGeometry(geometry);
            _data->targetArea = r;
        }
    } else {
        g_signal_emit_by_name(_data->surface.data()->_data->videoSink, "update-node", (void*)oldNode, r.x(), r.y(), r.width(), r.height(), (void**)&newNode);
    }

    return newNode;
}
#endif
