// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSGCURVEPROCESSOR_P_H
#define QSGCURVEPROCESSOR_P_H

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

#include <QtQuick/qtquickexports.h>
#include <QtQuick/private/qquadpath_p.h>
#include "qsgcurvestrokenode_p.h"

#include <QtCore/qloggingcategory.h>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(lcSGCurveProcessor);

class Q_QUICK_EXPORT QSGCurveProcessor
{
public:
    typedef std::function<QVector3D(QVector2D)> uvForPointCallback;
    typedef std::function<void(const std::array<QVector2D, 3> &,
                               const std::array<QVector2D, 3> &,
                               uvForPointCallback)> addTriangleCallback;
    typedef std::function<void(const std::array<QVector2D, 3> &,
                               const std::array<QVector2D, 3> &,
                               const std::array<QVector2D, 3> &,
                               QSGCurveStrokeNode::TriangleFlags)> addStrokeTriangleCallback;

    static void processFill(const QQuadPath &path,
                            Qt::FillRule fillRule,
                            addTriangleCallback addTriangle);
    static void processStroke(const QQuadPath &strokePath,
                              float miterLimit,
                              float penWidth,
                              Qt::PenJoinStyle joinStyle,
                              Qt::PenCapStyle capStyle,
                              addStrokeTriangleCallback addTriangle,
                              int subdivisions = 3);
    static bool solveOverlaps(QQuadPath &path);
    static QList<std::pair<int, int>> findOverlappingCandidates(const QQuadPath &path);
    static bool removeNestedSubpaths(QQuadPath &path);
    static bool solveIntersections(QQuadPath &path, bool removeNestedPaths = true);
};

QT_END_NAMESPACE

#endif // QSGCURVEPROCESSOR_P_H
