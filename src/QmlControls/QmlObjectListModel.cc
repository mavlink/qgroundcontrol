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

const int QmlObjectListModel::ObjectRole = Qt::UserRole;

QmlObjectListModel::QmlObjectListModel(QObject* parent)
    : QAbstractListModel(parent)
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
    } else {
        return QVariant();
    }
}

QHash<int, QByteArray> QmlObjectListModel::roleNames(void) const
{
    QHash<int, QByteArray> hash;
    
    hash[ObjectRole] = "object";
    
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
    
    beginInsertRows(QModelIndex(), position, position + rows - 1);
    endInsertRows();
    
    emit countChanged(count());
    
    return true;
}

bool QmlObjectListModel::removeRows(int position, int rows, const QModelIndex& parent)
{
    Q_UNUSED(parent);
    
    beginRemoveRows(QModelIndex(), position, position + rows - 1);
    for (int row=0; row<rows; row++) {
        _objectList.removeAt(position);
    }
    endRemoveRows();
    
    emit countChanged(count());
    
    return true;
}

QObject*& QmlObjectListModel::operator[](int index)
{
    return _objectList[index];
}

void QmlObjectListModel::clear(void)
{
    while (rowCount()) {
        removeRows(0, 1);
    }
}

void QmlObjectListModel::removeAt(int i)
{
    removeRows(i, 1);
}

void QmlObjectListModel::append(QObject* object)
{
    _objectList += object;
    insertRows(_objectList.count() - 1, 1);
}

int QmlObjectListModel::count(void)
{
    return rowCount();
}

QObject* QmlObjectListModel::get(int index)
{
    return _objectList[index];
}
