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
    for (; i < static_cast<int>(count()); i++) {
        auto *const currentGroup = value<FactGroupWithId*>(i);
        const int currentId = currentGroup->id()->rawValue().toInt();
        if (currentId > static_cast<int>(id)) {
            break;
        } else if (currentId == static_cast<int>(id)) {
            return currentGroup;
        }
    }

    auto *const newGroup = _createFactGroupWithId(id);
    insert(static_cast<int>(i), newGroup);
    vehicle->_addFactGroup(newGroup, _factGroupNameWithId(id));

    return newGroup;
}
