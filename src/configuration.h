#ifndef QGC_CONFIGURATION_H
#define QGC_CONFIGURATION_H

#include <QString>

/** @brief Polling interval in ms */
#define SERIAL_POLL_INTERVAL 5

/** @brief Heartbeat emission rate, in Hertz (times per second) */
#define MAVLINK_HEARTBEAT_DEFAULT_RATE 1
#define WITH_TEXT_TO_SPEECH 1

#define QGC_APPLICATION_NAME "QGroundControl"
#define QGC_APPLICATION_VERSION "v. 2.0.1 (beta)"

namespace QGC

{
const QString APPNAME = "QGROUNDCONTROL";
const QString ORG_NAME = "QGROUNDCONTROL.ORG"; //can be customized by forks to e.g. mycompany.com to maintain separate Settings for customized apps
const QString ORG_DOMAIN = "org.qgroundcontrol";//can be customized by forks
const int APPLICATIONVERSION = 201; // 2.0.1
}

#endif // QGC_CONFIGURATION_H
