// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKPOSITIONERS_P_P_H
#define QQUICKPOSITIONERS_P_P_H

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

#include <QtQuick/private/qtquickglobal_p.h>

QT_REQUIRE_CONFIG(quick_positioners);

#include "qquickpositioners_p.h"
#include "qquickimplicitsizeitem_p_p.h"

#include <private/qlazilyallocated_p.h>

#include <QtCore/qobject.h>
#include <QtCore/qstring.h>
#include <QtCore/qtimer.h>

#include <algorithm>

QT_BEGIN_NAMESPACE

class QQuickItemViewTransitioner;

class QQuickBasePositionerPrivate : public QQuickImplicitSizeItemPrivate,
                                    public QSafeQuickItemChangeListener<QQuickBasePositionerPrivate>
{
    Q_DECLARE_PUBLIC(QQuickBasePositioner)

public:
    struct ExtraData {
        ExtraData();

        qreal padding;
        qreal topPadding;
        qreal leftPadding;
        qreal rightPadding;
        qreal bottomPadding;
        bool explicitTopPadding : 1;
        bool explicitLeftPadding : 1;
        bool explicitRightPadding : 1;
        bool explicitBottomPadding : 1;
    };
    QLazilyAllocated<ExtraData> extra;

    QQuickBasePositionerPrivate()
        : spacing(0), type(QQuickBasePositioner::None)
#if QT_CONFIG(quick_viewtransitions)
        , transitioner(0)
#endif
        , positioningDirty(false)
        , doingPositioning(false), anchorConflict(false), layoutDirection(Qt::LeftToRight)

    {
    }

    void init(QQuickBasePositioner::PositionerType at)
    {
        type = at;
    }

    qreal spacing;

    QQuickBasePositioner::PositionerType type;
#if QT_CONFIG(quick_viewtransitions)
    QQuickItemViewTransitioner *transitioner;
#endif

    void watchChanges(QQuickItem *other);
    void unwatchChanges(QQuickItem* other);
    void setPositioningDirty() {
        Q_Q(QQuickBasePositioner);
        if (!positioningDirty) {
            positioningDirty = true;
            q->polish();
        }
    }

    bool positioningDirty : 1;
    bool doingPositioning : 1;
    bool anchorConflict : 1;

    Qt::LayoutDirection layoutDirection;

    void mirrorChange() override {
        effectiveLayoutDirectionChange();
    }
    bool isLeftToRight() const {
        if (type == QQuickBasePositioner::Vertical)
            return true;
        else
            return effectiveLayoutMirror ? layoutDirection == Qt::RightToLeft : layoutDirection == Qt::LeftToRight;
    }

    void itemSiblingOrderChanged(QQuickItem* other) override
    {
        Q_UNUSED(other);
        setPositioningDirty();
    }

    void itemGeometryChanged(QQuickItem *, QQuickGeometryChange change, const QRectF &) override
    {
        if (change.sizeChange())
            setPositioningDirty();
    }

    void itemVisibilityChanged(QQuickItem *) override
    {
        setPositioningDirty();
    }

    void itemDestroyed(QQuickItem *item) override
    {
        Q_Q(QQuickBasePositioner);
        auto it = std::find(q->positionedItems.begin(), q->positionedItems.end(), item);
        if (it != q->positionedItems.end())
            q->positionedItems.erase(it);
    }

    static Qt::LayoutDirection getLayoutDirection(const QQuickBasePositioner *positioner)
    {
        return positioner->d_func()->layoutDirection;
    }

    static Qt::LayoutDirection getEffectiveLayoutDirection(const QQuickBasePositioner *positioner)
    {
        if (positioner->d_func()->effectiveLayoutMirror)
            return positioner->d_func()->layoutDirection == Qt::RightToLeft ? Qt::LeftToRight : Qt::RightToLeft;
        else
            return positioner->d_func()->layoutDirection;
    }

    virtual void effectiveLayoutDirectionChange()
    {
    }

    inline qreal padding() const { return extra.isAllocated() ? extra->padding : 0.0; }
    void setTopPadding(qreal value, bool reset = false);
    void setLeftPadding(qreal value, bool reset = false);
    void setRightPadding(qreal value, bool reset = false);
    void setBottomPadding(qreal value, bool reset = false);
};

QT_END_NAMESPACE

#endif // QQUICKPOSITIONERS_P_P_H
