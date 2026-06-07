// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQmlTreeModelToTableModel_H
#define QQmlTreeModelToTableModel_H

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

#include "qtqmlmodelsglobal_p.h"

#include <QtCore/qset.h>
#include <QtCore/qpointer.h>
#include <QtCore/qabstractitemmodel.h>
#include <QtCore/qitemselectionmodel.h>
#include <QtCore/qvarlengtharray.h>

QT_BEGIN_NAMESPACE

class QAbstractItemModel;

class Q_QMLMODELS_EXPORT QQmlTreeModelToTableModel : public QAbstractItemModel
{
    Q_OBJECT
    Q_PROPERTY(QAbstractItemModel *model READ model WRITE setModel NOTIFY modelChanged FINAL)
    Q_PROPERTY(QModelIndex rootIndex READ rootIndex WRITE setRootIndex RESET resetRootIndex NOTIFY rootIndexChanged FINAL)

    struct TreeItem;

public:
    explicit QQmlTreeModelToTableModel(QObject *parent = nullptr);

    QAbstractItemModel *model() const;
    QModelIndex rootIndex() const;
    void setRootIndex(const QModelIndex &idx);
    void resetRootIndex();

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &child) const override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    enum {
        DepthRole = Qt::UserRole - 5,
        ExpandedRole,
        HasChildrenRole,
        HasSiblingRole,
        ModelIndexRole
    };

    QHash<int, QByteArray> roleNames() const override;
    QVariant data(const QModelIndex &, int role) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

    void clearModelData();

    bool isVisible(const QModelIndex &index);
    bool childrenVisible(const QModelIndex &index);

    QModelIndex mapToModel(const QModelIndex &index) const;
    QModelIndex mapFromModel(const QModelIndex &index) const;
    QModelIndex mapToModel(int row) const;

    Q_INVOKABLE QItemSelection selectionForRowRange(const QModelIndex &fromIndex, const QModelIndex &toIndex) const;

    void showModelTopLevelItems(bool doInsertRows = true);
    void showModelChildItems(const TreeItem &parent, int start, int end, bool doInsertRows = true, bool doExpandPendingRows = true);

    int itemIndex(const QModelIndex &index) const;
    void expandPendingRows(bool doInsertRows = true);
    int lastChildIndex(const QModelIndex &index) const;
    void removeVisibleRows(int startIndex, int endIndex, bool doRemoveRows = true);

    void dump() const;
    bool testConsistency(bool dumpOnFail = false) const;

    using QAbstractItemModel::hasChildren;

Q_SIGNALS:
    void modelChanged(QAbstractItemModel *model);
    void rootIndexChanged();
    void expanded(const QModelIndex &index);
    void collapsed(const QModelIndex &index);

public Q_SLOTS:
    void expand(const QModelIndex &);
    void collapse(const QModelIndex &);
    void setModel(QAbstractItemModel *model);
    bool isExpanded(const QModelIndex &) const;
    bool isExpanded(int row) const;
    bool hasChildren(int row) const;
    bool hasSiblings(int row) const;
    int depthAtRow(int row) const;
    void expandRow(int n);
    void expandRecursively(int row, int depth);
    void collapseRow(int n);
    void collapseRecursively(int row);

private Q_SLOTS:
    void modelHasBeenDestroyed();
    void modelHasBeenReset();
    void modelDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QList<int> &roles);
    void modelLayoutAboutToBeChanged(const QList<QPersistentModelIndex> &parents, QAbstractItemModel::LayoutChangeHint hint);
    void modelLayoutChanged(const QList<QPersistentModelIndex> &parents, QAbstractItemModel::LayoutChangeHint hint);
    void modelRowsAboutToBeInserted(const QModelIndex & parent, int start, int end);
    void modelRowsAboutToBeMoved(const QModelIndex & sourceParent, int sourceStart, int sourceEnd, const QModelIndex & destinationParent, int destinationRow);
    void modelRowsAboutToBeRemoved(const QModelIndex & parent, int start, int end);
    void modelRowsInserted(const QModelIndex & parent, int start, int end);
    void modelRowsMoved(const QModelIndex & sourceParent, int sourceStart, int sourceEnd, const QModelIndex & destinationParent, int destinationRow);
    void modelRowsRemoved(const QModelIndex & parent, int start, int end);
    void modelColumnsAboutToBeInserted(const QModelIndex & parent, int start, int end);
    void modelColumnsAboutToBeRemoved(const QModelIndex & parent, int start, int end);
    void modelColumnsInserted(const QModelIndex & parent, int start, int end);
    void modelColumnsRemoved(const QModelIndex & parent, int start, int end);

private:
    struct TreeItem {
        QPersistentModelIndex index;
        int depth;
        bool expanded;

        explicit TreeItem(const QModelIndex &idx = QModelIndex(), int d = 0, int e = false)
            : index(idx), depth(d), expanded(e)
        { }

        inline bool operator== (const TreeItem &other) const
        {
            return this->index == other.index;
        }
    };

    struct DataChangedParams {
        int top;
        int bottom;
        QVarLengthArray<int, 5> roles;
    };

    struct SignalFreezer {
        SignalFreezer(QQmlTreeModelToTableModel *parent) : m_parent(parent) {
            m_parent->enableSignalAggregation();
        }
        ~SignalFreezer() { m_parent->disableSignalAggregation(); }

    private:
        QQmlTreeModelToTableModel *m_parent;
    };

    void enableSignalAggregation();
    void disableSignalAggregation();
    bool isAggregatingSignals() const { return m_signalAggregatorStack > 0; }
    void queueDataChanged(int top, int bottom, std::initializer_list<int> roles);
    void emitQueuedSignals();
    void connectToModel();

    QPointer<QAbstractItemModel> m_model = nullptr;
    QPersistentModelIndex m_rootIndex;
    QList<TreeItem> m_items;
    QSet<QPersistentModelIndex> m_expandedItems;
    QList<TreeItem> m_itemsToExpand;
    mutable int m_lastItemIndex = 0;
    bool m_visibleRowsMoved = false;
    bool m_modelLayoutChanged = false;
    int m_signalAggregatorStack = 0;
    QList<DataChangedParams> m_queuedDataChanged;
    std::array<QMetaObject::Connection, 15> m_connections;
};

QT_END_NAMESPACE

#endif // QQmlTreeModelToTableModel_H
