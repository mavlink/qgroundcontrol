/*
    Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
    Copyright (C) 2013 basysKom GmbH <info@basyskom.com>
    Copyright (C) 2013 Collabora Ltd. <info@collabora.com>

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

#include "videonode.h"
#include "videomaterial.h"

#include <QtQuick/QSGFlatColorMaterial>

VideoNode::VideoNode()
  : QSGGeometryNode()
{
    setFlags(OwnsGeometry | OwnsMaterial, true);
    setMaterialTypeSolidBlack();
}

void VideoNode::changeFormat(const BufferFormat & format)
{
    setMaterial(VideoMaterial::create(format));
    setGeometry(0);
    m_materialType = MaterialTypeVideo;
}

void VideoNode::setMaterialTypeSolidBlack()
{
    QSGFlatColorMaterial *m = new QSGFlatColorMaterial;
    m->setColor(Qt::black);
    setMaterial(m);
    setGeometry(0);
    m_materialType = MaterialTypeSolidBlack;
}

void VideoNode::setCurrentFrame(GstBuffer* buffer)
{
    Q_ASSERT (m_materialType == MaterialTypeVideo);
    static_cast<VideoMaterial*>(material())->setCurrentFrame(buffer);
    markDirty(DirtyMaterial);
}

void VideoNode::updateColors(int brightness, int contrast, int hue, int saturation)
{
    Q_ASSERT (m_materialType == MaterialTypeVideo);
    static_cast<VideoMaterial*>(material())->updateColors(brightness, contrast, hue, saturation);
    markDirty(DirtyMaterial);
}

/* Helpers */
template <typename V>
static inline void setGeom(V *v, const QPointF &p)
{
    v->x = p.x();
    v->y = p.y();
}

static inline void setTex(QSGGeometry::TexturedPoint2D *v, const QPointF &p)
{
    v->tx = p.x();
    v->ty = p.y();
}

void VideoNode::updateGeometry(const PaintAreas & areas)
{
    QSGGeometry *g = geometry();

    if (m_materialType == MaterialTypeVideo) {
        if (!g)
            g = new QSGGeometry(QSGGeometry::defaultAttributes_TexturedPoint2D(), 4);

        QSGGeometry::TexturedPoint2D *v = g->vertexDataAsTexturedPoint2D();

        // Set geometry first
        setGeom(v + 0, areas.videoArea.topLeft());
        setGeom(v + 1, areas.videoArea.bottomLeft());
        setGeom(v + 2, areas.videoArea.topRight());
        setGeom(v + 3, areas.videoArea.bottomRight());

        // and then texture coordinates
        setTex(v + 0, areas.sourceRect.topLeft());
        setTex(v + 1, areas.sourceRect.bottomLeft());
        setTex(v + 2, areas.sourceRect.topRight());
        setTex(v + 3, areas.sourceRect.bottomRight());
    } else {
        if (!g)
            g = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), 4);

        QSGGeometry::Point2D *v = g->vertexDataAsPoint2D();

        setGeom(v + 0, areas.videoArea.topLeft());
        setGeom(v + 1, areas.videoArea.bottomLeft());
        setGeom(v + 2, areas.videoArea.topRight());
        setGeom(v + 3, areas.videoArea.bottomRight());
    }

    if (!geometry())
        setGeometry(g);

    markDirty(DirtyGeometry);
}
