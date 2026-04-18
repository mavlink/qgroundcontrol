/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "ObjectItemModelBase.h"

/// Base class for flat QObject* list models. Inherits common dirty/reset/role
/// handling from ObjectItemModelBase and adds flat-list index()/parent() overrides.
class ObjectListModelBase : public ObjectItemModelBase
{
    Q_OBJECT

public:
    ObjectListModelBase(QObject* parent = nullptr);
    ~ObjectListModelBase() override;

    virtual void clearAndDeleteContents() = 0; ///< Clears the list and calls deleteLater on each entry
    virtual QObject* removeOne(const QObject* object) = 0;
    virtual bool contains(const QObject* object) = 0;

    // Flat-list overrides of QAbstractItemModel — same behavior as QAbstractListModel
    QModelIndex index(int row, int column = 0, const QModelIndex& parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex& child) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    bool hasChildren(const QModelIndex& parent = QModelIndex()) const override;

protected:
    // Overrides from QAbstractItemModel which must be implemented by derived classes
    int rowCount(const QModelIndex& parent = QModelIndex()) const override = 0;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override = 0;
    bool insertRows(int position, int rows, const QModelIndex& index = QModelIndex()) override = 0;
    bool removeRows(int position, int rows, const QModelIndex& index = QModelIndex()) override = 0;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override = 0;

    bool _skipDirtyFirstItem = false;
};
