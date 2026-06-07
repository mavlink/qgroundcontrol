// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKGRIDLAYOUTENGINE_P_H
#define QQUICKGRIDLAYOUTENGINE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of the graphics view layout classes.  This header
// file may change from version to version without notice, or even be removed.
//
// We mean it.
//

#include <QtGui/private/qgridlayoutengine_p.h>
#include <QtGui/private/qlayoutpolicy_p.h>
#include <QtCore/qmath.h>

#include "qquickitem.h"
#include "qquicklayout_p.h"
#include "qdebug.h"
QT_BEGIN_NAMESPACE

class QQuickGridLayoutItem : public QGridLayoutItem {
public:
    QQuickGridLayoutItem(QQuickItem *item, int row, int column,
                         int rowSpan = 1, int columnSpan = 1, Qt::Alignment alignment = { })
        : QGridLayoutItem(row, column, rowSpan, columnSpan, alignment), m_item(item), sizeHintCacheDirty(true), useFallbackToWidthOrHeight(true) {}


    QSizeF sizeHint(Qt::SizeHint which, const QSizeF &constraint) const override
    {
        Q_UNUSED(constraint);   // Quick Layouts does not support constraint atm
        return effectiveSizeHints()[which];
    }

    QSizeF *effectiveSizeHints() const
    {
        if (!sizeHintCacheDirty)
            return cachedSizeHints;

        QQuickLayout::effectiveSizeHints_helper(m_item, cachedSizeHints, nullptr, useFallbackToWidthOrHeight);
        useFallbackToWidthOrHeight = false;

        sizeHintCacheDirty = false;
        return cachedSizeHints;
    }

    void setCachedSizeHints(QSizeF *sizeHints)
    {
        for (int i = 0; i < Qt::NSizeHints; ++i) {
            cachedSizeHints[i] = sizeHints[i];
        }
        sizeHintCacheDirty = false;
    }

    void invalidate()
    {
        qCDebug(lcQuickLayouts) << "QQuickGridLayoutItem::invalidate()";
        sizeHintCacheDirty = true;
    }

    QLayoutPolicy::Policy sizePolicy(Qt::Orientation orientation) const override
    {
        return QQuickLayout::effectiveSizePolicy_helper(m_item, orientation, attachedLayoutObject(m_item, false));
    }

    void setGeometry(const QRectF &rect) override
    {
        QQuickLayoutAttached *info = attachedLayoutObject(m_item, false);
        const QRectF r = info ? rect.marginsRemoved(info->effectiveQMargins()) : rect;
        const QSizeF oldSize(m_item->width(), m_item->height());
        const QSizeF newSize = r.size();
        m_item->setPosition(r.topLeft());
        if (newSize == oldSize) {
            // We need to enforce a rearrange when the geometry is the same
            if (QQuickLayout *lay = qobject_cast<QQuickLayout *>(m_item)) {
                if (lay->invalidatedArrangement())
                    lay->rearrange(newSize);
            }
        } else {
            m_item->setSize(newSize);
        }
    }

    inline virtual QString toString() const override { return QDebug::toString(m_item); }

    QQuickItem *layoutItem() const { return m_item; }

    QQuickItem *m_item;
private:
    mutable QSizeF cachedSizeHints[Qt::NSizeHints];
    mutable unsigned sizeHintCacheDirty : 1;
    mutable unsigned useFallbackToWidthOrHeight : 1;
};

class QQuickGridLayoutEngine : public QGridLayoutEngine {
public:
    QQuickGridLayoutEngine() : QGridLayoutEngine(Qt::AlignVCenter, true /*snapToPixelGrid*/) { }

    int indexOf(QQuickItem *item) const {
        for (int i = 0; i < q_items.size(); ++i) {
            if (item == static_cast<QQuickGridLayoutItem*>(q_items.at(i))->layoutItem())
                return i;
        }
        return -1;
    }

    QQuickGridLayoutItem *findLayoutItem(QQuickItem *layoutItem) const
    {
        for (int i = q_items.size() - 1; i >= 0; --i) {
            QQuickGridLayoutItem *item = static_cast<QQuickGridLayoutItem*>(q_items.at(i));
            if (item->layoutItem() == layoutItem)
                return item;
        }
        return 0;
    }

    void setAlignment(QQuickItem *quickItem, Qt::Alignment alignment);

    void setStretchFactor(QQuickItem *quickItem, int stretch, Qt::Orientation orientation);

};



QT_END_NAMESPACE

#endif // QQUICKGRIDLAYOUTENGINE_P_H
