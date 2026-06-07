// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKDRAWER_P_P_H
#define QQUICKDRAWER_P_P_H

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

#include <QtQuickTemplates2/private/qquickdrawer_p.h>
#include <QtQuickTemplates2/private/qquickpopup_p_p.h>
#include <QtQuickTemplates2/private/qquickvelocitycalculator_p_p.h>

QT_BEGIN_NAMESPACE

class Q_QUICKTEMPLATES2_EXPORT QQuickDrawerPrivate : public QQuickPopupPrivate
{
    Q_DECLARE_PUBLIC(QQuickDrawer)

public:
    static QQuickDrawerPrivate *get(QQuickDrawer *drawer)
    {
        return drawer->d_func();
    }

    qreal offsetAt(const QPointF &point) const;
    qreal positionAt(const QPointF &point) const;

    QQuickPopupPositioner *getPositioner() override;
    void showDimmer() override;
    void hideDimmer() override;
    void resizeDimmer() override;

    bool startDrag(QEvent *event);
    bool grabMouse(QQuickItem *item, QMouseEvent *event);
#if QT_CONFIG(quicktemplates2_multitouch)
    bool grabTouch(QQuickItem *item, QTouchEvent *event);
#endif
    bool blockInput(QQuickItem *item, const QPointF &point) const override;

    bool handlePress(QQuickItem* item, const QPointF &point, ulong timestamp) override;
    bool handleMove(QQuickItem* item, const QPointF &point, ulong timestamp) override;
    bool handleRelease(QQuickItem* item, const QPointF &point, ulong timestamp) override;
    void handleUngrab() override;

    bool prepareEnterTransition() override;
    bool prepareExitTransition() override;

    QQuickPopup::PopupType resolvedPopupType() const override;

    bool setEdge(Qt::Edge edge);
    Qt::Edge effectiveEdge() const;
    bool isWithinDragMargin(const QPointF &point) const;

    Qt::Edge edge = Qt::LeftEdge;
    qreal offset = 0;
    qreal position = 0;
    qreal dragMargin = 0;
    QQuickVelocityCalculator velocityCalculator;
    bool delayedEnterTransition = false;
};

QT_END_NAMESPACE

#endif // QQUICKDRAWER_P_P_H
