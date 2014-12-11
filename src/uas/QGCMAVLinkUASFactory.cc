#include "QGCMAVLinkUASFactory.h"
#include "UASManager.h"
#include "QGXPX4UAS.h"

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

    UASInterface* uas;

    QThread* worker = new QThread();

    switch (heartbeat->autopilot)
    {
    case MAV_AUTOPILOT_GENERIC:
    {
        UAS* mav = new UAS(mavlink, worker, sysid);
        // Set the system type
        mav->setSystemType((int)heartbeat->type);

        // Connect this robot to the UAS object
        connect(mavlink, SIGNAL(messageReceived(LinkInterface*, mavlink_message_t)), mav, SLOT(receiveMessage(LinkInterface*, mavlink_message_t)));
        connect(mavlink, SIGNAL(messageReceived(LinkInterface*,mavlink_message_t)), mav->getFileManager(), SLOT(receiveMessage(LinkInterface*, mavlink_message_t)));
        uas = mav;
    }
    break;
    case MAV_AUTOPILOT_PX4:
    {
        QGXPX4UAS* px4 = new QGXPX4UAS(mavlink, worker, sysid);
        // Set the system type
        px4->setSystemType((int)heartbeat->type);

        // Connect this robot to the UAS object
        // it is IMPORTANT here to use the right object type,
        // else the slot of the parent object is called (and thus the special
        // packets never reach their goal)
        connect(mavlink, SIGNAL(messageReceived(LinkInterface*, mavlink_message_t)), px4, SLOT(receiveMessage(LinkInterface*, mavlink_message_t)));
        uas = px4;
    }
    break;
    default:
    {
        UAS* mav = new UAS(mavlink, worker, sysid);
        mav->setSystemType((int)heartbeat->type);

        // Connect this robot to the UAS object
        // it is IMPORTANT here to use the right object type,
        // else the slot of the parent object is called (and thus the special
        // packets never reach their goal)
        connect(mavlink, SIGNAL(messageReceived(LinkInterface*, mavlink_message_t)), mav, SLOT(receiveMessage(LinkInterface*, mavlink_message_t)));
        uas = mav;
    }
    break;
    }

    // Get the UAS ready
    worker->start(QThread::HighPriority);
    connect(uas, SIGNAL(destroyed()), worker, SLOT(quit()));

    // Set the autopilot type
    uas->setAutopilotType((int)heartbeat->autopilot);

    // Make UAS aware that this link can be used to communicate with the actual robot
    uas->addLink(link);

    // First thing we do with a new UAS is get the parameters
    uas->getParamManager()->requestParameterList();
    
    // Now add UAS to "official" list, which makes the whole application aware of it
    UASManager::instance()->addUAS(uas);
    
    return uas;
}
