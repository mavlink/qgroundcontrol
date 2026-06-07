// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QTABLEWIDGET_P_H
#define QTABLEWIDGET_P_H

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
#include <qheaderview.h>
#include <qtablewidget.h>
#include <qabstractitemmodel.h>
#include <private/qabstractitemmodel_p.h>
#include <private/qtableview_p.h>
#include <private/qwidgetitemdata_p.h>

#include <array>

QT_REQUIRE_CONFIG(tablewidget);

QT_BEGIN_NAMESPACE

class QTableWidgetMimeData : public QMimeData
{
    Q_OBJECT
public:
    QList<QTableWidgetItem*> items;
};

class QTableModelLessThan
{
public:
    inline bool operator()(QTableWidgetItem *i1, QTableWidgetItem *i2) const
        { return (*i1 < *i2); }
};

class QTableModelGreaterThan
{
public:
    inline bool operator()(QTableWidgetItem *i1, QTableWidgetItem *i2) const
        { return (*i2 < *i1); }
};

class QTableModel : public QAbstractTableModel
{
    Q_OBJECT
    friend class QTableWidget;

public:
    QTableModel(int rows, int columns, QTableWidget *parent);
    ~QTableModel();

    inline QTableWidget *view() const { return qobject_cast<QTableWidget *>(QObject::parent()); }

    bool insertRows(int row, int count = 1, const QModelIndex &parent = QModelIndex()) override;
    bool insertColumns(int column, int count = 1, const QModelIndex &parent = QModelIndex()) override;

    bool removeRows(int row, int count = 1, const QModelIndex &parent = QModelIndex()) override;
    bool removeColumns(int column, int count = 1, const QModelIndex &parent = QModelIndex()) override;

    bool moveRows(const QModelIndex &sourceParent, int sourceRow, int count, const QModelIndex &destinationParent, int destinationChild) override;

    void setItem(int row, int column, QTableWidgetItem *item);
    QTableWidgetItem *takeItem(int row, int column);
    QTableWidgetItem *item(int row, int column) const;
    QTableWidgetItem *item(const QModelIndex &index) const;
    void removeItem(QTableWidgetItem *item);

    void setHorizontalHeaderItem(int section, QTableWidgetItem *item);
    void setVerticalHeaderItem(int section, QTableWidgetItem *item);
    QTableWidgetItem *takeHorizontalHeaderItem(int section);
    QTableWidgetItem *takeVerticalHeaderItem(int section);
    QTableWidgetItem *horizontalHeaderItem(int section);
    QTableWidgetItem *verticalHeaderItem(int section);

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override
        { return QAbstractTableModel::index(row, column, parent); }

    QModelIndex index(const QTableWidgetItem *item) const;

    void setRowCount(int rows);
    void setColumnCount(int columns);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    bool setItemData(const QModelIndex &index, const QMap<int, QVariant> &roles) override;
    bool clearItemData(const QModelIndex &index) override;

    QMap<int, QVariant> itemData(const QModelIndex &index) const override;

    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    bool setHeaderData(int section, Qt::Orientation orientation, const QVariant &value, int role) override;

    Qt::ItemFlags flags(const QModelIndex &index) const override;

    void sort(int column, Qt::SortOrder order) override;
    static bool itemLessThan(const std::pair<QTableWidgetItem*,int> &left,
                             const std::pair<QTableWidgetItem*,int> &right);
    static bool itemGreaterThan(const std::pair<QTableWidgetItem*,int> &left,
                                const std::pair<QTableWidgetItem*,int> &right);

    void ensureSorted(int column, Qt::SortOrder order, int start, int end);
    QList<QTableWidgetItem *> columnItems(int column) const;
    void updateRowIndexes(QModelIndexList &indexes, int movedFromRow, int movedToRow);
    static QList<QTableWidgetItem *>::iterator
    sortedInsertionIterator(const QList<QTableWidgetItem *>::iterator &begin,
                            const QList<QTableWidgetItem *>::iterator &end, Qt::SortOrder order,
                            QTableWidgetItem *item);

    bool isValid(const QModelIndex &index) const;
    inline long tableIndex(int row, int column) const
        { return (row * horizontalHeaderItems.size()) + column; }

    void clear();
    void clearContents();
    void itemChanged(QTableWidgetItem *item, const QList<int> &roles = QList<int>());

    QTableWidgetItem *createItem() const;
    const QTableWidgetItem *itemPrototype() const;
    void setItemPrototype(const QTableWidgetItem *item);

    // dnd
    QStringList mimeTypes() const override;
    QMimeData *mimeData(const QModelIndexList &indexes) const override;
    bool dropMimeData(const QMimeData *data, Qt::DropAction action,
            int row, int column, const QModelIndex &parent) override;
    Qt::DropActions supportedDropActions() const override;
    Qt::DropActions supportedDragActions() const override;

    QMimeData *internalMimeData()  const;

private:
    const QTableWidgetItem *prototype;
    QList<QTableWidgetItem *> tableItems;
    QList<QTableWidgetItem *> verticalHeaderItems;
    QList<QTableWidgetItem *> horizontalHeaderItems;

    // A cache must be mutable if get-functions should have const modifiers
    mutable QModelIndexList cachedIndexes;
};

class QTableWidgetPrivate : public QTableViewPrivate
{
    Q_DECLARE_PUBLIC(QTableWidget)
public:
    QTableWidgetPrivate() : QTableViewPrivate() {}
    inline QTableModel *tableModel() const { return qobject_cast<QTableModel*>(model); }
    void setup();
    void clearConnections();

    // view signals
    void emitItemPressed(const QModelIndex &index);
    void emitItemClicked(const QModelIndex &index);
    void emitItemDoubleClicked(const QModelIndex &index);
    void emitItemActivated(const QModelIndex &index);
    void emitItemEntered(const QModelIndex &index);
    // model signals
    void emitItemChanged(const QModelIndex &index);
    // selection signals
    void emitCurrentItemChanged(const QModelIndex &previous, const QModelIndex &current);
    // sorting
    void sort();
    void dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight);

    std::array<QMetaObject::Connection, 10> connections;
    std::optional<Qt::DropActions> supportedDragActions;
};

class QTableWidgetItemPrivate
{
public:
    QTableWidgetItemPrivate(QTableWidgetItem *item) : q(item), id(-1), headerItem(false) {}
    QTableWidgetItem *q;
    int id;
    bool headerItem; // Qt 7 TODO: inline this stuff in the public class.
};

QT_END_NAMESPACE

#endif // QTABLEWIDGET_P_H
