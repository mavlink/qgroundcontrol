// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKFXVIEWITEM_P_P_H
#define QQUICKFXVIEWITEM_P_P_H

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
#include <QtQuick/private/qquickitem_p.h>
#if QT_CONFIG(quick_viewtransitions)
#include <QtQuick/private/qquickitemviewtransition_p.h>
#endif
#include <private/qanimationjobutil_p.h>

#include <QtCore/qpointer.h>

QT_REQUIRE_CONFIG(quick_itemview);

QT_BEGIN_NAMESPACE

class Q_QUICK_EXPORT QQuickItemViewFxItem
{
public:
    QQuickItemViewFxItem(QQuickItem *item, bool ownItem, QQuickItemChangeListener *changeListener);
    virtual ~QQuickItemViewFxItem();

    qreal itemX() const;
    qreal itemY() const;
    inline qreal itemWidth() const { return item ? item->width() : 0; }
    inline qreal itemHeight() const { return item ? item->height() : 0; }

    void moveTo(const QPointF &pos, bool immediate);
    void setVisible(bool visible);
    void trackGeometry(bool track);

    QRectF geometry() const;
    void setGeometry(const QRectF &geometry);

#if QT_CONFIG(quick_viewtransitions)
    QQuickItemViewTransitioner::TransitionType scheduledTransitionType() const;
    bool transitionScheduledOrRunning() const;
    bool transitionRunning() const;
    bool isPendingRemoval() const;

    void transitionNextReposition(QQuickItemViewTransitioner *transitioner, QQuickItemViewTransitioner::TransitionType type, bool asTarget);
    bool prepareTransition(QQuickItemViewTransitioner *transitioner, const QRectF &viewBounds);
    void startTransition(QQuickItemViewTransitioner *transitioner);
#endif

    // these are positions and sizes along the current direction of scrolling/flicking
    virtual qreal position() const = 0;
    virtual qreal endPosition() const = 0;
    virtual qreal size() const = 0;
    virtual qreal sectionSize() const = 0;

    virtual bool contains(qreal x, qreal y) const = 0;

    SelfDeletable m_selfDeletable;
    QPointer<QQuickItem> item;
    QQuickItemChangeListener *changeListener;
#if QT_CONFIG(quick_viewtransitions)
    QQuickItemViewTransitionableItem *transitionableItem;
#endif
    int index = -1;
    bool ownItem : 1;
    bool releaseAfterTransition : 1;
    bool trackGeom : 1;
};

QT_END_NAMESPACE

#endif // QQUICKFXVIEWITEM_P_P_H
