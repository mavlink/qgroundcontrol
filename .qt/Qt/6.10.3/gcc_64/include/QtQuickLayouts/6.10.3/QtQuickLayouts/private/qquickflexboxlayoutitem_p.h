// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKFLEXBOXLAYOUTITEM_P_H
#define QQUICKFLEXBOXLAYOUTITEM_P_H

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

#include <QtQuick/private/qquickitem_p.h>
#include <QtQuickLayouts/private/qquickflexboxlayout_p.h>
#include <yoga/Yoga.h>

QT_BEGIN_NAMESPACE

struct SizeHints {
    inline QSizeF &min() { return array[Qt::MinimumSize]; }
    inline QSizeF &pref() { return array[Qt::PreferredSize]; }
    inline QSizeF &max() { return array[Qt::MaximumSize]; }
    QSizeF array[Qt::NSizeHints];
};

class QQuickFlexboxLayoutItem
{
public:
    explicit QQuickFlexboxLayoutItem(QQuickItem *item);
    ~QQuickFlexboxLayoutItem();

    void resetDefault();
    void resetMargins();
    void resetPaddings();
    void resetSize();

    QQuickItem *quickItem() const { return m_item; }
    YGNodeRef yogaItem() const { return m_yogaNode; }

    void insertChild(QQuickFlexboxLayoutItem *item, int index);

    // Adapter APIs to the yoga library
    void setMinSize(const QSizeF &size);
    void setSize(const QSizeF &size);
    void setWidth(const qreal &width);
    void setHeight(const qreal &height);
    void setMaxSize(const QSizeF &size);
    void setFlexBasis(qreal value, bool reset = false);
    void setFlexDirection(QQuickFlexboxLayout::FlexboxDirection direction);
    void setFlexWrap(QQuickFlexboxLayout::FlexboxWrap wrap);
    void setFlexAlignItemsProperty(QQuickFlexboxLayout::FlexboxAlignment align);
    void setFlexAlignSelfProperty(QQuickFlexboxLayout::FlexboxAlignment align);
    void setFlexAlignContentProperty(QQuickFlexboxLayout::FlexboxAlignment align);
    void setFlexJustifyContentProperty(QQuickFlexboxLayout::FlexboxJustify justify);
    void setItemGrowAlongMainAxis(qreal value);
    void setItemShrinkAlongMainAxis(qreal value);
    void setFlexMargin(QQuickFlexboxLayout::FlexboxEdge edge, qreal value);
    void setFlexPadding(QQuickFlexboxLayout::FlexboxEdge edge, qreal value);
    void setItemStretchAlongCrossSection();
    void setFlexGap(QQuickFlexboxLayout::FlexboxGap gap, qreal value);
    void inheritItemStretchAlongCrossSection();

    bool hasMeasureFunc() const;
    void resetMeasureFunc();
    QPoint position() const;
    QSizeF size() const;
    bool isFlexBasisUndefined() const;
    bool isItemStreched() const;
    QSizeF computedLayoutSize() { return m_computedLayoutSize; }
    SizeHints &cachedItemSizeHints() const;
    void computeLayout(const QSizeF &size = QSizeF());

private:
    QQuickItem *m_item;
    YGNodeRef m_yogaNode;
    QSizeF m_computedLayoutSize;
    mutable SizeHints m_cachedSizeHint;
};

QT_END_NAMESPACE

#endif // QQUICKFLEXBOXLAYOUTITEM_P_H
