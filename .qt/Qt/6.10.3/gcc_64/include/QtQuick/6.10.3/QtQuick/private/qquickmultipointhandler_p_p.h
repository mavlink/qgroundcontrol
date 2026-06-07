// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKPOINTERMULTIHANDLER_P_H
#define QQUICKPOINTERMULTIHANDLER_P_H

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

#include "qquickhandlerpoint_p.h"
#include "qquickpointerdevicehandler_p_p.h"
#include "qquickmultipointhandler_p.h"

QT_BEGIN_NAMESPACE

class Q_QUICK_EXPORT QQuickMultiPointHandlerPrivate : public QQuickPointerDeviceHandlerPrivate
{
    Q_DECLARE_PUBLIC(QQuickMultiPointHandler)

public:
    static QQuickMultiPointHandlerPrivate* get(QQuickMultiPointHandler *q) { return q->d_func(); }
    static const QQuickMultiPointHandlerPrivate* get(const QQuickMultiPointHandler *q) { return q->d_func(); }

    QQuickMultiPointHandlerPrivate(int minPointCount, int maxPointCount);

    QMetaProperty &xMetaProperty() const;
    QMetaProperty &yMetaProperty() const;

    QVector<QQuickHandlerPoint> currentPoints;
    QQuickHandlerPoint centroid;
    int minimumPointCount;
    int maximumPointCount;
    mutable QMetaProperty xProperty;
    mutable QMetaProperty yProperty;
};

QT_END_NAMESPACE

#endif // QQUICKPOINTERMULTIHANDLER_P_H
