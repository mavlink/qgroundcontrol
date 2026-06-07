// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKPOINTERHANDLER_P_H
#define QQUICKPOINTERHANDLER_P_H

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

#include "qevent.h"

#include <QtCore/private/qobject_p.h>
#include <QtQuick/private/qquickevents_p_p.h>
#include <QtQuick/private/qquickpointerhandler_p.h>

QT_BEGIN_NAMESPACE

class Q_QUICK_EXPORT QQuickPointerHandlerPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QQuickPointerHandler)

public:
    static QQuickPointerHandlerPrivate* get(QQuickPointerHandler *q) { return q->d_func(); }
    static const QQuickPointerHandlerPrivate* get(const QQuickPointerHandler *q) { return q->d_func(); }

    QQuickPointerHandlerPrivate();

    template<typename TEventPoint>
    bool dragOverThreshold(qreal d, Qt::Axis axis, const TEventPoint &p) const;

    bool dragOverThreshold(QVector2D delta) const;
    bool dragOverThreshold(const QEventPoint &point) const;

    virtual void onParentChanged(QQuickItem * /*oldParent*/, QQuickItem * /*newParent*/) {}
    virtual void onEnabledChanged() {}

    static QVector<QObject *> &deviceDeliveryTargets(const QInputDevice *device);

    QPointerEvent *currentEvent = nullptr;
    QQuickItem *target = nullptr;
    qreal m_margin = 0;
    quint64 lastEventTime = 0;
    qint16 dragThreshold = -1;   // -1 means use the platform default
    uint8_t grabPermissions : 8;
    Qt::CursorShape cursorShape : 6;
    bool enabled : 1;
    bool active : 1;
    bool targetExplicitlySet : 1;
    bool hadKeepMouseGrab : 1;    // some handlers override target()->setKeepMouseGrab(); this remembers previous state
    bool hadKeepTouchGrab : 1;    // some handlers override target()->setKeepTouchGrab(); this remembers previous state
    bool cursorSet : 1;
    bool cursorDirty : 1;
};

QT_END_NAMESPACE

#endif // QQUICKPOINTERHANDLER_P_H
