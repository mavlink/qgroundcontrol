#include "QmlObjectListModel.h"
#include <QtCore/QLoggingCategory>

Q_STATIC_LOGGING_CATEGORY(QmlObjectListModelLog, "API.QmlObjectListModel")

QmlObjectListModel::QmlObjectListModel(QObject* parent)
    : ObjectListModelBase(parent)
{

}

QObject* QmlObjectListModel::get(int index)
{
    if (index < 0 || index >= _objectList.count()) {
        qCWarning(QmlObjectListModelLog) << "InternalError: Invalid index - index:count" << index << _objectList.count() << this;
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

void QmlObjectListModel::clearAndDeleteContents()
{
    for (int i=0; i<_objectList.count(); i++) {
        _objectList[i]->deleteLater();
    }
    clear();
}
