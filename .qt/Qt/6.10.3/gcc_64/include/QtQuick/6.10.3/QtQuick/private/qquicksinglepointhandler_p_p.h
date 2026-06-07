// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKPOINTERSINGLEHANDLER_P_H
#define QQUICKPOINTERSINGLEHANDLER_P_H

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
#include "qquicksinglepointhandler_p.h"

QT_BEGIN_NAMESPACE

class Q_QUICK_EXPORT QQuickSinglePointHandlerPrivate : public QQuickPointerDeviceHandlerPrivate
{
    Q_DECLARE_PUBLIC(QQuickSinglePointHandler)

public:
    static QQuickSinglePointHandlerPrivate* get(QQuickSinglePointHandler *q) { return q->d_func(); }
    static const QQuickSinglePointHandlerPrivate* get(const QQuickSinglePointHandler *q) { return q->d_func(); }

    QQuickSinglePointHandlerPrivate();

    void reset();

    QQuickHandlerPoint pointInfo;
    bool ignoreAdditionalPoints = false;
};

QT_END_NAMESPACE

#endif // QQUICKPOINTERSINGLEHANDLER_P_H

