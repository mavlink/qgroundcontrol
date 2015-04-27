#include "QGCMAVLinkUASFactory.h"
#include "UASManager.h"

QGCMAVLinkUASFactory::QGCMAVLinkUASFactory(QObject *parent) :
    QObject(parent)
{
}

UASInterface* QGCMAVLinkUASFactory::createUAS(MAVLinkProtocol* mavlink, LinkInterface* link, int sysid, mavlink_heartbeat_t* heartbeat, QObject* parent)
{
    QPointer<QObject> p;

    if (parent != NULL)
    {
        p = parent;
    }
    else
    {
        p = mavlink;
    }

    UASInterface* uasInterface;

    UAS* uasObject = new UAS(mavlink, sysid);
    Q_CHECK_PTR(uasObject);
    uasInterface = uasObject;
    
    uasObject->setSystemType((int)heartbeat->type);

    // Connect this robot to the UAS object
    // It is IMPORTANT here to use the right object type,
    // else the slot of the parent object is called (and thus the special
    // packets never reach their goal)
    connect(mavlink, &MAVLinkProtocol::messageReceived, uasObject, &UAS::receiveMessage);

    // Set the autopilot type
    uasInterface->setAutopilotType((int)heartbeat->autopilot);

    // Make UAS aware that this link can be used to communicate with the actual robot
    uasInterface->addLink(link);

    // Now add UAS to "official" list, which makes the whole application aware of it
    UASManager::instance()->addUAS(uasInterface);
    
    return uasInterface;
}
