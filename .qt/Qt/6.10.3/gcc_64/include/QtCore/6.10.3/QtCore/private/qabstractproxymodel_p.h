// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QABSTRACTPROXYMODEL_P_H
#define QABSTRACTPROXYMODEL_P_H

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

#include "qabstractproxymodel.h"
#include "private/qabstractitemmodel_p.h"
#include "private/qproperty_p.h"

QT_REQUIRE_CONFIG(proxymodel);

QT_BEGIN_NAMESPACE

class Q_CORE_EXPORT QAbstractProxyModelPrivate : public QAbstractItemModelPrivate
{
    Q_DECLARE_PUBLIC(QAbstractProxyModel)
public:
    QAbstractProxyModelPrivate()
        : QAbstractItemModelPrivate(),
        sourceHadZeroRows(false),
        sourceHadZeroColumns(false),
        updateVerticalHeader(false),
        updateHorizontalHeader(false)
    {}
    void setModelForwarder(QAbstractItemModel *sourceModel)
    {
        q_func()->setSourceModel(sourceModel);
    }
    void modelChangedForwarder()
    {
        Q_EMIT q_func()->sourceModelChanged(QAbstractProxyModel::QPrivateSignal());
    }
    QAbstractItemModel *getModelForwarder() const { return q_func()->sourceModel(); }

    Q_OBJECT_COMPAT_PROPERTY_WITH_ARGS(QAbstractProxyModelPrivate, QAbstractItemModel *, model,
                                       &QAbstractProxyModelPrivate::setModelForwarder,
                                       &QAbstractProxyModelPrivate::modelChangedForwarder,
                                       &QAbstractProxyModelPrivate::getModelForwarder, nullptr)
    virtual void _q_sourceModelDestroyed();
    void _q_sourceModelRowsAboutToBeInserted(const QModelIndex &parent, int first, int last);
    void _q_sourceModelRowsInserted(const QModelIndex &parent, int first, int last);
    void _q_sourceModelRowsRemoved(const QModelIndex &parent, int first, int last);
    void _q_sourceModelColumnsAboutToBeInserted(const QModelIndex &parent, int first, int last);
    void _q_sourceModelColumnsInserted(const QModelIndex &parent, int first, int last);
    void _q_sourceModelColumnsRemoved(const QModelIndex &parent, int first, int last);

    void mapDropCoordinatesToSource(int row, int column, const QModelIndex &parent,
                                    int *source_row, int *source_column, QModelIndex *source_parent) const;

    void scheduleHeaderUpdate(Qt::Orientation orientation);
    void emitHeaderDataChanged();

    unsigned int sourceHadZeroRows : 1;
    unsigned int sourceHadZeroColumns : 1;
    unsigned int updateVerticalHeader : 1;
    unsigned int updateHorizontalHeader : 1;
};

QT_END_NAMESPACE

#endif // QABSTRACTPROXYMODEL_P_H
