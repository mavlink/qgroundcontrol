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
    return nullptr;
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
QSGGeometry* VideoItem::_createDefaultGeometry(QRectF& rectBound)
{
	QSGGeometry *geometry = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), 4);
	geometry->vertexDataAsPoint2D()[0].set(rectBound.x(), rectBound.y());
	geometry->vertexDataAsPoint2D()[1].set(rectBound.x(), rectBound.height());
	geometry->vertexDataAsPoint2D()[2].set(rectBound.width(), rectBound.y());
	geometry->vertexDataAsPoint2D()[3].set(rectBound.width(), rectBound.height());

	return geometry;
}

QSGNode* VideoItem::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData*)
{
    QRectF r = boundingRect();
    QSGNode* newNode = nullptr;

    if (_data->surfaceDirty) {
        delete oldNode;
        oldNode = nullptr;
        _data->surfaceDirty = false;
    }

    if (!_data->surface || _data->surface.data()->_data->videoSink == nullptr) {
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
            QSGGeometryNode *node = static_cast<QSGGeometryNode*>(newNode);
			node->setGeometry(_createDefaultGeometry(r));
            _data->targetArea = r;
        }
    } else {
        g_signal_emit_by_name(_data->surface.data()->_data->videoSink, "update-node", (void*)oldNode, r.x(), r.y(), r.width(), r.height(), (void**)&newNode);
    }

	// Sometimes we can still end up here with no geometry when gstreamer fails to create it for whatever reason. If that happens it can
	// cause crashes.
	QSGGeometryNode *node = static_cast<QSGGeometryNode*>(newNode);
	if (node->geometry() == nullptr) {
		qDebug() << "Creating default geom";
		node->setGeometry(_createDefaultGeometry(r));
	}

    return newNode;
}
#endif
