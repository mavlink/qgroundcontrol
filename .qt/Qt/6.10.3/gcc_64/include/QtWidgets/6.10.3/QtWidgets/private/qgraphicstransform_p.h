// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QGRAPHICSTRANSFORM_P_H
#define QGRAPHICSTRANSFORM_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of other Qt classes.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtWidgets/private/qtwidgetsglobal_p.h>
#include "private/qobject_p.h"

QT_REQUIRE_CONFIG(graphicsview);

QT_BEGIN_NAMESPACE

class QGraphicsItem;

class QGraphicsTransformPrivate : public QObjectPrivate {
public:
    Q_DECLARE_PUBLIC(QGraphicsTransform)

    QGraphicsTransformPrivate()
        : QObjectPrivate(), item(nullptr) {}
    ~QGraphicsTransformPrivate();

    QGraphicsItem *item;

    void setItem(QGraphicsItem *item);
    static void updateItem(QGraphicsItem *item);
};

QT_END_NAMESPACE

#endif // QGRAPHICSTRANSFORM_P_H
