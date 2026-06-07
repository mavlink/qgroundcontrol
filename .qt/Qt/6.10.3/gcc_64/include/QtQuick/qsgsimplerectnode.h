// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSGSIMPLERECTNODE_H
#define QSGSIMPLERECTNODE_H

#include <QtQuick/qsgnode.h>
#include <QtQuick/qsgflatcolormaterial.h>

QT_BEGIN_NAMESPACE

class Q_QUICK_EXPORT QSGSimpleRectNode : public QSGGeometryNode
{
public:
    QSGSimpleRectNode(const QRectF &rect, const QColor &color);
    QSGSimpleRectNode();

    void setRect(const QRectF &rect);
    inline void setRect(qreal x, qreal y, qreal w, qreal h) { setRect(QRectF(x, y, w, h)); }
    QRectF rect() const;

    void setColor(const QColor &color);
    QColor color() const;

private:
    QSGFlatColorMaterial m_material;
    QSGGeometry m_geometry;
    void *reserved;
};

QT_END_NAMESPACE

#endif // SOLIDRECTNODE_H
