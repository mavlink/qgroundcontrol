// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQMLSORTFILTERPROXYMODEL_P_P_H
#define QQMLSORTFILTERPROXYMODEL_P_P_H

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

#include <QtCore/qitemselectionmodel.h>
#include <QtQml/private/qqmlcustomparser_p.h>
#include <QtQmlModels/private/qtqmlmodelsglobal_p.h>
#include <QtQmlModels/private/qsortfilterproxymodelhelper_p.h>
#include <QtQmlModels/private/qqmlsortfilterproxymodel_p.h>

QT_BEGIN_NAMESPACE

class QQmlSortFilterProxyModelPrivate : public QAbstractProxyModelPrivate, public QSortFilterProxyModelHelper
{
    Q_DECLARE_PUBLIC(QQmlSortFilterProxyModel)

public:
    void init();

    bool containRoleForRecursiveFilter(const QList<int> &roles) const;
    bool recursiveParentAcceptsRow(const QModelIndex &source_parent) const;
    bool recursiveChildAcceptsRow(int source_row, const QModelIndex &source_parent) const;

    QList<std::pair<int, QList<int>>> proxy_intervals_for_source_items_to_add(
            const QList<int> &proxy_to_source, const QList<int> &source_items,
            const QModelIndex &source_parent, QSortFilterProxyModelHelper::Direction direction) const override;
    bool needsReorder(const QList<int> &source_rows, const QModelIndex &source_parent) const;
    bool updatePrimaryColumn();
    int findPrimarySortColumn() const;

    void changePersistentIndexList(const QModelIndexList &from, const QModelIndexList &to) override;
    void beginInsertRows(const QModelIndex &parent, int first, int last) override;
    void beginInsertColumns(const QModelIndex &parent, int first, int last) override;
    void endInsertRows() override;
    void endInsertColumns() override;
    void beginRemoveRows(const QModelIndex &parent, int first, int last) override;
    void beginRemoveColumns(const QModelIndex &parent, int first, int last) override;
    void endRemoveRows() override;
    void endRemoveColumns() override;
    void beginResetModel() override;
    void endResetModel() override;

    // Update the proxy model when there is any change in the source model
    void _q_sourceDataChanged(const QModelIndex &source_top_left,
                              const QModelIndex &source_bottom_right,
                              const QList<int> &roles);
    void _q_sourceHeaderDataChanged(Qt::Orientation orientation,
                                    int start, int end);
    void _q_sourceAboutToBeReset();
    void _q_sourceReset();
    void _q_clearMapping();
    void _q_sourceLayoutAboutToBeChanged(const QList<QPersistentModelIndex> &sourceParents,
                                         QAbstractItemModel::LayoutChangeHint hint);
    void _q_sourceLayoutChanged(const QList<QPersistentModelIndex> &sourceParents,
                                QAbstractItemModel::LayoutChangeHint hint);
    void _q_sourceRowsAboutToBeInserted(const QModelIndex &source_parent,
                                        int start, int end);
    void _q_sourceRowsInserted(const QModelIndex &source_parent,
                               int start, int end);
    void _q_sourceRowsAboutToBeRemoved(const QModelIndex &source_parent,
                                       int start, int end);
    void _q_sourceRowsRemoved(const QModelIndex &source_parent,
                              int start, int end);
    void _q_sourceRowsAboutToBeMoved(const QModelIndex &sourceParent,
                                     int sourceStart, int sourceEnd,
                                     const QModelIndex &destParent, int dest);
    void _q_sourceRowsMoved(const QModelIndex &sourceParent,
                            int sourceStart, int sourceEnd,
                            const QModelIndex &destParent, int dest);
    void _q_sourceColumnsAboutToBeInserted(const QModelIndex &source_parent,
                                           int start, int end);
    void _q_sourceColumnsInserted(const QModelIndex &source_parent,
                                  int start, int end);
    void _q_sourceColumnsAboutToBeRemoved(const QModelIndex &source_parent,
                                          int start, int end);
    void _q_sourceColumnsRemoved(const QModelIndex &source_parent,
                                 int start, int end);
    void _q_sourceColumnsAboutToBeMoved(const QModelIndex &sourceParent,
                                        int sourceStart, int sourceEnd,
                                        const QModelIndex &destParent, int dest);
    void _q_sourceColumnsMoved(const QModelIndex &sourceParent,
                               int sourceStart, int sourceEnd,
                               const QModelIndex &destParent, int dest);

    const QAbstractProxyModel *proxyModel() const override { return q_func(); }
    QModelIndex createIndex(int row, int column,
                            QHash<QtPrivate::QModelIndexWrapper, QSortFilterProxyModelHelper::Mapping *>::const_iterator it) const override;
    bool filterAcceptsRowInternal(int sourceRow, const QModelIndex &sourceIndex) const override;
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;
    bool filterAcceptsColumnInternal(int sourceColumn, const QModelIndex &sourceIndex) const override;
    bool filterAcceptsColumn(int source_column, const QModelIndex &source_parent) const override;
    void sort_source_rows(QList<int> &source_rows, const QModelIndex &source_parent) const override;
    bool lessThan(const QModelIndex &source_left, const QModelIndex &source_right) const override;

    // Internal
    QModelIndex m_lastTopSource;
    QRowsRemoval m_itemsBeingRemoved;
    bool m_completeInsert = false;
    QModelIndexPairList m_savedPersistentIndexes;
    QList<QPersistentModelIndex> m_savedLayoutChangeParents;
    std::array<QMetaObject::Connection, 18> m_sourceConnections;
    bool m_componentCompleted = false;

    // Properties exposed to the user
    QQmlFilterCompositor* m_filters;
    QQmlSorterCompositor* m_sorters;
    bool m_dynamicSortFilter = true;
    bool m_recursiveFiltering = false;
    bool m_autoAcceptChildRows = false;
    int m_primarySortColumn = -1;
    int m_proxySortColumn = -1;
    Qt::SortOrder m_sortOrder = Qt::AscendingOrder;
    QVariant m_sourceModel;
};

QT_END_NAMESPACE

#endif // QQMLSORTFILTERPROXYMODEL_P_P_H
