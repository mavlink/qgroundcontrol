/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#include "MCUManager.h"
#include "QGCLoggingCategory.h"
#include "QGCApplication.h"
#include "QGCMAVLink.h"

QGC_LOGGING_CATEGORY(MCUManagerLog, "MCUManagerLog")

MCUManager::MCUManager(QGCApplication* app, QGCToolbox* toolbox)
    : QGCTool(app, toolbox)
    , _mavlinkProtocol(nullptr)
    , _percentRemaining(0)
{

}

void MCUManager::setToolbox(QGCToolbox *toolbox)
{
    QGCTool::setToolbox(toolbox);

    _mavlinkProtocol = _toolbox->mavlinkProtocol();

    QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);
    qmlRegisterUncreatableType<MCUManager>("QGroundControl.MCUManager", 1, 0, "MCUManager", "Reference only");

    connect(_mavlinkProtocol, &MAVLinkProtocol::messageReceived, this, &MCUManager::_mavlinkMessageReceived);

}

void MCUManager::_mavlinkMessageReceived(LinkInterface* link, mavlink_message_t message)
{
    Q_UNUSED(link);

    uint32_t msgid = message.msgid;
    const mavlink_message_info_t* msgInfo = mavlink_get_message_info(&message);
    if(!msgInfo) {
        qWarning() << "Invalid MAVLink message received. ID:" << msgid;
        return;
    }
    qCDebug(MCUManagerLog) << "MCUManager::_mavlinkMessageReceived  " << msgInfo->name;

    switch (message.msgid)
    {
        case MAVLINK_MSG_ID_MCU_DATA:
            _handleMcuData(message);
            break;

        case MAVLINK_MSG_ID_MCU_UPGRADE:
            break;
    }
}

void MCUManager::_handleMcuData(mavlink_message_t& message)
{
    mavlink_mcu_data_t mcuData;

    mavlink_msg_mcu_data_decode(&message, &mcuData);

    switch (mcuData.type)
    {
        case MCU_TYPE_GS_SMART_BATTERY_INFO:
            int newPercentRemaining = mcuData.data[1];
            if (_percentRemaining != newPercentRemaining)
            {
                _percentRemaining = newPercentRemaining;
                emit percentRemainingChanged(_percentRemaining);
            }
            break;
    }
}