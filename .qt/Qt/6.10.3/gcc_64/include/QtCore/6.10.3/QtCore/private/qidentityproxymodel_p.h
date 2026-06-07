// Copyright (C) 2021 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Stephen Kelly <stephen.kelly@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QIDENTITYPROXYMODEL_P_H
#define QIDENTITYPROXYMODEL_P_H

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

#include <QtCore/private/qabstractproxymodel_p.h>
#include <QtCore/qidentityproxymodel.h>

QT_BEGIN_NAMESPACE

class Q_CORE_EXPORT QIdentityProxyModelPrivate : public QAbstractProxyModelPrivate
{
    Q_DECLARE_PUBLIC(QIdentityProxyModel)

public:
    QIdentityProxyModelPrivate()
    {
    }
    ~QIdentityProxyModelPrivate() override;

    QList<QPersistentModelIndex> layoutChangePersistentIndexes;
    QModelIndexList proxyIndexes;

    void sourceRowsAboutToBeInserted(const QModelIndex &parent, int start, int end);
    void sourceRowsInserted(const QModelIndex &parent, int start, int end);
    void sourceRowsAboutToBeRemoved(const QModelIndex &parent, int start, int end);
    void sourceRowsRemoved(const QModelIndex &parent, int start, int end);
    void sourceRowsAboutToBeMoved(const QModelIndex &sourceParent, int sourceStart, int sourceEnd,
                                  const QModelIndex &destParent, int dest);
    void sourceRowsMoved(const QModelIndex &sourceParent, int sourceStart, int sourceEnd,
                         const QModelIndex &destParent, int dest);

    void sourceColumnsAboutToBeInserted(const QModelIndex &parent, int start, int end);
    void sourceColumnsInserted(const QModelIndex &parent, int start, int end);
    void sourceColumnsAboutToBeRemoved(const QModelIndex &parent, int start, int end);
    void sourceColumnsRemoved(const QModelIndex &parent, int start, int end);
    void sourceColumnsAboutToBeMoved(const QModelIndex &sourceParent, int sourceStart,
                                     int sourceEnd, const QModelIndex &destParent, int dest);
    void sourceColumnsMoved(const QModelIndex &sourceParent, int sourceStart, int sourceEnd,
                            const QModelIndex &destParent, int dest);

    void sourceDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight,
                           const QList<int> &roles);
    void sourceHeaderDataChanged(Qt::Orientation orientation, int first, int last);

    void sourceLayoutAboutToBeChanged(const QList<QPersistentModelIndex> &sourceParents,
                                      QAbstractItemModel::LayoutChangeHint hint);
    void sourceLayoutChanged(const QList<QPersistentModelIndex> &sourceParents,
                             QAbstractItemModel::LayoutChangeHint hint);
    void sourceModelAboutToBeReset();
    void sourceModelReset();

private:
    bool m_handleLayoutChanges = true;
    bool m_handleDataChanges = true;
    QVarLengthArray<QMetaObject::Connection, 18> m_sourceModelConnections;
};

QT_END_NAMESPACE

#endif // QIDENTITYPROXYMODEL_P_H
