/*=====================================================================
 
 QGroundControl Open Source Ground Control Station
 
 (c) 2009, 2015 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 
 This file is part of the QGROUNDCONTROL project
 
 QGROUNDCONTROL is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 QGROUNDCONTROL is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.
 
 ======================================================================*/

/// @file
///     @author Don Gagne <don@thegagnes.com>

#include "QmlObjectListModel.h"

#include <QDebug>
#include <QQmlEngine>

const int QmlObjectListModel::ObjectRole = Qt::UserRole;
const int QmlObjectListModel::TextRole = Qt::UserRole + 1;

QmlObjectListModel::QmlObjectListModel(QObject* parent)
    : QAbstractListModel(parent)
    , _dirty(false)
    , _skipDirtyFirstItem(false)
{

}

QmlObjectListModel::~QmlObjectListModel()
{
    
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
    
    if (index.row() >= _objectList.count()) {
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
        qWarning() << "Invalid position position:count" << position << _objectList.count();
    }
    
    beginInsertRows(QModelIndex(), position, position + rows - 1);
    endInsertRows();
    
    emit countChanged(count());
    
    return true;
}

bool QmlObjectListModel::removeRows(int position, int rows, const QModelIndex& parent)
{
    Q_UNUSED(parent);
    
    if (position < 0 || position >= _objectList.count()) {
        qWarning() << "Invalid position position:count" << position << _objectList.count();
    } else if (position + rows > _objectList.count()) {
        qWarning() << "Invalid rows position:rows:count" << position << rows << _objectList.count();
    }
    
    beginRemoveRows(QModelIndex(), position, position + rows - 1);
    for (int row=0; row<rows; row++) {
        // FIXME: Need to figure our correct memory management for here
        //_objectList[position]->deleteLater();
        _objectList.removeAt(position);
    }
    endRemoveRows();
    
    emit countChanged(count());
    
    return true;
}

QObject* QmlObjectListModel::operator[](int index)
{
    return _objectList[index];
}

const QObject* QmlObjectListModel::operator[](int index) const
{
    return _objectList[index];
}

void QmlObjectListModel::clear(void)
{
    while (rowCount()) {
        removeAt(0);
    }
}

QObject* QmlObjectListModel::removeAt(int i)
{
    QObject* removedObject = _objectList[i];

    // Look for a dirtyChanged signal on the object
    if (_objectList[i]->metaObject()->indexOfSignal(QMetaObject::normalizedSignature("dirtyChanged(bool)")) != -1) {
        if (!_skipDirtyFirstItem || i != 0) {
            QObject::disconnect(_objectList[i], SIGNAL(dirtyChanged(bool)), this, SLOT(_childDirtyChanged(bool)));
        }
    }
    
    removeRows(i, 1);
    
    setDirty(true);
    
    return removedObject;
}

void QmlObjectListModel::insert(int i, QObject* object)
{
    if (i < 0 || i > _objectList.count()) {
        qWarning() << "Invalid index index:count" << i << _objectList.count();
    }
    
    QQmlEngine::setObjectOwnership(object, QQmlEngine::CppOwnership);
    
    // Look for a dirtyChanged signal on the object
    if (object->metaObject()->indexOfSignal(QMetaObject::normalizedSignature("dirtyChanged(bool)")) != -1) {
        if (!_skipDirtyFirstItem || i != 0) {
            QObject::connect(object, SIGNAL(dirtyChanged(bool)), this, SLOT(_childDirtyChanged(bool)));
        }
    }

    _objectList.insert(i, object);
    insertRows(i, 1);
    
    setDirty(true);
}

void QmlObjectListModel::append(QObject* object)
{
    insert(_objectList.count(), object);
}

int QmlObjectListModel::count(void) const
{
    return rowCount();
}

void QmlObjectListModel::setDirty(bool dirty)
{
    _dirty = dirty;

    if (!dirty) {
        // Need to clear dirty from all children
        foreach(QObject* object, _objectList) {
            if (object->property("dirty").isValid()) {
                object->setProperty("dirty", false);
            }
        }
    }
    
    emit dirtyChanged(_dirty);
}

void QmlObjectListModel::_childDirtyChanged(bool dirty)
{
    _dirty |= dirty;
    // We want to emit dirtyChanged even if the actual value of _dirty didn't change. It can be a useful
    // signal to know when a child has changed dirty state
    emit dirtyChanged(_dirty);
}

void QmlObjectListModel::deleteListAndContents(void)
{
    for (int i=0; i<_objectList.count(); i++) {
        _objectList[i]->deleteLater();
    }
    deleteLater();
}

void QmlObjectListModel::clearAndDeleteContents(void)
{
    for (int i=0; i<_objectList.count(); i++) {
        _objectList[i]->deleteLater();
    }
    clear();
}
