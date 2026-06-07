// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKSELECTIONRECTANGLE_P_P_H
#define QQUICKSELECTIONRECTANGLE_P_P_H

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

#include "qquickselectionrectangle_p.h"

#include <QtCore/qpointer.h>
#include <QtCore/qtimer.h>

#include <QtQuick/private/qquickselectable_p.h>
#include <QtQuick/private/qquicktaphandler_p.h>
#include <QtQuick/private/qquickdraghandler_p.h>

#include <QtQuickTemplates2/private/qquickcontrol_p_p.h>

QT_BEGIN_NAMESPACE

class QQuickSelectionRectanglePrivate : public QQuickControlPrivate
{
public:
    Q_DECLARE_PUBLIC(QQuickSelectionRectangle)

    QQuickSelectionRectanglePrivate();

    void updateDraggingState(bool isDragging);
    void updateActiveState(bool isActive);
    void updateHandles();
    void updateSelectionMode();
    void connectToTarget();
    void scrollTowardsPos(const QPointF &pos);
    QQuickItem *handleUnderPos(const QPointF &pos);

    QQuickItem *createHandle(QQmlComponent *delegate, Qt::Corner corner);

    QQuickSelectionRectangleAttached *getAttachedObject(const QObject *object) const;

public:
    QPointer<QQuickItem> m_target;

    QQmlComponent *m_topLeftHandleDelegate = nullptr;
    QQmlComponent *m_bottomRightHandleDelegate = nullptr;
    QScopedPointer<QQuickItem> m_topLeftHandle;
    QScopedPointer<QQuickItem> m_bottomRightHandle;
    QPointer<QQuickItem> m_draggedHandle;

    QQuickSelectable *m_selectable = nullptr;

    QQuickTapHandler *m_tapHandler = nullptr;
    QQuickDragHandler *m_dragHandler = nullptr;

    QTimer m_scrollTimer;
    QPointF m_scrollToPoint;
    QSizeF m_scrollSpeed = QSizeF(1, 1);

    QQuickSelectionRectangle::SelectionMode m_selectionMode = QQuickSelectionRectangle::Auto;
    QQuickSelectionRectangle::SelectionMode m_effectiveSelectionMode = QQuickSelectionRectangle::Drag;

    bool m_enabled = true;
    bool m_active = false;
    bool m_dragging = false;
};

QT_END_NAMESPACE

#endif // QQUICKSELECTIONRECTANGLE_P_P_H
