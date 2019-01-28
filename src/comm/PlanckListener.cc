#include <QGeoCoordinate>
#include <QGeoPositionInfo>

#include "PlanckListener.h"
#include "LandingPadPosition.h"
#include "LinkInterface.h"
#include "MAVLinkProtocol.h"
#include "QGCApplication.h"

PlanckListener::PlanckListener(QGCApplication* app, QGCToolbox* toolbox)
: QGCTool(app, toolbox)
{
}

void PlanckListener::setToolbox(QGCToolbox *toolbox)
{
    QGCTool::setToolbox(toolbox);
    connect(toolbox->mavlinkProtocol(), &MAVLinkProtocol::messageReceived, this, &PlanckListener::onMAVLinkMessage);
}

void PlanckListener::onMAVLinkMessage(LinkInterface* link, mavlink_message_t message)
{
    Q_UNUSED(link);
    if (message.msgid == MAVLINK_MSG_ID_PLANCK_LANDING_PLATFORM_STATE) {
        mavlink_planck_landing_platform_state_t lps;
        mavlink_msg_planck_landing_platform_state_decode(&message, &lps);

        LandingPadPosition* pos = qgcApp()->toolbox()->landingPadManager();

        if(pos)
        {
            pos->setPosition(lps.latitude, lps.longitude);
        }
    }
}
