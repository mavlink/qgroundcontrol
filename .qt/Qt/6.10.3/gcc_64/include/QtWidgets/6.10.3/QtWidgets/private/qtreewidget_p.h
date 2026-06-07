// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QTREEWIDGET_P_H
#define QTREEWIDGET_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. This header file may change
// from version to version without notice, or even be removed.
//
// We mean it.
//

#include <QtWidgets/private/qtwidgetsglobal_p.h>
#include <QtCore/qabstractitemmodel.h>
#include <private/qabstractitemmodel_p.h>
#include <QtCore/qbasictimer.h>
#include <QtWidgets/qtreewidget.h>
#include <private/qtreeview_p.h>
#include <QtWidgets/qheaderview.h>

#include <array>

QT_REQUIRE_CONFIG(treewidget);

QT_BEGIN_NAMESPACE

class QTreeWidgetItem;
class QTreeWidgetItemIterator;
class QTreeModelPrivate;

class QTreeModel : public QAbstractItemModel
{
    Q_OBJECT
    friend class QTreeWidget;
    friend class QTreeWidgetPrivate;
    friend class QTreeWidgetItem;
    friend class QTreeWidgetItemPrivate;
    friend class QTreeWidgetItemIterator;
    friend class QTreeWidgetItemIteratorPrivate;

public:
    explicit QTreeModel(int columns = 0, QTreeWidget *parent = nullptr);
    ~QTreeModel();

    inline QTreeWidget *view() const
        { return qobject_cast<QTreeWidget*>(QObject::parent()); }

    void clear();
    void setColumnCount(int columns);

    QTreeWidgetItem *item(const QModelIndex &index) const;
    void itemChanged(QTreeWidgetItem *item);

    QModelIndex index(const QTreeWidgetItem *item, int column) const;
    QModelIndex index(int row, int column, const QModelIndex &parent) const override;
    QModelIndex parent(const QModelIndex &child) const override;
    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    bool hasChildren(const QModelIndex &parent) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    bool clearItemData(const QModelIndex &index) override;
    QMap<int, QVariant> itemData(const QModelIndex &index) const override;

    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    bool setHeaderData(int section, Qt::Orientation orientation, const QVariant &value,
                       int role) override;

    Qt::ItemFlags flags(const QModelIndex &index) const override;

    void sort(int column, Qt::SortOrder order) override;
    void ensureSorted(int column, Qt::SortOrder order,
                      int start, int end, const QModelIndex &parent);
    static bool itemLessThan(const std::pair<QTreeWidgetItem*,int> &left,
                             const std::pair<QTreeWidgetItem*,int> &right);
    static bool itemGreaterThan(const std::pair<QTreeWidgetItem*,int> &left,
                                const std::pair<QTreeWidgetItem*,int> &right);
    static QList<QTreeWidgetItem*>::iterator sortedInsertionIterator(
        const QList<QTreeWidgetItem*>::iterator &begin,
        const QList<QTreeWidgetItem*>::iterator &end,
        Qt::SortOrder order, QTreeWidgetItem *item);

    bool insertRows(int row, int count, const QModelIndex &) override;
    bool insertColumns(int column, int count, const QModelIndex &) override;

    bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;

    // dnd
    QStringList mimeTypes() const override;
    QMimeData *mimeData(const QModelIndexList &indexes) const override;
    bool dropMimeData(const QMimeData *data, Qt::DropAction action,
                      int row, int column, const QModelIndex &parent) override;
    Qt::DropActions supportedDropActions() const override;
    Qt::DropActions supportedDragActions() const override;

    QMimeData *internalMimeData() const;

    inline QModelIndex createIndexFromItem(int row, int col, QTreeWidgetItem *item) const
    { return createIndex(row, col, item); }

protected:
    QTreeModel(QTreeModelPrivate &, QTreeWidget *parent = nullptr);
    void emitDataChanged(QTreeWidgetItem *item, int column, const QList<int> &roles);
    void beginInsertItems(QTreeWidgetItem *parent, int row, int count);
    void endInsertItems();
    void beginRemoveItems(QTreeWidgetItem *parent, int row, int count);
    void endRemoveItems();
    void sortItems(QList<QTreeWidgetItem*> *items, int column, Qt::SortOrder order);
    void timerEvent(QTimerEvent *) override;

private:
    QTreeWidgetItem *rootItem;
    QTreeWidgetItem *headerItem;

    mutable QModelIndexList cachedIndexes;
    QList<QTreeWidgetItemIterator*> iterators;

    mutable QBasicTimer sortPendingTimer;
    mutable bool skipPendingSort = false; // no sorting during internal operations
    bool inline executePendingSort() const;

    bool isChanging() const;

private:
    Q_DECLARE_PRIVATE(QTreeModel)
public:
    struct SkipSorting
    {
        const QTreeModel * const model;
        const bool previous;
        SkipSorting(const QTreeModel *m) : model(m), previous(model ? model->skipPendingSort : false)
        { if (model) model->skipPendingSort = true; }
        ~SkipSorting() { if (model) model->skipPendingSort = previous; }
    };
    friend struct SkipSorting;
};

QT_BEGIN_INCLUDE_NAMESPACE
#include "private/qabstractitemmodel_p.h"
QT_END_INCLUDE_NAMESPACE

class QTreeModelPrivate : public QAbstractItemModelPrivate
{
    Q_DECLARE_PUBLIC(QTreeModel)
    void executePendingOperations() const override;
};

class QTreeWidgetItemPrivate
{
public:
    QTreeWidgetItemPrivate(QTreeWidgetItem *item)
        : q(item), disabled(false), selected(false), hidden(false), rowGuess(-1),
          policy(QTreeWidgetItem::DontShowIndicatorWhenChildless) {}
    void propagateDisabled(QTreeWidgetItem *item);
    void updateHiddenStatus(QTreeWidgetItem *item, bool inserting);
    void sortChildren(int column, Qt::SortOrder order, bool climb);
    QTreeWidgetItem *q;
    QVariantList display;
    uint disabled : 1;
    uint selected : 1;
    uint hidden : 1;
    int rowGuess;
    QTreeWidgetItem::ChildIndicatorPolicy policy;
};


inline bool QTreeModel::executePendingSort() const
{
    if (!skipPendingSort && sortPendingTimer.isActive() && !isChanging()) {
        sortPendingTimer.stop();
        int column = view()->header()->sortIndicatorSection();
        Qt::SortOrder order = view()->header()->sortIndicatorOrder();
        QTreeModel *that = const_cast<QTreeModel*>(this);
        that->sort(column, order);
        return true;
    }
    return false;
}

class QTreeWidgetPrivate : public QTreeViewPrivate
{
    friend class QTreeModel;
    Q_DECLARE_PUBLIC(QTreeWidget)
public:
    QTreeWidgetPrivate() : QTreeViewPrivate(), explicitSortColumn(-1) {}
    void clearConnections();
    inline QTreeModel *treeModel() const { return qobject_cast<QTreeModel*>(model); }
    inline QModelIndex index(const QTreeWidgetItem *item, int column = 0) const
        { return treeModel()->index(item, column); }
    inline QTreeWidgetItem *item(const QModelIndex &index) const
        { return treeModel()->item(index); }
    void emitItemPressed(const QModelIndex &index);
    void emitItemClicked(const QModelIndex &index);
    void emitItemDoubleClicked(const QModelIndex &index);
    void emitItemActivated(const QModelIndex &index);
    void emitItemEntered(const QModelIndex &index);
    void emitItemChanged(const QModelIndex &index);
    void emitItemExpanded(const QModelIndex &index);
    void emitItemCollapsed(const QModelIndex &index);
    void emitCurrentItemChanged(const QModelIndex &previous, const QModelIndex &index);
    void sort();
    void dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight);
    void selectionChanged(const QItemSelection &selected, const QItemSelection &deselected);

     // used by QTreeWidgetItem::sortChildren to make sure the column argument is used
    int explicitSortColumn;

    std::array<QMetaObject::Connection, 12> connections;
    std::optional<Qt::DropActions> supportedDragActions;
};

QT_END_NAMESPACE

#endif // QTREEWIDGET_P_H
