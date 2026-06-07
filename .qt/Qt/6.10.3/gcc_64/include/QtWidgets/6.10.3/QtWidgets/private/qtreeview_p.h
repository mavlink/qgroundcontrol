// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QTREEVIEW_P_H
#define QTREEVIEW_P_H

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
#include "qtreeview.h"
#include "private/qabstractitemview_p.h"
#include <QtCore/qabstractitemmodel.h>
#include <QtCore/qbasictimer.h>
#include <QtCore/qlist.h>
#if QT_CONFIG(animation)
#include <QtCore/qvariantanimation.h>
#endif

#include <array>

QT_REQUIRE_CONFIG(treeview);

QT_BEGIN_NAMESPACE

struct QTreeViewItem
{
    QTreeViewItem() : parentItem(-1), expanded(false), spanning(false), hasChildren(false),
                      hasMoreSiblings(false), total(0), level(0), height(0) {}
    QModelIndex index; // we remove items whenever the indexes are invalidated
    int parentItem; // parent item index in viewItems
    uint expanded : 1;
    uint spanning : 1;
    uint hasChildren : 1; // if the item has visible children (even if collapsed)
    uint hasMoreSiblings : 1;
    uint total : 28; // total number of children visible
    uint level : 16; // indentation
    int height : 16; // row height
};

Q_DECLARE_TYPEINFO(QTreeViewItem, Q_RELOCATABLE_TYPE);

class Q_WIDGETS_EXPORT QTreeViewPrivate : public QAbstractItemViewPrivate
{
    Q_DECLARE_PUBLIC(QTreeView)
public:

    QTreeViewPrivate()
        : QAbstractItemViewPrivate(),
          header(nullptr), indent(20), lastViewedItem(0), defaultItemHeight(-1),
          uniformRowHeights(false), rootDecoration(true),
          itemsExpandable(true), sortingEnabled(false),
          expandsOnDoubleClick(true),
          allColumnsShowFocus(false), customIndent(false), current(0), spanning(false),
          animationsEnabled(false),
          autoExpandDelay(-1), hoverBranch(-1), geometryRecursionBlock(false), hasRemovedItems(false),
          treePosition(0) {}

    ~QTreeViewPrivate() {}
    void initialize();
    void clearConnections();
    int logicalIndexForTree() const;
    inline bool isTreePosition(int logicalIndex) const
    {
        return logicalIndex == logicalIndexForTree();
    }

    QItemViewPaintPairs draggablePaintPairs(const QModelIndexList &indexes, QRect *r) const override;
    void adjustViewOptionsForIndex(QStyleOptionViewItem *option, const QModelIndex &current) const override;

#if QT_CONFIG(animation)
    struct AnimatedOperation : public QVariantAnimation
    {
        int item;
        QPixmap before;
        QPixmap after;
        QWidget *viewport;
        AnimatedOperation() : item(0) { setEasingCurve(QEasingCurve::InOutQuad); }
        int top() const { return startValue().toInt(); }
        QRect rect() const { QRect rect = viewport->rect(); rect.moveTop(top()); return rect; }
        void updateCurrentValue(const QVariant &) override { viewport->update(rect()); }
        void updateState(State state, State) override { if (state == Stopped) before = after = QPixmap(); }
    } animatedOperation;
    void prepareAnimatedOperation(int item, QVariantAnimation::Direction d);
    void beginAnimatedOperation();
    void drawAnimatedOperation(QPainter *painter) const;
    QPixmap renderTreeToPixmapForAnimation(const QRect &rect) const;
    void endAnimatedOperation();
#endif // animation

    void expand(int item, bool emitSignal);
    void collapse(int item, bool emitSignal);

    void columnsAboutToBeRemoved(const QModelIndex &, int, int) override;
    void columnsRemoved(const QModelIndex &, int, int) override;
    void modelAboutToBeReset();
    void sortIndicatorChanged(int column, Qt::SortOrder order);
    void modelDestroyed() override;
    QRect intersectedRect(const QRect rect, const QModelIndex &topLeft, const QModelIndex &bottomRight) const override;

    void layout(int item, bool recusiveExpanding = false, bool afterIsUninitialized = false);

    int pageUp(int item) const;
    int pageDown(int item) const;
    int itemForKeyHome() const;
    int itemForKeyEnd() const;

    int itemHeight(int item) const;
    int indentationForItem(int item) const;
    int coordinateForItem(int item) const;
    int itemAtCoordinate(int coordinate) const;

    int viewIndex(const QModelIndex &index) const;
    QModelIndex modelIndex(int i, int column = 0) const;

    void insertViewItems(int pos, int count, const QTreeViewItem &viewItem);
    void removeViewItems(int pos, int count);
#if 0
    bool checkViewItems() const;
#endif

    int firstVisibleItem(int *offset = nullptr) const;
    int lastVisibleItem(int firstVisual = -1, int offset = -1) const;
    int columnAt(int x) const;
    bool hasVisibleChildren( const QModelIndex& parent) const;

    bool expandOrCollapseItemAtPos(const QPoint &pos);

    void updateScrollBars();

    int itemDecorationAt(const QPoint &pos) const;
    QRect itemDecorationRect(const QModelIndex &index) const;

    QList<std::pair<int, int>> columnRanges(const QModelIndex &topIndex,
                                        const QModelIndex &bottomIndex) const;
    void select(const QModelIndex &start, const QModelIndex &stop, QItemSelectionModel::SelectionFlags command);

    std::pair<int,int> startAndEndColumns(const QRect &rect) const;

    void updateChildCount(const int parentItem, const int delta);

    void paintAlternatingRowColors(QPainter *painter, QStyleOptionViewItem *option, int y, int bottom) const;

    // logicalIndices: vector of currently visibly logical indices
    // itemPositions: vector of view item positions (beginning/middle/end/onlyone)
    void calcLogicalIndices(QList<int> *logicalIndices,
                            QList<QStyleOptionViewItem::ViewItemPosition> *itemPositions, int left,
                            int right) const;
    int widthHintForIndex(const QModelIndex &index, int hint, const QStyleOptionViewItem &option, int i) const;

    enum RectRule {
        FullRow,
        SingleSection,
        AddRowIndicatorToFirstSection
    };

    // Base class will get the first visual rect including row indicator
    QRect visualRect(const QModelIndex &index) const override
    {
        return visualRect(index, AddRowIndicatorToFirstSection);
    }

    QRect visualRect(const QModelIndex &index, RectRule rule) const;

    QHeaderView *header;
    int indent;

    mutable QList<QTreeViewItem> viewItems;
    mutable int lastViewedItem;
    int defaultItemHeight; // this is just a number; contentsHeight() / numItems
    bool uniformRowHeights; // used when all rows have the same height
    bool rootDecoration;
    bool itemsExpandable;
    bool sortingEnabled;
    bool expandsOnDoubleClick;
    bool allColumnsShowFocus;
    bool customIndent;

    // used for drawing
    mutable std::pair<int,int> leftAndRight;
    mutable int current;
    mutable bool spanning;

    // used when expanding and collapsing items
    QSet<QPersistentModelIndex> expandedIndexes;
    bool animationsEnabled;

    inline bool storeExpanded(const QPersistentModelIndex &idx) {
        if (expandedIndexes.contains(idx))
            return false;
        expandedIndexes.insert(idx);
        return true;
    }

    inline bool isIndexExpanded(const QModelIndex &idx) const {
        //We first check if the idx is a QPersistentModelIndex, because creating QPersistentModelIndex is slow
        return !(idx.flags() & Qt::ItemNeverHasChildren) && isPersistent(idx) && expandedIndexes.contains(idx);
    }

    // used when hiding and showing items
    QSet<QPersistentModelIndex> hiddenIndexes;

    inline bool isRowHidden(const QModelIndex &idx) const {
        if (hiddenIndexes.isEmpty())
            return false;
        //We first check if the idx is a QPersistentModelIndex, because creating QPersistentModelIndex is slow
        return isPersistent(idx) && hiddenIndexes.contains(idx);
    }

    inline bool isItemHiddenOrDisabled(int i) const {
        if (i < 0 || i >= viewItems.size())
            return false;
        const QModelIndex index = viewItems.at(i).index;
        return isRowHidden(index) || !isIndexEnabled(index);
    }

    inline int above(int item) const
        { int i = item; while (isItemHiddenOrDisabled(--item)){} return item < 0 ? i : item; }
    inline int below(int item) const
        { int i = item; while (isItemHiddenOrDisabled(++item)){} return item >= viewItems.size() ? i : item; }
    inline void invalidateHeightCache(int item) const
        { viewItems[item].height = 0; }

    inline int accessibleTable2Index(const QModelIndex &index) const {
        return (viewIndex(index) + (header ? 1 : 0)) * model->columnCount()+index.column();
    }

    int accessibleTree2Index(const QModelIndex &index) const;

    void updateIndentationFromStyle();

    // used for spanning rows
    QSet<QPersistentModelIndex> spanningIndexes;

    // used for updating resized columns
    QBasicTimer columnResizeTimer;
    QList<int> columnsToUpdate;

    // used for the automatic opening of nodes during DND
    int autoExpandDelay;
    QBasicTimer openTimer;

    // used for drawing highlighted expand/collapse indicators
    mutable int hoverBranch;

    // used for blocking recursion when calling setViewportMargins from updateGeometries
    bool geometryRecursionBlock;

    // If we should clean the set
    bool hasRemovedItems;

    // tree position
    int treePosition;

    // pending accessibility update
#if QT_CONFIG(accessibility)
    bool pendingAccessibilityUpdate = false;
#endif
    void updateAccessibility();

    QMetaObject::Connection animationConnection;
    QMetaObject::Connection selectionmodelConnection;
    std::array<QMetaObject::Connection, 2> modelConnections;
    std::array<QMetaObject::Connection, 5> headerConnections;
    QMetaObject::Connection sortHeaderConnection;
};

QT_END_NAMESPACE

#endif // QTREEVIEW_P_H
