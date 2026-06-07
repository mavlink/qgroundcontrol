// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QLISTWIDGET_P_H
#define QLISTWIDGET_P_H

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
#include <QtWidgets/qabstractitemview.h>
#include <QtWidgets/qlistwidget.h>
#include <private/qlistview_p.h>
#include <private/qwidgetitemdata_p.h>

#include <array>

QT_REQUIRE_CONFIG(listwidget);

QT_BEGIN_NAMESPACE

class QListModelLessThan
{
public:
    inline bool operator()(QListWidgetItem *i1, QListWidgetItem *i2) const
        { return *i1 < *i2; }
};

class QListModelGreaterThan
{
public:
    inline bool operator()(QListWidgetItem *i1, QListWidgetItem *i2) const
        { return *i2 < *i1; }
};

class Q_AUTOTEST_EXPORT QListModel : public QAbstractListModel
{
    Q_OBJECT
    friend class QListWidget;

public:
    QListModel(QListWidget *parent);
    ~QListModel();

    inline QListWidget *view() const { return qobject_cast<QListWidget *>(QObject::parent()); }

    void clear();
    QListWidgetItem *at(int row) const;
    void insert(int row, QListWidgetItem *item);
    void insert(int row, const QStringList &items);
    void remove(QListWidgetItem *item);
    QListWidgetItem *take(int row);
    void move(int srcRow, int dstRow);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    QModelIndex index(const QListWidgetItem *item) const;
    QModelIndex index(int row, int column = 0, const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    bool clearItemData(const QModelIndex &index) override;

    QMap<int, QVariant> itemData(const QModelIndex &index) const override;

    bool insertRows(int row, int count = 1, const QModelIndex &parent = QModelIndex()) override;
    bool removeRows(int row, int count = 1, const QModelIndex &parent = QModelIndex()) override;
    bool moveRows(const QModelIndex &sourceParent, int sourceRow, int count, const QModelIndex &destinationParent, int destinationChild) override;

    Qt::ItemFlags flags(const QModelIndex &index) const override;

    void sort(int column, Qt::SortOrder order) override;
    void ensureSorted(int column, Qt::SortOrder order, int start, int end);
    static bool itemLessThan(const std::pair<QListWidgetItem*,int> &left,
                             const std::pair<QListWidgetItem*,int> &right);
    static bool itemGreaterThan(const std::pair<QListWidgetItem*,int> &left,
                                const std::pair<QListWidgetItem*,int> &right);
    static QList<QListWidgetItem*>::iterator sortedInsertionIterator(
        const QList<QListWidgetItem*>::iterator &begin,
        const QList<QListWidgetItem*>::iterator &end,
        Qt::SortOrder order, QListWidgetItem *item);

    void itemChanged(QListWidgetItem *item, const QList<int> &roles = QList<int>());

    // dnd
    QStringList mimeTypes() const override;
    QMimeData *mimeData(const QModelIndexList &indexes) const override;
#if QT_CONFIG(draganddrop)
    bool dropMimeData(const QMimeData *data, Qt::DropAction action,
                      int row, int column, const QModelIndex &parent) override;
    Qt::DropActions supportedDropActions() const override;
    Qt::DropActions supportedDragActions() const override;
#endif
    QMimeData *internalMimeData()  const;
private:
    QList<QListWidgetItem*> items;

    // A cache must be mutable if get-functions should have const modifiers
    mutable QModelIndexList cachedIndexes;
};



class QListWidgetPrivate : public QListViewPrivate
{
    Q_DECLARE_PUBLIC(QListWidget)
public:
    QListWidgetPrivate() : QListViewPrivate(), sortOrder(Qt::AscendingOrder), sortingEnabled(false) {}
    inline QListModel *listModel() const { return qobject_cast<QListModel*>(model); }
    void setup();
    void clearConnections();
    void emitItemPressed(const QModelIndex &index);
    void emitItemClicked(const QModelIndex &index);
    void emitItemDoubleClicked(const QModelIndex &index);
    void emitItemActivated(const QModelIndex &index);
    void emitItemEntered(const QModelIndex &index);
    void emitItemChanged(const QModelIndex &index);
    void emitCurrentItemChanged(const QModelIndex &current, const QModelIndex &previous);
    void sort();
    void dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight);

    Qt::SortOrder sortOrder;
    bool sortingEnabled;
    std::optional<Qt::DropActions> supportedDragActions;
    std::array<QMetaObject::Connection, 8> connections;
    std::array<QMetaObject::Connection, 2> selectionModelConnections;
};

class QListWidgetItemPrivate
{
public:
    QListWidgetItemPrivate(QListWidgetItem *item) : q(item), theid(-1) {}
    QListWidgetItem *q;
    QList<QWidgetItemData> values;
    int theid;
};

QT_END_NAMESPACE

#endif // QLISTWIDGET_P_H
