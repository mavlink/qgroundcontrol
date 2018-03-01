/****************************************************************************
 *
 *   (c) 2017 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "AirspaceManager.h"
#include "AirspaceFlightPlanProvider.h"
#include <QQmlEngine>

//-----------------------------------------------------------------------------
AirspaceFlightAuthorization::AirspaceFlightAuthorization(QObject *parent)
    : QObject(parent)
{
}

//-----------------------------------------------------------------------------
AirspaceFlightInfo::AirspaceFlightInfo(QObject *parent)
    : QObject(parent)
    , _beingDeleted(false)
    , _selected(false)
{
}

//-----------------------------------------------------------------------------
AirspaceFlightPlanProvider::AirspaceFlightPlanProvider(QObject *parent)
    : QObject(parent)
{
}

//-----------------------------------------------------------------------------
AirspaceFlightModel::AirspaceFlightModel(QObject *parent)
    : QAbstractListModel(parent)
{

}

//-----------------------------------------------------------------------------
AirspaceFlightInfo*
AirspaceFlightModel::get(int index)
{
    if (index < 0 || index >= _flightEntries.count()) {
        return NULL;
    }
    return _flightEntries[index];
}

//-----------------------------------------------------------------------------
int
AirspaceFlightModel::findFlightPlanID(QString flightPlanID)
{
    for(int i = 0; i < _flightEntries.count(); i++) {
        if(_flightEntries[i]->flightPlanID() == flightPlanID) {
            return i;
        }
    }
    return -1;
}

//-----------------------------------------------------------------------------
int
AirspaceFlightModel::count() const
{
    return _flightEntries.count();
}

//-----------------------------------------------------------------------------
void
AirspaceFlightModel::append(AirspaceFlightInfo* object)
{
    beginInsertRows(QModelIndex(), rowCount(), rowCount());
    QQmlEngine::setObjectOwnership(object, QQmlEngine::CppOwnership);
    _flightEntries.append(object);
    endInsertRows();
    emit countChanged();
}

//-----------------------------------------------------------------------------
void
AirspaceFlightModel::remove(const QString& flightPlanID)
{
    remove(findFlightPlanID(flightPlanID));
}

//-----------------------------------------------------------------------------
void
AirspaceFlightModel::remove(int index)
{
    if (index >= 0 && index < _flightEntries.count()) {
        beginRemoveRows(QModelIndex(), index, index);
        AirspaceFlightInfo* entry = _flightEntries[index];
        if(entry) {
            qCDebug(AirspaceManagementLog) << "Deleting flight plan" << entry->flightPlanID();
            entry->deleteLater();
        }
        _flightEntries.removeAt(index);
        endRemoveRows();
        emit countChanged();
    }
}

//-----------------------------------------------------------------------------
void
AirspaceFlightModel::clear(void)
{
    if(!_flightEntries.isEmpty()) {
        beginRemoveRows(QModelIndex(), 0, _flightEntries.count());
        while (_flightEntries.count()) {
            AirspaceFlightInfo* entry = _flightEntries.last();
            if(entry) entry->deleteLater();
            _flightEntries.removeLast();
        }
        endRemoveRows();
        emit countChanged();
    }
}

//-----------------------------------------------------------------------------
AirspaceFlightInfo*
AirspaceFlightModel::operator[](int index)
{
    return get(index);
}

//-----------------------------------------------------------------------------
int
AirspaceFlightModel::rowCount(const QModelIndex& /*parent*/) const
{
    return _flightEntries.count();
}

//-----------------------------------------------------------------------------
QVariant
AirspaceFlightModel::data(const QModelIndex & index, int role) const {
    if (index.row() < 0 || index.row() >= _flightEntries.count())
        return QVariant();
    if (role == ObjectRole)
        return QVariant::fromValue(_flightEntries[index.row()]);
    return QVariant();
}

//-----------------------------------------------------------------------------
QHash<int, QByteArray>
AirspaceFlightModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[ObjectRole] = "flightEntry";
    return roles;
}
