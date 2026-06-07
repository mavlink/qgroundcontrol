// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSGRECTANGLENODE_H
#define QSGRECTANGLENODE_H

#include <QtQuick/qsgnode.h>

QT_BEGIN_NAMESPACE

class Q_QUICK_EXPORT QSGRectangleNode : public QSGGeometryNode
{
public:
    ~QSGRectangleNode() override = default;

    virtual void setRect(const QRectF &rect) = 0;
    inline void setRect(qreal x, qreal y, qreal w, qreal h) { setRect(QRectF(x, y, w, h)); }
    virtual QRectF rect() const = 0;

    virtual void setColor(const QColor &color) = 0;
    virtual QColor color() const = 0;
};

QT_END_NAMESPACE

#endif // QSGRECTANGLENODE_H
