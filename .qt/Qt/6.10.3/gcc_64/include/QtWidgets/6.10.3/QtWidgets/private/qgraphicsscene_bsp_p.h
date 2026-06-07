// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QGRAPHICSSCENEBSPTREE_P_H
#define QGRAPHICSSCENEBSPTREE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of other Qt classes.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtWidgets/private/qtwidgetsglobal_p.h>
#include <QtCore/qlist.h>
#include <QtCore/qrect.h>
#include <QtCore/qset.h>

QT_REQUIRE_CONFIG(graphicsview);

QT_BEGIN_NAMESPACE

class QGraphicsItem;

class QGraphicsSceneBspTree
{
public:
    struct Node
    {
        enum Type { Horizontal, Vertical, Leaf };
        union {
            qreal offset;
            int leafIndex;
        };
        Type type;
    };

    QGraphicsSceneBspTree();
    ~QGraphicsSceneBspTree();

    void initialize(const QRectF &rect, int depth);
    void clear();

    void insertItem(QGraphicsItem *item, const QRectF &rect);
    void removeItem(QGraphicsItem *item, const QRectF &rect);
    void removeItems(const QSet<QGraphicsItem *> &items);

    QList<QGraphicsItem *> items(const QRectF &rect, bool onlyTopLevelItems = false) const;
    int leafCount() const;

    inline int firstChildIndex(int index) const
    { return index * 2 + 1; }

    inline int parentIndex(int index) const
    { return index > 0 ? ((index & 1) ? ((index - 1) / 2) : ((index - 2) / 2)) : -1; }

    QString debug(int index) const;

private:
    void initialize(const QRectF &rect, int depth, int index);
    template<typename Visitor>
    void climbTree(Visitor &&visitor, const QRectF &rect, int index = 0) const;
    QRectF rectForIndex(int index) const;

    QList<Node> nodes;
    QList<QList<QGraphicsItem *>> leaves;
    int leafCnt;
    QRectF rect;
};

QT_END_NAMESPACE

#endif // QGRAPHICSSCENEBSPTREE_P_H
