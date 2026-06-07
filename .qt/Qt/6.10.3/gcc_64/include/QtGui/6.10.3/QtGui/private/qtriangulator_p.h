// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QTRIANGULATOR_P_H
#define QTRIANGULATOR_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtGui/private/qtguiglobal_p.h>
#include <QtGui/private/qvectorpath_p.h>
#include <QtCore/qlist.h>

QT_BEGIN_NAMESPACE

class QVertexIndexVector
{
public:
    enum Type {
        UnsignedInt,
        UnsignedShort
    };

    inline Type type() const { return t; }

    inline void setDataUint(const QList<quint32> &data)
    {
        t = UnsignedInt;
        indices32 = data;
    }

    inline void setDataUshort(const QList<quint16> &data)
    {
        t = UnsignedShort;
        indices16 = data;
    }

    inline const void* data() const
    {
        if (t == UnsignedInt)
            return indices32.data();
        return indices16.data();
    }

    inline int size() const
    {
        if (t == UnsignedInt)
            return indices32.size();
        return indices16.size();
    }

private:

    Type t;
    QList<quint32> indices32;
    QList<quint16> indices16;
};

struct QTriangleSet
{
    // The vertices of a triangle are given by: (x[i[n]], y[i[n]]), (x[j[n]], y[j[n]]), (x[k[n]], y[k[n]]), n = 0, 1, ...
    QList<qreal> vertices; // [x[0], y[0], x[1], y[1], x[2], ...]
    QVertexIndexVector indices; // [i[0], j[0], k[0], i[1], j[1], k[1], i[2], ...]
};

struct QPolylineSet
{
    QList<qreal> vertices; // [x[0], y[0], x[1], y[1], x[2], ...]
    QVertexIndexVector indices; // End of polyline is marked with -1.
};

// The vertex coordinates of the returned triangle set will be rounded to a grid with a mesh size
// of 1/32. The polygon is first transformed, then scaled by 32, the coordinates are rounded to
// integers, the polygon is triangulated, and then scaled back by 1/32.
// 'hint' should be a combination of QVectorPath::Hints.
// 'lod' is the level of detail. Default is 1. Curves are split into more lines when 'lod' is higher.
QTriangleSet Q_GUI_EXPORT qTriangulate(const qreal *polygon, int count,
                                       uint hint = QVectorPath::PolygonHint | QVectorPath::OddEvenFill,
                                       const QTransform &matrix = QTransform(),
                                       bool allowUintIndices = true);
QTriangleSet Q_GUI_EXPORT qTriangulate(const QVectorPath &path, const QTransform &matrix = QTransform(),
                                       qreal lod = 1, bool allowUintIndices = true);
QTriangleSet Q_GUI_EXPORT qTriangulate(const QPainterPath &path, const QTransform &matrix = QTransform(),
                                       qreal lod = 1, bool allowUintIndices = true);
QPolylineSet qPolyline(const QVectorPath &path, const QTransform &matrix = QTransform(),
                       qreal lod = 1, bool allowUintIndices = true);
QPolylineSet Q_GUI_EXPORT qPolyline(const QPainterPath &path, const QTransform &matrix = QTransform(),
                                    qreal lod = 1, bool allowUintIndices = true);

QT_END_NAMESPACE

#endif
