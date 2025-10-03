/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "QmlObjectListModel.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QDebug>
#include <QtQml/QQmlEngine>

QGC_LOGGING_CATEGORY(QmlObjectListModelLog, "API.QmlObjectListModel")

QmlObjectListModel::QmlObjectListModel(QObject* parent)
    : QAbstractListModel        (parent)
    , _dirty                    (false)
    , _skipDirtyFirstItem       (false)
{
    QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);
}

QmlObjectListModel::~QmlObjectListModel()
{
    if (_resetModelNestingCount > 0) {
        qCWarning(QmlObjectListModelLog) << "QmlObjectListModel destroyed with unbalanced nesting of begin/endResetModel calls - _resetModelNestingCount:" << _resetModelNestingCount << this;
    }
}

QObject* QmlObjectListModel::get(int index)
{
    if (index < 0 || index >= _objectList.count()) {
        return nullptr;
    }
    return _objectList[index];
}

int QmlObjectListModel::rowCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    
    return _objectList.count();
}

QVariant QmlObjectListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }
    
    if (index.row() < 0 || index.row() >= _objectList.count()) {
        return QVariant();
    }
    
    if (role == ObjectRole) {
        return QVariant::fromValue(_objectList[index.row()]);
    } else if (role == TextRole) {
        return QVariant::fromValue(_objectList[index.row()]->objectName());
    } else {
        return QVariant();
    }
}

QHash<int, QByteArray> QmlObjectListModel::roleNames(void) const
{
    QHash<int, QByteArray> hash;
    
    hash[ObjectRole] = "object";
    hash[TextRole] = "text";
    
    return hash;
}

bool QmlObjectListModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (index.isValid() && role == ObjectRole) {
        _objectList.replace(index.row(), value.value<QObject*>());
        emit dataChanged(index, index);
        return true;
    }
    
    return false;
}

bool QmlObjectListModel::insertRows(int position, int rows, const QModelIndex& parent)
{
    Q_UNUSED(parent);
    
    if (position < 0 || position > _objectList.count() + 1) {
        qCWarning(QmlObjectListModelLog) << "Invalid position - position:count" << position << _objectList.count() << this;
    }
    
    beginInsertRows(QModelIndex(), position, position + rows - 1);
    endInsertRows();

    _signalCountChangedIfNotNested();
    
    return true;
}

bool QmlObjectListModel::removeRows(int position, int rows, const QModelIndex& parent)
{
    Q_UNUSED(parent);
    
    if (position < 0 || position >= _objectList.count()) {
        qCWarning(QmlObjectListModelLog) << "Invalid position - position:count" << position << _objectList.count() << this;
    } else if (position + rows > _objectList.count()) {
        qCWarning(QmlObjectListModelLog) << "Invalid rows - position:rows:count" << position << rows << _objectList.count() << this;
    }
    
    beginRemoveRows(QModelIndex(), position, position + rows - 1);
    for (int row=0; row<rows; row++) {
        _objectList.removeAt(position);
    }
    endRemoveRows();
    
    _signalCountChangedIfNotNested();
    
    return true;
}

void QmlObjectListModel::move(int from, int to)
{
    if(0 <= from && from < count() && 0 <= to && to < count() && from != to) {
        // Workaround to allow move item to the bottom. Done according to
        // beginMoveRows() documentation and implementation specificity:
        // https://doc.qt.io/qt-5/qabstractitemmodel.html#beginMoveRows
        // (see 3rd picture explanation there)
        if(from == to - 1) {
            to = from++;
        }
        beginMoveRows(QModelIndex(), from, from, QModelIndex(), to);
        _objectList.move(from, to);
        endMoveRows();
    }
}

QObject* QmlObjectListModel::operator[](int index)
{
    if (index < 0 || index >= _objectList.count()) {
        return nullptr;
    }
    return _objectList[index];
}

const QObject* QmlObjectListModel::operator[](int index) const
{
    if (index < 0 || index >= _objectList.count()) {
        return nullptr;
    }
    return _objectList[index];
}

void QmlObjectListModel::clear()
{
    beginResetModel();
    _objectList.clear();
    endResetModel();
}

QObject* QmlObjectListModel::removeAt(int i)
{
    QObject* removedObject = _objectList[i];
    if(removedObject) {
        // Look for a dirtyChanged signal on the object
        if (_objectList[i]->metaObject()->indexOfSignal(QMetaObject::normalizedSignature("dirtyChanged(bool)").constData()) != -1) {
            if (!_skipDirtyFirstItem || i != 0) {
                QObject::disconnect(_objectList[i], SIGNAL(dirtyChanged(bool)), this, SLOT(_childDirtyChanged(bool)));
            }
        }
    }
    removeRows(i, 1);
    setDirty(true);
    return removedObject;
}

void QmlObjectListModel::insert(int i, QObject* object)
{
    if (i < 0 || i > _objectList.count()) {
        qCWarning(QmlObjectListModelLog) << "Invalid index - index:count" << i << _objectList.count() << this;
    }
    if(object) {
        QQmlEngine::setObjectOwnership(object, QQmlEngine::CppOwnership);
        // Look for a dirtyChanged signal on the object
        if (object->metaObject()->indexOfSignal(QMetaObject::normalizedSignature("dirtyChanged(bool)").constData()) != -1) {
            if (!_skipDirtyFirstItem || i != 0) {
                QObject::connect(object, SIGNAL(dirtyChanged(bool)), this, SLOT(_childDirtyChanged(bool)));
            }
        }
    }
    _objectList.insert(i, object);
    insertRows(i, 1);
    setDirty(true);
}

void QmlObjectListModel::insert(int i, QList<QObject*> objects)
{
    if (i < 0 || i > _objectList.count()) {
        qCWarning(QmlObjectListModelLog) << "Invalid index - index:count" << i << _objectList.count() << this;
    }

    int j = i;
    for (QObject* object: objects) {
        QQmlEngine::setObjectOwnership(object, QQmlEngine::CppOwnership);

        // Look for a dirtyChanged signal on the object
        if (object->metaObject()->indexOfSignal(QMetaObject::normalizedSignature("dirtyChanged(bool)").constData()) != -1) {
            if (!_skipDirtyFirstItem || j != 0) {
                QObject::connect(object, SIGNAL(dirtyChanged(bool)), this, SLOT(_childDirtyChanged(bool)));
            }
        }

        _objectList.insert(j, object);
        j++;
    }

    insertRows(i, objects.count());

    setDirty(true);
}

void QmlObjectListModel::append(QObject* object)
{
    insert(_objectList.count(), object);
}

void QmlObjectListModel::append(QList<QObject*> objects)
{
    insert(_objectList.count(), objects);
}

QObjectList QmlObjectListModel::swapObjectList(const QObjectList& newlist)
{
    QObjectList oldlist(_objectList);
    beginResetModel();
    _objectList = newlist;
    endResetModel();
    return oldlist;
}

int QmlObjectListModel::count() const
{
    return rowCount();
}

void QmlObjectListModel::setDirty(bool dirty)
{
    if (_dirty != dirty) {
        _dirty = dirty;
        if (!dirty) {
            // Need to clear dirty from all children
            for(QObject* object: _objectList) {
                if (object->property("dirty").isValid()) {
                    object->setProperty("dirty", false);
                }
            }
        }
        emit dirtyChanged(_dirty);
    }
}

void QmlObjectListModel::_childDirtyChanged(bool dirty)
{
    _dirty |= dirty;
    // We want to emit dirtyChanged even if the actual value of _dirty didn't change. It can be a useful
    // signal to know when a child has changed dirty state
    emit dirtyChanged(_dirty);
}

void QmlObjectListModel::deleteListAndContents()
{
    for (int i=0; i<_objectList.count(); i++) {
        _objectList[i]->deleteLater();
    }
    deleteLater();
}

void QmlObjectListModel::clearAndDeleteContents()
{
    for (int i=0; i<_objectList.count(); i++) {
        _objectList[i]->deleteLater();
    }
    clear();
}

void QmlObjectListModel::beginResetModel()
{
    if (_resetModelNestingCount == 0) {
        qCDebug(QmlObjectListModelLog) << "First call to begindResetModel - calling QAbstractListModel::beginResetModel" << this;
        QAbstractListModel::beginResetModel();
    }
    _resetModelNestingCount++;
    qCDebug(QmlObjectListModelLog) << "_resetModelNestingCount:" << _resetModelNestingCount << this;
}

void QmlObjectListModel::endResetModel()
{
    if (_resetModelNestingCount == 0) {
        qCWarning(QmlObjectListModelLog) << "QmlObjectListModel::endResetModel called without prior beginResetModel";
        return;
    }
    _resetModelNestingCount--;
    qCDebug(QmlObjectListModelLog) << "_resetModelNestingCount:" << _resetModelNestingCount << this;
    if (_resetModelNestingCount == 0) {
        qCDebug(QmlObjectListModelLog) << "Last call to endResetModel - calling QAbstractListModel::endResetModel" << this;
        QAbstractListModel::endResetModel();
        emit countChanged(count());
    }
}

void QmlObjectListModel::_signalCountChangedIfNotNested()
{
    if (_resetModelNestingCount == 0) {
        emit countChanged(count());
    }
}
