// Copyright (C) 2018 Luca Beldi <v.ronin@yahoo.it>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QTRANSPOSEPROXYMODEL_P_H
#define QTRANSPOSEPROXYMODEL_P_H

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

#include "qtransposeproxymodel.h"
#include <private/qabstractproxymodel_p.h>

QT_BEGIN_NAMESPACE

class QTransposeProxyModelPrivate : public QAbstractProxyModelPrivate
{
    Q_DECLARE_PUBLIC(QTransposeProxyModel)
    Q_DISABLE_COPY(QTransposeProxyModelPrivate)
private:
    QTransposeProxyModelPrivate() = default;
    QList<QMetaObject::Connection> sourceConnections;
    QList<QPersistentModelIndex> layoutChangePersistentIndexes;
    QModelIndexList layoutChangeProxyIndexes;
    QModelIndex uncheckedMapToSource(const QModelIndex &proxyIndex) const;
    QModelIndex uncheckedMapFromSource(const QModelIndex &sourceIndex) const;
    void onLayoutChanged(const QList<QPersistentModelIndex> &parents, QAbstractItemModel::LayoutChangeHint hint);
    void onLayoutAboutToBeChanged(const QList<QPersistentModelIndex> &parents, QAbstractItemModel::LayoutChangeHint hint);
    void onDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QList<int> &roles);
    void onHeaderDataChanged(Qt::Orientation orientation, int first, int last);
    void onColumnsAboutToBeInserted(const QModelIndex &parent, int first, int last);
    void onColumnsAboutToBeRemoved(const QModelIndex &parent, int first, int last);
    void onColumnsAboutToBeMoved(const QModelIndex &sourceParent, int sourceStart, int sourceEnd, const QModelIndex &destinationParent, int destinationColumn);
    void onRowsAboutToBeInserted(const QModelIndex &parent, int first, int last);
    void onRowsAboutToBeRemoved(const QModelIndex &parent, int first, int last);
    void onRowsAboutToBeMoved(const QModelIndex &sourceParent, int sourceStart, int sourceEnd, const QModelIndex &destinationParent, int destinationRow);
};

QT_END_NAMESPACE

#endif //QTRANSPOSEPROXYMODEL_P_H
