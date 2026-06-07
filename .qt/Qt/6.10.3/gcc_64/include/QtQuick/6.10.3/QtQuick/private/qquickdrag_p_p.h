// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKDRAG_P_P_H
#define QQUICKDRAG_P_P_H

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

#include "qquickdrag_p.h"

#include <QtCore/qobject.h>
#include <QtCore/private/qobject_p.h>
#include <QtCore/qpointer.h>
#include <QtCore/qurl.h>
#include <QtCore/qvariant.h>

#include <QtQuick/qquickitem.h>
#include <QtQuick/private/qquickitemchangelistener_p.h>
#include <QtQuick/private/qquickpixmap_p.h>

QT_BEGIN_NAMESPACE

class Q_AUTOTEST_EXPORT QQuickDragAttachedPrivate : public QObjectPrivate,
                                                    public QSafeQuickItemChangeListener<QQuickDragAttachedPrivate>
{
    Q_DECLARE_PUBLIC(QQuickDragAttached)
    QML_ANONYMOUS
    QML_ADDED_IN_VERSION(2, 0)

public:
    static QQuickDragAttachedPrivate *get(QQuickDragAttached *attached) {
        return static_cast<QQuickDragAttachedPrivate *>(QObjectPrivate::get(attached)); }

    QQuickDragAttachedPrivate()
        : attachedItem(nullptr)
        , mimeData(nullptr)
        , proposedAction(Qt::MoveAction)
        , supportedActions(Qt::MoveAction | Qt::CopyAction | Qt::LinkAction)
        , active(false)
        , listening(false)
        , inEvent(false)
        , dragRestarted(false)
        , itemMoved(false)
        , eventQueued(false)
        , overrideActions(false)
        , dragType(QQuickDrag::Internal)
    {
    }

    void itemGeometryChanged(QQuickItem *, QQuickGeometryChange, const QRectF &) override;
    void itemParentChanged(QQuickItem *, QQuickItem *parent) override;
    void updatePosition();
    void restartDrag();
    void deliverEnterEvent();
    void deliverMoveEvent();
    void deliverLeaveEvent();
    void deliverEvent(QQuickWindow *window, QEvent *event);
    void start(Qt::DropActions supportedActions);
    Qt::DropAction startDrag(Qt::DropActions supportedActions);
    void setTarget(QQuickItem *item);
    QMimeData *createMimeData() const;
    void loadPixmap();

    QQuickDragGrabber dragGrabber;

    QPointer<QObject> source;
    QPointer<QObject> target;
    QPointer<QQuickWindow> window;
    QQuickItem *attachedItem;
    QQuickDragMimeData *mimeData;
    Qt::DropAction proposedAction;
    Qt::DropActions supportedActions;
    bool active : 1;
    bool listening : 1;
    bool inEvent : 1;
    bool dragRestarted : 1;
    bool itemMoved : 1;
    bool eventQueued : 1;
    bool overrideActions : 1;
    QPointF hotSpot;
    QUrl imageSource;
    QSize imageSourceSize;
    QQuickPixmap pixmapLoader;
    QStringList keys;
    QVariantMap externalMimeData;
    QQuickDrag::DragType dragType;
};

QT_END_NAMESPACE

#endif
