/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "ObjectListModelBase.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(ObjectListModelBaseLog, "API.ObjectListModelBase")

ObjectListModelBase::ObjectListModelBase(QObject* parent)
    : ObjectItemModelBase(parent)
{
}

ObjectListModelBase::~ObjectListModelBase()
{
}

// Flat-list overrides — same semantics as QAbstractListModel

QModelIndex ObjectListModelBase::index(int row, int column, const QModelIndex& parent) const
{
    if (parent.isValid() || column != 0 || row < 0 || row >= rowCount()) {
        return {};
    }
    return createIndex(row, 0);
}

QModelIndex ObjectListModelBase::parent(const QModelIndex& child) const
{
    Q_UNUSED(child);
    return {};
}

int ObjectListModelBase::columnCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return 1;
}

bool ObjectListModelBase::hasChildren(const QModelIndex& parent) const
{
    // Only the root (invalid parent) has children in a flat list
    return !parent.isValid() && rowCount() > 0;
}
