// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKSWIPEDELEGATE_P_P_H
#define QQUICKSWIPEDELEGATE_P_P_H

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

#include <QtQuickTemplates2/private/qquickitemdelegate_p_p.h>
#include <QtQuickTemplates2/private/qquickswipe_p.h>

QT_BEGIN_NAMESPACE

class QQuickSwipeDelegate;

class QQuickSwipeDelegatePrivate : public QQuickItemDelegatePrivate
{
    Q_DECLARE_PUBLIC(QQuickSwipeDelegate)

public:
    QQuickSwipeDelegatePrivate(QQuickSwipeDelegate *control);

    bool handleMousePressEvent(QQuickItem *item, QMouseEvent *event);
    bool handleMouseMoveEvent(QQuickItem *item, QMouseEvent *event);
    bool handleMouseReleaseEvent(QQuickItem *item, QMouseEvent *event);
    void forwardMouseEvent(QMouseEvent *event, QQuickItem *destination, QPointF localPos);
    bool attachedObjectsSetPressed(QQuickItem *item, QPointF scenePos, bool pressed, bool cancel = false);
    QQuickItem *getPressedItem(QQuickItem *childItem, QMouseEvent *event) const;

    void resizeContent() override;
    void resizeBackground() override;

    QPalette defaultPalette() const override;

    QQuickSwipe swipe;
    QQuickItem *pressedItem = nullptr;
};

QT_END_NAMESPACE

#endif // QQUICKSWIPEDELEGATE_P_P_H
