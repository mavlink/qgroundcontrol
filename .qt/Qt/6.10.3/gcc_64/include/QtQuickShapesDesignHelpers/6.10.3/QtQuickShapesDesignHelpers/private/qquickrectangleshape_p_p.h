// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQUICKRECTANGLESHAPE_P_P_H
#define QQUICKRECTANGLESHAPE_P_P_H

#include <QtQuickShapesDesignHelpers/private/qquickrectangleshape_p.h>

#include <QtQml/private/qqmlpropertyutils_p.h>
#include <QtQuickShapes/private/qquickshape_p_p.h>

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

QT_BEGIN_NAMESPACE

class Q_QUICKSHAPESDESIGNHELPERS_EXPORT QQuickRectangleShapePrivate : public QQuickShapePrivate
{
    Q_DECLARE_PUBLIC(QQuickRectangleShape)

public:
    QQuickRectangleShapePrivate() = default;

    static QQuickRectangleShapePrivate *get(QQuickRectangleShape *p) { return p->d_func(); }

    void updateStrokeAdjustment();

    QQuickShapePath *shapePath = nullptr;
    QQuickPathRectangle *pathRectangle = nullptr;

    qreal borderOffset = 0;
    qreal borderRadiusAdjustment = 0;
    QQuickRectangleShape::BorderMode borderMode = QQuickRectangleShape::Inside;

};

QT_END_NAMESPACE

#endif // QQUICKRECTANGLESHAPE_P_P_H
