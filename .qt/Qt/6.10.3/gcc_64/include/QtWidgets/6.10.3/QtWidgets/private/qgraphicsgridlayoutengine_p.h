// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QGRAPHICSGRIDLAYOUTENGINE_P_H
#define QGRAPHICSGRIDLAYOUTENGINE_P_H

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

#include <QtWidgets/private/qtwidgetsglobal_p.h>
#include <QtGui/private/qgridlayoutengine_p.h>

#include <QtWidgets/qsizepolicy.h>
#include <QtWidgets/qstyle.h>
#include <QtWidgets/qstyleoption.h>
#include "qgraphicslayoutitem.h"

QT_REQUIRE_CONFIG(graphicsview);

QT_BEGIN_NAMESPACE

class QGraphicsLayoutPrivate;

class QGraphicsGridLayoutEngineItem : public QGridLayoutItem {
public:
    QGraphicsGridLayoutEngineItem(QGraphicsLayoutItem *item, int row, int columns, int rowSpan = 1, int columnSpan = 1,
                            Qt::Alignment alignment = { })
        : QGridLayoutItem(row, columns, rowSpan, columnSpan, alignment), q_layoutItem(item) {}

    virtual QLayoutPolicy::Policy sizePolicy(Qt::Orientation orientation) const override
    {
        QSizePolicy sizePolicy(q_layoutItem->sizePolicy());
        return (QLayoutPolicy::Policy)((orientation == Qt::Horizontal) ? sizePolicy.horizontalPolicy()
                                               : sizePolicy.verticalPolicy());
    }

    virtual QLayoutPolicy::ControlTypes controlTypes(LayoutSide) const override
    {
        const QSizePolicy::ControlType ct = q_layoutItem->sizePolicy().controlType();
        return (QLayoutPolicy::ControlTypes)ct;
    }

    virtual QSizeF sizeHint(Qt::SizeHint which, const QSizeF &constraint) const override
    {
        return q_layoutItem->effectiveSizeHint(which, constraint);
    }

    virtual void setGeometry(const QRectF &rect) override
    {
         q_layoutItem->setGeometry(rect);
    }

    virtual bool hasDynamicConstraint() const override;
    virtual Qt::Orientation dynamicConstraintOrientation() const override;

    virtual bool isEmpty() const override;

    QGraphicsLayoutItem *layoutItem() const { return q_layoutItem; }

protected:
    QGraphicsLayoutItem *q_layoutItem;
};


class QGraphicsGridLayoutEngine : public QGridLayoutEngine
{
public:
    QGraphicsGridLayoutEngineItem *findLayoutItem(QGraphicsLayoutItem *layoutItem) const
    {
        const int index = indexOf(layoutItem);
        if (index < 0)
            return nullptr;
        return static_cast<QGraphicsGridLayoutEngineItem*>(q_items.at(index));
    }

    int indexOf(QGraphicsLayoutItem *item) const
    {
        for (int i = 0; i < q_items.size(); ++i) {
            if (item == static_cast<QGraphicsGridLayoutEngineItem*>(q_items.at(i))->layoutItem())
                return i;
        }
        return -1;
    }

    void setAlignment(QGraphicsLayoutItem *graphicsLayoutItem, Qt::Alignment alignment);
    Qt::Alignment alignment(QGraphicsLayoutItem *graphicsLayoutItem) const;

    void setStretchFactor(QGraphicsLayoutItem *layoutItem, int stretch, Qt::Orientation orientation);
    int stretchFactor(QGraphicsLayoutItem *layoutItem, Qt::Orientation orientation) const;

};

QT_END_NAMESPACE

#endif // QGRAPHICSGRIDLAYOUTENGINE_P_H
