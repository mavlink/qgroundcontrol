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

#include <QtCore/QDebug>
#include <QtQml/QQmlEngine>

QGC_LOGGING_CATEGORY(ObjectListModelBaseLog, "API.ObjectListModelBase")

ObjectListModelBase::ObjectListModelBase(QObject* parent)
    : QAbstractListModel        (parent)
    , _dirty                    (false)
    , _skipDirtyFirstItem       (false)
{
    QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);
}

ObjectListModelBase::~ObjectListModelBase()
{
    if (_resetModelNestingCount > 0) {
        qCWarning(ObjectListModelBaseLog) << "Destroyed with unbalanced nesting of begin/endResetModel calls - _resetModelNestingCount:" << _resetModelNestingCount << this;
    }
}

QHash<int, QByteArray> ObjectListModelBase::roleNames(void) const
{
    QHash<int, QByteArray> hash;

    hash[ObjectRole] = "object";
    hash[TextRole] = "text";

    return hash;
}

void ObjectListModelBase::_childDirtyChanged(bool dirty)
{
    _dirty |= dirty;
    // We want to emit dirtyChanged even if the actual value of _dirty didn't change. It can be a useful
    // signal to know when a child has changed dirty state
    emit dirtyChanged(_dirty);
}

void ObjectListModelBase::beginResetModel()
{
    if (_resetModelNestingCount == 0) {
        qCDebug(ObjectListModelBaseLog) << "First call to begiResetModel - calling QAbstractListModel::beginResetModel" << this;
        QAbstractListModel::beginResetModel();
    }
    _resetModelNestingCount++;
    qCDebug(ObjectListModelBaseLog) << "Reset model nesting count" << _resetModelNestingCount << this;
}

void ObjectListModelBase::endResetModel()
{
    if (_resetModelNestingCount == 0) {
        qCWarning(ObjectListModelBaseLog) << "Internal Error: endResetModel called without prior beginResetModel";
        return;
    }
    _resetModelNestingCount--;
    qCDebug(ObjectListModelBaseLog) << "Reset model nesting count" << _resetModelNestingCount << this;
    if (_resetModelNestingCount == 0) {
        qCDebug(ObjectListModelBaseLog) << "Last call to endResetModel - calling QAbstractListModel::endResetModel" << this;
        QAbstractListModel::endResetModel();
        emit countChanged(count());
    }
}

void ObjectListModelBase::_signalCountChangedIfNotNested()
{
    if (_resetModelNestingCount == 0) {
        emit countChanged(count());
    }
}
