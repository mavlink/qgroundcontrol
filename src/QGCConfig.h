#ifndef QGC_CONFIGURATION_H
#define QGC_CONFIGURATION_H

#include <QString>

/** @brief Polling interval in ms */
#define SERIAL_POLL_INTERVAL 4

/** @brief Heartbeat emission rate, in Hertz (times per second) */
#define MAVLINK_HEARTBEAT_DEFAULT_RATE 1
#define WITH_TEXT_TO_SPEECH 1

#define QGC_APPLICATION_NAME "QGroundControl"
#define QGC_APPLICATION_VERSION_BASE "v2.0.3"

#ifdef QGC_APPLICATION_VERSION_SUFFIX
    #define QGC_APPLICATION_VERSION QGC_APPLICATION_VERSION_BASE QGC_APPLICATION_VERSION_SUFFIX
#else
    #define QGC_APPLICATION_VERSION QGC_APPLICATION_VERSION_BASE " (Developer Build)"
#endif

namespace QGC

{
const QString APPNAME = "QGROUNDCONTROL";
const QString ORG_NAME = "QGROUNDCONTROL.ORG"; //can be customized by forks to e.g. mycompany.com to maintain separate Settings for customized apps
const QString ORG_DOMAIN = "org.qgroundcontrol";//can be customized by forks
const int APPLICATIONVERSION = 203; // 2.0.3
}

#endif // QGC_CONFIGURATION_H
