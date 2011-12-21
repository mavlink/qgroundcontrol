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

    UASInterface* uas;

    switch (heartbeat->autopilot)
    {
    case MAV_AUTOPILOT_GENERIC:
    {
        UAS* mav = new UAS(mavlink, sysid);
        // Set the system type
        mav->setSystemType((int)heartbeat->type);
        // Connect this robot to the UAS object
        connect(mavlink, SIGNAL(messageReceived(LinkInterface*, mavlink_message_t)), mav, SLOT(receiveMessage(LinkInterface*, mavlink_message_t)));
#ifdef QGC_PROTOBUF_ENABLED
        connect(mavlink, SIGNAL(extendedMessageReceived(LinkInterface*, std::tr1::shared_ptr<google::protobuf::Message>)), mav, SLOT(receiveExtendedMessage(LinkInterface*, std::tr1::shared_ptr<google::protobuf::Message>)));
#endif
        uas = mav;
    }
    break;
    case MAV_AUTOPILOT_PIXHAWK:
    {
        PxQuadMAV* mav = new PxQuadMAV(mavlink, sysid);
        // Set the system type
        mav->setSystemType((int)heartbeat->type);
        // Connect this robot to the UAS object
        // it is IMPORTANT here to use the right object type,
        // else the slot of the parent object is called (and thus the special
        // packets never reach their goal)
        connect(mavlink, SIGNAL(messageReceived(LinkInterface*, mavlink_message_t)), mav, SLOT(receiveMessage(LinkInterface*, mavlink_message_t)));
#ifdef QGC_PROTOBUF_ENABLED
        connect(mavlink, SIGNAL(extendedMessageReceived(LinkInterface*, std::tr1::shared_ptr<google::protobuf::Message>)), mav, SLOT(receiveExtendedMessage(LinkInterface*, std::tr1::shared_ptr<google::protobuf::Message>)));
#endif
        uas = mav;
    }
    break;
    case MAV_AUTOPILOT_SLUGS:
    {
        SlugsMAV* mav = new SlugsMAV(mavlink, sysid);
        // Set the system type
        mav->setSystemType((int)heartbeat->type);
        // Connect this robot to the UAS object
        // it is IMPORTANT here to use the right object type,
        // else the slot of the parent object is called (and thus the special
        // packets never reach their goal)
        connect(mavlink, SIGNAL(messageReceived(LinkInterface*, mavlink_message_t)), mav, SLOT(receiveMessage(LinkInterface*, mavlink_message_t)));
        uas = mav;
    }
    break;
    case MAV_AUTOPILOT_ARDUPILOTMEGA:
    {
        ArduPilotMegaMAV* mav = new ArduPilotMegaMAV(mavlink, sysid);
        // Set the system type
        mav->setSystemType((int)heartbeat->type);
        // Connect this robot to the UAS object
        // it is IMPORTANT here to use the right object type,
        // else the slot of the parent object is called (and thus the special
        // packets never reach their goal)
        connect(mavlink, SIGNAL(messageReceived(LinkInterface*, mavlink_message_t)), mav, SLOT(receiveMessage(LinkInterface*, mavlink_message_t)));
        uas = mav;
    }
    break;
#ifdef QGC_USE_SENSESOAR_MESSAGES
	case MAV_AUTOPILOT_SENSESOAR:
		{
			senseSoarMAV* mav = new senseSoarMAV(mavlink,sysid);
			mav->setSystemType((int)heartbeat->type);
			connect(mavlink, SIGNAL(messageReceived(LinkInterface*, mavlink_message_t)), mav, SLOT(receiveMessage(LinkInterface*, mavlink_message_t)));
			uas = mav;
			break;
		}
#endif
    default:
    {
        UAS* mav = new UAS(mavlink, sysid);
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

    // Set the autopilot type
    uas->setAutopilotType((int)heartbeat->autopilot);

    // Make UAS aware that this link can be used to communicate with the actual robot
    uas->addLink(link);

    // Now add UAS to "official" list, which makes the whole application aware of it
    UASManager::instance()->addUAS(uas);

    return uas;
}
