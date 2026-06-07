// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSORTFILTERPROXYMODELHELPER_H
#define QSORTFILTERPROXYMODELHELPER_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of QAbstractItemModel*.  This header file may change from version
// to version without notice, or even be removed.
//
// We mean it.
//
//

#include <QtCore/private/qabstractitemmodel_p.h>
#include <QtCore/private/qabstractproxymodel_p.h>
#include <QtQmlModels/private/qtqmlmodelsglobal_p.h>

QT_BEGIN_NAMESPACE

class QQmlSortFilterProxyModelLessThan;
class QQmlSortFilterProxyModelGreaterThan;
class QQmlSortFilterProxyModel;

using QModelIndexPairList = QList<std::pair<QModelIndex, QPersistentModelIndex>>;

class Q_QMLMODELS_EXPORT QSortFilterProxyModelHelper
{
    friend class QQmlSortFilterProxyModelGreaterThan;
    friend class QQmlSortFilterProxyModelLessThan;

public:
    QSortFilterProxyModelHelper();
    virtual ~QSortFilterProxyModelHelper();

    static void setProperties(
            QVariant *target, const QQmlSortFilterProxyModel *proxyModel,
            const QModelIndex &sourceIndex);

    enum Direction {
        Rows = 0x01,
        Columns = 0x02,
        Both = Rows | Columns,
    };

    struct Mapping {
        QList<int> source_rows;
        QList<int> source_columns;
        QList<int> proxy_rows;
        QList<int> proxy_columns;
        QList<QModelIndex> mapped_children;
        QModelIndex source_parent;
    };

    using IndexMap = QHash<QtPrivate::QModelIndexWrapper, Mapping *>;
    mutable IndexMap source_index_mapping;

    static inline QSet<int> qListToSet(const QList<int> &vector) { return {vector.begin(), vector.end()}; }

    inline IndexMap::const_iterator index_to_iterator(
            const QModelIndex &proxy_index) const {
        Q_ASSERT(proxy_index.isValid());
        Q_ASSERT(proxy_index.model() == proxyModel());
        const void *p = proxy_index.internalPointer();
        Q_ASSERT(p);
        IndexMap::const_iterator it =
                source_index_mapping.constFind(static_cast<const Mapping*>(p)->source_parent);
        Q_ASSERT(it != source_index_mapping.constEnd());
        Q_ASSERT(it.value());
        return it;
    }

    // Core mapping APIs
    IndexMap::const_iterator create_mapping(const QModelIndex &source_parent) const;
    IndexMap::const_iterator create_mapping_recursive(const QModelIndex &source_parent) const;
    bool can_create_mapping(const QModelIndex &source_parent) const;
    void remove_from_mapping(const QModelIndex &source_parent);
    void clearSourceIndexMapping();
    QModelIndex source_to_proxy(const QModelIndex &source_index) const;
    void build_source_to_proxy_mapping(
                QList<int> &proxy_to_source, QList<int> &source_to_proxy, int start = 0) const;
    QModelIndex proxy_to_source(const QModelIndex &proxy_index) const;
    void updateChildrenMapping(const QModelIndex &source_parent, Mapping *parent_mapping,
                                Direction direction, int start, int end, int delta_item_count, bool remove);
    void proxy_item_range(const QList<int> &source_to_proxy, const QList<int> &source_items,
                int &proxy_low, int &proxy_high) const;

    // Model update APIs
    QModelIndexPairList store_persistent_indexes() const;
    void update_persistent_indexes(const QModelIndexPairList &source_indexes);

    // Sort filter proxy model update APIs
    virtual void filter_changed(Direction dir = Direction::Both,
                                const QModelIndex &source_parent = QModelIndex());
    virtual QSet<int> handle_filter_changed(QList<int> &source_to_proxy, QList<int> &proxy_to_source,
                const QModelIndex &source_parent, Direction direction);

    virtual void insert_source_items(QList<int> &source_to_proxy, QList<int> &proxy_to_source,
                            const QList<int> &source_items, const QModelIndex &source_parent,
                            Direction direction, bool emit_signal = true);
    virtual void source_items_inserted(const QModelIndex &source_parent,
                            int start, int end, Direction direction);
    virtual void source_items_about_to_be_removed(const QModelIndex &source_parent, int start,
                            int end, Direction direction);
    virtual void source_items_removed(const QModelIndex &source_parent, int start, int end,
                                      Direction direction);
    virtual void remove_source_items(QList<int> &source_to_proxy, QList<int> &proxy_to_source,
                            const QList<int> &source_items, const QModelIndex &source_parent,
                            Direction direction, bool emit_signal = true);
    virtual void remove_proxy_interval(QList<int> &source_to_proxy, QList<int> &proxy_to_source, int proxy_start, int proxy_end,
                            const QModelIndex &proxy_parent, Direction direction, bool emit_signal = true);

    virtual QList<std::pair<int, int>> proxy_intervals_for_source_items(const QList<int> &source_to_proxy,
                                                                const QList<int> &source_items) const;
    virtual QList<std::pair<int, QList<int>>> proxy_intervals_for_source_items_to_add(const QList<int> &,
                    const QList<int> &,const QModelIndex &, Direction) const { return {}; }
    virtual void sort();

protected:
    virtual const QAbstractProxyModel *proxyModel() const = 0;

    // Proxy model protected functions need to be overridden in the corresponding model
    virtual void beginInsertRows(const QModelIndex &, int, int) {};
    virtual void beginInsertColumns(const QModelIndex &, int, int) {};
    virtual void endInsertRows() {};
    virtual void endInsertColumns() {};
    virtual void beginRemoveRows(const QModelIndex &, int , int) {};
    virtual void beginRemoveColumns(const QModelIndex &, int , int) {};
    virtual void endRemoveRows() {};
    virtual void endRemoveColumns() {};
    virtual void beginResetModel() {};
    virtual void endResetModel() {};

    virtual QModelIndex createIndex(int , int , IndexMap::const_iterator ) const { return QModelIndex(); }
    virtual void changePersistentIndexList(const QModelIndexList &, const QModelIndexList &) { };
    virtual bool filterAcceptsRowInternal(int , const QModelIndex &) const { return true; }
    virtual bool filterAcceptsRow(int , const QModelIndex &) const { return true; }
    virtual bool filterAcceptsColumnInternal(int , const QModelIndex &) const { return true; }
    virtual bool filterAcceptsColumn(int , const QModelIndex &) const { return true; }
    virtual void sort_source_rows(QList<int> &, const QModelIndex &) const {}
    virtual bool lessThan(const QModelIndex&, const QModelIndex &) const { return true; }
};

struct QSortFilterProxyModelDataChanged
{
    QSortFilterProxyModelDataChanged(const QModelIndex &tl, const QModelIndex &br)
        : topLeft(tl), bottomRight(br) { }

    QModelIndex topLeft;
    QModelIndex bottomRight;
};

class QQmlSortFilterProxyModelLessThan
{
public:
    inline QQmlSortFilterProxyModelLessThan(int column, const QModelIndex &parent,
                                         const QAbstractItemModel *source,
                                         const QSortFilterProxyModelHelper *helper)
        : sort_column(column), source_parent(parent), source_model(source), proxy_model(helper) {}

    inline bool operator()(int r1, int r2) const
    {
        QModelIndex i1 = source_model->index(r1, sort_column, source_parent);
        QModelIndex i2 = source_model->index(r2, sort_column, source_parent);
        return proxy_model->lessThan(i1, i2);
    }

private:
    int sort_column;
    QModelIndex source_parent;
    const QAbstractItemModel *source_model;
    const QSortFilterProxyModelHelper *proxy_model;
};

class QQmlSortFilterProxyModelGreaterThan
{
public:
    inline QQmlSortFilterProxyModelGreaterThan(int column, const QModelIndex &parent,
                                            const QAbstractItemModel *source,
                                            const QSortFilterProxyModelHelper *helper)
        : sort_column(column), source_parent(parent),
          source_model(source), proxy_model(helper) {}

    inline bool operator()(int r1, int r2) const
    {
        QModelIndex i1 = source_model->index(r1, sort_column, source_parent);
        QModelIndex i2 = source_model->index(r2, sort_column, source_parent);
        return proxy_model->lessThan(i2, i1);
    }

private:
    int sort_column;
    QModelIndex source_parent;
    const QAbstractItemModel *source_model;
    const QSortFilterProxyModelHelper *proxy_model;
};

//this struct is used to store what are the rows that are removed
//between a call to rowsAboutToBeRemoved and rowsRemoved
//it avoids readding rows to the mapping that are currently being removed
struct QRowsRemoval
{
    QRowsRemoval(const QModelIndex &parent_source, int start, int end) : parent_source(parent_source), start(start), end(end)
    {
    }

    QRowsRemoval() : start(-1), end(-1)
    {
    }

    bool contains(QModelIndex parent, int row) const
    {
        do {
            if (parent == parent_source)
                return row >= start && row <= end;
            row = parent.row();
            parent = parent.parent();
        } while (row >= 0);
        return false;
    }
private:
    QModelIndex parent_source;
    int start;
    int end;
};

QT_END_NAMESPACE

#endif // QSORTFILTERPROXYMODELHELPER_H
