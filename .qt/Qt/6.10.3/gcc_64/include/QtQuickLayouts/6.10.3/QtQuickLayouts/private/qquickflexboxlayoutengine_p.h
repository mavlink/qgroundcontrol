// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKFLEXBOXLAYOUTENGINE_H
#define QQUICKFLEXBOXLAYOUTENGINE_H

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

#include <QtQuickLayouts/private/qquickflexboxlayoutitem_p.h>

QT_BEGIN_NAMESPACE

class QQuickFlexboxLayoutEngine
{
public:
    QQuickFlexboxLayoutEngine();
    ~QQuickFlexboxLayoutEngine();

    void setGeometries(const QSizeF &contentSize);
    QSizeF sizeHint(Qt::SizeHint whichSizeHint) const;
    void collectItemSizeHints(QQuickFlexboxLayoutItem *flexItem, QSizeF *sizeHints) const;
    void removeItemSizeHint(QQuickItem *item);
    void invalidateItemSizeHint(QQuickItem *item);
    bool isChildOverflowingParent(QQuickItem *item);

    void insertItem(QQuickFlexboxLayoutItem *item);
    int itemCount() const;
    QQuickItem *itemAt(int index) const;
    int indexOf(QQuickItem *item) const;
    QQuickFlexboxLayoutItem *findFlexboxLayoutItem(QQuickItem *item) const;
    void clearItems();

    void setFlexboxParentItem(QQuickFlexboxLayoutItem *parentItem);
    QQuickFlexboxLayoutItem *getFlexboxParentItem() { return m_flexboxParentItem; }

private:
    mutable QSizeF m_cachedSizeHints[Qt::NSizeHints];
    SizeHints &cachedItemSizeHints(int index) const;

    QList<QQuickFlexboxLayoutItem *> m_flexLayoutItems;
    QQuickFlexboxLayoutItem *m_flexboxParentItem;
    int m_visualDirection;
    QSizeF m_contentSize;
};

QT_END_NAMESPACE

#endif // QQUICKFLEXBOXLAYOUTENGINE_H
