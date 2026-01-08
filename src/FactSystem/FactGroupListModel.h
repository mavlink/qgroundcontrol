#pragma once

#include "QmlObjectListModel.h"
#include "QGCMAVLink.h"
#include "FactGroupWithId.h"

#include <QList>

/// Dynamically manages FactGroupWithIds based on incoming messages.
class FactGroupListModel : public QmlObjectListModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")

public:
    explicit FactGroupListModel(const char* factGroupNamePrefix, QObject* parent = nullptr);

    /// Allows for creation/updating of dynamic FactGroups based on incoming messages
    void handleMessageForFactGroupCreation(Vehicle *vehicle, const mavlink_message_t &message);

protected:
    virtual bool _shouldHandleMessage(const mavlink_message_t &message, QList<uint32_t> &ids) const = 0;
    virtual FactGroupWithId *_createFactGroupWithId(uint32_t id) = 0;

    FactGroupWithId *_findOrAddFactGroupById(Vehicle *vehicle, uint32_t id);
    QString _factGroupNameWithId(uint32_t id) const;

    const char* _factGroupNamePrefix;
};
