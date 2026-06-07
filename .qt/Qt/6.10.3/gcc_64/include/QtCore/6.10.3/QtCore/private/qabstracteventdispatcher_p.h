// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QABSTRACTEVENTDISPATCHER_P_H
#define QABSTRACTEVENTDISPATCHER_P_H

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

#include "QtCore/qabstracteventdispatcher.h"
#include "QtCore/qnamespace.h"
#include "private/qobject_p.h"
#include "QtCore/qttypetraits.h"

QT_BEGIN_NAMESPACE

Q_AUTOTEST_EXPORT qsizetype qGlobalPostedEventsCount();

class Q_CORE_EXPORT QAbstractEventDispatcherPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QAbstractEventDispatcher)
public:
    QAbstractEventDispatcherPrivate();
    ~QAbstractEventDispatcherPrivate() override;

    QList<QAbstractNativeEventFilter *> eventFilters;

    bool isV2 = false;

    static int allocateTimerId();
    static void releaseTimerId(int id);
    static void releaseTimerId(Qt::TimerId id)
    { releaseTimerId(qToUnderlying(id)); }

    static QAbstractEventDispatcherPrivate *get(QAbstractEventDispatcher *o)
    { return o->d_func(); }
    static const QAbstractEventDispatcherPrivate *get(const QAbstractEventDispatcher *o)
    { return o->d_func(); }
};

QT_END_NAMESPACE

#endif // QABSTRACTEVENTDISPATCHER_P_H
