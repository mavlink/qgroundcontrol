// #include "QGCApplication.h"
// #include "TestMavlinkPlugin.h"
// #include "QGCApplication.h"
// #include "Vehicle.h"
// #include "MAVLinkProtocol.h"
// #include "MultiVehicleManager.h"
// #include "LinkInterface.h"
// #include <QDebug>
// //
//
// TestMavlinkPlugin::TestMavlinkPlugin(QObject* parent)
//     : QObject(parent)
// {
//     // 监听 MAVLink 消息
//     connect(qgcApp()->toolbox()->mavlinkProtocol(),
//             &MAVLinkProtocol::messageReceived,
//             this,
//             &TestMavlinkPlugin::_mavlinkMessageReceived);
//
//     // 启动后主动发送一次心跳
//     sendHeartbeat();
// }
//
// void TestMavlinkPlugin::sendHeartbeat()
// {
//     mavlink_heartbeat_t hb = {};
//     hb.type = MAV_TYPE_GCS;
//     hb.autopilot = MAV_AUTOPILOT_INVALID;
//     hb.base_mode = MAV_MODE_MANUAL_ARMED;
//     hb.system_status = MAV_STATE_ACTIVE;
//     hb.mavlink_version = 3;
//
//     mavlink_message_t msg;
//     mavlink_msg_heartbeat_encode(255, 0, &msg, &hb); // System ID = 255, Component ID = 0
//
//     _sendMessage(msg);
// }
//
// void TestMavlinkPlugin::_sendMessage(const mavlink_message_t& msg)
// {
//     Vehicle* vehicle = qgcApp()->toolbox()->multiVehicleManager()->activeVehicle();
//     if (vehicle && vehicle->priorityLink()) {
//         vehicle->sendMessageOnLink(vehicle->priorityLink(), msg);
//         qDebug() << "已发送 MAVLink 心跳消息";
//     } else {
//         qWarning() << "未找到活跃的 Vehicle，无法发送消息";
//     }
// }
//
// void TestMavlinkPlugin::_mavlinkMessageReceived(LinkInterface* link, mavlink_message_t message)
// {
//     if (message.msgid == MAVLINK_MSG_ID_HEARTBEAT) {
//         mavlink_heartbeat_t hb;
//         mavlink_msg_heartbeat_decode(&message, &hb);
//         qDebug() << "[收到飞控心跳]"
//                  << "类型:" << hb.type
//                  << "状态:" << hb.system_status
//                  << "基模式:" << hb.base_mode;
//     }
// }
