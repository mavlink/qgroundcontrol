/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "ObjectItemModelBase.h"
#include "QGCLoggingCategory.h"

#include <QtQml/QQmlEngine>

QGC_LOGGING_CATEGORY(ObjectItemModelBaseLog, "API.ObjectItemModelBase")

ObjectItemModelBase::ObjectItemModelBase(QObject* parent)
    : QAbstractItemModel(parent)
{
    QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);
}

ObjectItemModelBase::~ObjectItemModelBase()
{
    if (_resetModelNestingCount > 0) {
        qCWarning(ObjectItemModelBaseLog) << "Destroyed with unbalanced nesting of begin/endResetModel calls - _resetModelNestingCount:" << _resetModelNestingCount << this;
    }
}

QHash<int, QByteArray> ObjectItemModelBase::roleNames() const
{
    return {
        { ObjectRole, "object" },
        { TextRole, "text" },
    };
}

void ObjectItemModelBase::_childDirtyChanged(bool dirty)
{
    _dirty |= dirty;
    emit dirtyChanged(_dirty);
}

void ObjectItemModelBase::beginResetModel()
{
    if (_resetModelNestingCount == 0) {
        qCDebug(ObjectItemModelBaseLog) << "First beginResetModel - calling QAbstractItemModel::beginResetModel" << this;
        QAbstractItemModel::beginResetModel();
    }
    _resetModelNestingCount++;
    qCDebug(ObjectItemModelBaseLog) << "Reset model nesting count" << _resetModelNestingCount << this;
}

void ObjectItemModelBase::endResetModel()
{
    if (_resetModelNestingCount == 0) {
        qCWarning(ObjectItemModelBaseLog) << "endResetModel called without prior beginResetModel";
        return;
    }
    _resetModelNestingCount--;
    qCDebug(ObjectItemModelBaseLog) << "Reset model nesting count" << _resetModelNestingCount << this;
    if (_resetModelNestingCount == 0) {
        qCDebug(ObjectItemModelBaseLog) << "Last endResetModel - calling QAbstractItemModel::endResetModel" << this;
        QAbstractItemModel::endResetModel();
        emit countChanged(count());
    }
}

void ObjectItemModelBase::_signalCountChangedIfNotNested()
{
    if (_resetModelNestingCount == 0) {
        emit countChanged(count());
    }
}
