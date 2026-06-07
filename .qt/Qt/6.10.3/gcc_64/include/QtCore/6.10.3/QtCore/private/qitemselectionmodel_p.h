// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QITEMSELECTIONMODEL_P_H
#define QITEMSELECTIONMODEL_P_H

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

#include "qitemselectionmodel.h"
#include "private/qobject_p.h"
#include "private/qproperty_p.h"
#include <array>

QT_REQUIRE_CONFIG(itemmodel);

QT_BEGIN_NAMESPACE

class QItemSelectionModelPrivate: public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QItemSelectionModel)
public:
    QItemSelectionModelPrivate()
      : currentCommand(QItemSelectionModel::NoUpdate),
        tableSelected(false), tableColCount(0), tableRowCount(0) {}
    ~QItemSelectionModelPrivate() override;

    QItemSelection expandSelection(const QItemSelection &selection,
                                   QItemSelectionModel::SelectionFlags command) const;

    void initModel(QAbstractItemModel *model);

    void rowsAboutToBeRemoved(const QModelIndex &parent, int start, int end);
    void columnsAboutToBeRemoved(const QModelIndex &parent, int start, int end);
    void rowsAboutToBeInserted(const QModelIndex &parent, int start, int end);
    void columnsAboutToBeInserted(const QModelIndex &parent, int start, int end);
    void layoutAboutToBeChanged(const QList<QPersistentModelIndex> &parents, QAbstractItemModel::LayoutChangeHint hint);
    void triggerLayoutToBeChanged()
    {
        layoutAboutToBeChanged(QList<QPersistentModelIndex>(), QAbstractItemModel::NoLayoutChangeHint);
    }

    void layoutChanged(const QList<QPersistentModelIndex> &parents, QAbstractItemModel::LayoutChangeHint hint);
    void triggerLayoutChanged()
    {
        layoutChanged(QList<QPersistentModelIndex>(), QAbstractItemModel::NoLayoutChangeHint);
    }

    void modelDestroyed();

    inline void remove(QList<QItemSelectionRange> &r)
    {
        QList<QItemSelectionRange>::const_iterator it = r.constBegin();
        for (; it != r.constEnd(); ++it)
            ranges.removeAll(*it);
    }

    inline void finalize()
    {
        ranges.merge(currentSelection, currentCommand);
        if (!currentSelection.isEmpty())  // ### perhaps this should be in QList
            currentSelection.clear();
    }

    void setModel(QAbstractItemModel *mod) { q_func()->setModel(mod); }
    void disconnectModel();
    void modelChanged(QAbstractItemModel *mod) { Q_EMIT q_func()->modelChanged(mod); }
    Q_OBJECT_COMPAT_PROPERTY_WITH_ARGS(QItemSelectionModelPrivate, QAbstractItemModel *, model,
                                       &QItemSelectionModelPrivate::setModel,
                                       &QItemSelectionModelPrivate::modelChanged, nullptr)

    QItemSelection ranges;
    QItemSelection currentSelection;
    QPersistentModelIndex currentIndex;
    QItemSelectionModel::SelectionFlags currentCommand;
    QList<QPersistentModelIndex> savedPersistentIndexes;
    QList<QPersistentModelIndex> savedPersistentCurrentIndexes;
    QList<std::pair<QPersistentModelIndex, uint>> savedPersistentRowLengths;
    QList<std::pair<QPersistentModelIndex, uint>> savedPersistentCurrentRowLengths;
    // optimization when all indexes are selected
    bool tableSelected;
    QPersistentModelIndex tableParent;
    int tableColCount, tableRowCount;
    std::array<QMetaObject::Connection, 12> connections;
};

QT_END_NAMESPACE

#endif // QITEMSELECTIONMODEL_P_H
