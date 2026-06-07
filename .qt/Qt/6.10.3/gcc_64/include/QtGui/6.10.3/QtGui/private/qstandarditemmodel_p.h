// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QSTANDARDITEMMODEL_P_H
#define QSTANDARDITEMMODEL_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of other Qt classes.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtGui/qstandarditemmodel.h>

#include <QtGui/private/qtguiglobal_p.h>
#include "private/qabstractitemmodel_p.h"

#include <QtCore/qlist.h>
#include <QtCore/qstack.h>
#include <QtCore/qvariant.h>
#include <QtCore/qdebug.h>

QT_REQUIRE_CONFIG(standarditemmodel);

QT_BEGIN_NAMESPACE

class QStandardItemData
{
public:
    inline QStandardItemData() : role(-1) {}
    inline QStandardItemData(int r, const QVariant &v) :
          role(r == Qt::EditRole ? Qt::DisplayRole : r), value(v) {}
    inline QStandardItemData(const std::pair<const int&, const QVariant&> &p) :
          role(p.first == Qt::EditRole ? Qt::DisplayRole : p.first), value(p.second) {}
    int role;
    QVariant value;
    inline bool operator==(const QStandardItemData &other) const { return role == other.role && value == other.value; }
};
Q_DECLARE_TYPEINFO(QStandardItemData, Q_RELOCATABLE_TYPE);

#ifndef QT_NO_DATASTREAM

inline QDataStream &operator>>(QDataStream &in, QStandardItemData &data)
{
    in >> data.role;
    in >> data.value;
    return in;
}

inline QDataStream &operator<<(QDataStream &out, const QStandardItemData &data)
{
    out << data.role;
    out << data.value;
    return out;
}

inline QDebug &operator<<(QDebug &debug, const QStandardItemData &data)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << data.role
                    << " "
                    << data.value;
    return debug.space();
}

#endif // QT_NO_DATASTREAM

class QStandardItemPrivate
{
    Q_DECLARE_PUBLIC(QStandardItem)
public:
    inline QStandardItemPrivate()
        : model(nullptr),
          parent(nullptr),
          rows(0),
          columns(0),
          q_ptr(nullptr),
          lastKnownIndex(-1)
        { }

    inline int childIndex(int row, int column) const {
        if ((row < 0) || (column < 0)
            || (row >= rowCount()) || (column >= columnCount())) {
            return -1;
        }
        return (row * columnCount()) + column;
    }
    inline int childIndex(const QStandardItem *child) const {
        const int lastChild = children.size() - 1;
        int &childsLastIndexInParent = child->d_func()->lastKnownIndex;
        if (childsLastIndexInParent != -1 && childsLastIndexInParent <= lastChild) {
            if (children.at(childsLastIndexInParent) == child)
                return childsLastIndexInParent;
        } else {
            childsLastIndexInParent = lastChild / 2;
        }

        // assuming the item is in the vicinity of the previous index, iterate forwards and
        // backwards through the children
        int backwardIter = childsLastIndexInParent - 1;
        int forwardIter = childsLastIndexInParent;
        for (;;) {
            if (forwardIter <= lastChild) {
                if (children.at(forwardIter) == child) {
                    childsLastIndexInParent = forwardIter;
                    break;
                }
                ++forwardIter;
            } else if (backwardIter < 0) {
                childsLastIndexInParent = -1;
                break;
            }
            if (backwardIter >= 0) {
                if (children.at(backwardIter) == child) {
                    childsLastIndexInParent = backwardIter;
                    break;
                }
                --backwardIter;
            }
        }
        return childsLastIndexInParent;
    }
    std::pair<int, int> position() const;
    void setChild(int row, int column, QStandardItem *item,
                  bool emitChanged = false);
    inline int rowCount() const {
        return rows;
    }
    inline int columnCount() const {
        return columns;
    }
    void childDeleted(QStandardItem *child);

    void setModel(QStandardItemModel *mod);

    inline void setParentAndModel(
        QStandardItem *par,
        QStandardItemModel *mod) {
        setModel(mod);
        parent = par;
    }

    void changeFlags(bool enable, Qt::ItemFlags f);
    void setItemData(const QMap<int, QVariant> &roles);
    QMap<int, QVariant> itemData() const;

    bool insertRows(int row, int count, const QList<QStandardItem*> &items);
    bool insertRows(int row, const QList<QStandardItem*> &items);
    bool insertColumns(int column, int count, const QList<QStandardItem*> &items);

    void sortChildren(int column, Qt::SortOrder order);

    QStandardItemModel *model;
    QStandardItem *parent;
    QList<QStandardItemData> values;
    QList<QStandardItem *> children;
    int rows;
    int columns;

    QStandardItem *q_ptr;

    mutable int lastKnownIndex; // this is a cached value
};

class QStandardItemModelPrivate : public QAbstractItemModelPrivate
{
    Q_DECLARE_PUBLIC(QStandardItemModel)

public:
    QStandardItemModelPrivate();
    ~QStandardItemModelPrivate();

    void init();

    inline QStandardItem *createItem() const {
        return itemPrototype ? itemPrototype->clone() : new QStandardItem;
    }

    inline QStandardItem *itemFromIndex(const QModelIndex &index) const {
        Q_Q(const QStandardItemModel);
        if (!index.isValid())
            return root.data();
        if (index.model() != q)
            return nullptr;
        QStandardItem *parent = static_cast<QStandardItem*>(index.internalPointer());
        if (parent == nullptr)
            return nullptr;
        return parent->child(index.row(), index.column());
    }

    void sort(QStandardItem *parent, int column, Qt::SortOrder order);
    void itemChanged(QStandardItem *item, const QList<int> &roles = QList<int>());
    void rowsAboutToBeInserted(QStandardItem *parent, int start, int end);
    void columnsAboutToBeInserted(QStandardItem *parent, int start, int end);
    void rowsAboutToBeRemoved(QStandardItem *parent, int start, int end);
    void columnsAboutToBeRemoved(QStandardItem *parent, int start, int end);
    void rowsInserted(QStandardItem *parent, int row, int count);
    void columnsInserted(QStandardItem *parent, int column, int count);
    void rowsRemoved(QStandardItem *parent, int row, int count);
    void columnsRemoved(QStandardItem *parent, int column, int count);

    void _q_emitItemChanged(const QModelIndex &topLeft,
                            const QModelIndex &bottomRight);

    void decodeDataRecursive(QDataStream &stream, QStandardItem *item);

    QList<QStandardItem *> columnHeaderItems;
    QList<QStandardItem *> rowHeaderItems;
    QHash<int, QByteArray> roleNames;
    QScopedPointer<QStandardItem> root;
    const QStandardItem *itemPrototype;
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(QStandardItemModelPrivate, int, sortRole, Qt::DisplayRole)
};

QT_END_NAMESPACE

#endif // QSTANDARDITEMMODEL_P_H
