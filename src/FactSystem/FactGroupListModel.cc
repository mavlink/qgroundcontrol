/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "FactGroupListModel.h"
#include "Vehicle.h"

FactGroupListModel::FactGroupListModel(const char* factGroupNamePrefix, QObject* parent)
    : QmlObjectListModel(parent)
    , _factGroupNamePrefix(factGroupNamePrefix)
{

}

void FactGroupListModel::handleMessageForFactGroupCreation(Vehicle *vehicle, const mavlink_message_t &message)
{
    QList<uint32_t> ids;

    if (_shouldHandleMessage(message, ids)) {
        for (const uint32_t id : ids) {
            _findOrAddFactGroupById(vehicle, id);
        }
    }
}

QString FactGroupListModel::_factGroupNameWithId(uint32_t id) const
{
    return QStringLiteral("%1%2").arg(_factGroupNamePrefix).arg(id);
}

FactGroupWithId *FactGroupListModel::_findOrAddFactGroupById(Vehicle *vehicle, uint32_t id)
{
    int i = 0;

    // We maintain the list in order sorted by id so the ui shows them sorted.
    for (; i < count(); i++) {
        auto *const currentGroup = value<FactGroupWithId*>(i);
        const int currentId = currentGroup->id()->rawValue().toInt();
        if (currentId > id) {
            break;
        } else if (currentId == id) {
            return currentGroup;
        }
    }

    auto *const newGroup = _createFactGroupWithId(id);
    insert(i, newGroup);
    vehicle->_addFactGroup(newGroup, _factGroupNameWithId(id));

    return newGroup;
}
