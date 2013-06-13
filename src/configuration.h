#ifndef QGC_CONFIGURATION_H
#define QGC_CONFIGURATION_H

#include <QString>

/** @brief Polling interval in ms */
#define SERIAL_POLL_INTERVAL 9

/** @brief Heartbeat emission rate, in Hertz (times per second) */
#define MAVLINK_HEARTBEAT_DEFAULT_RATE 1
#define WITH_TEXT_TO_SPEECH 1

#define QGC_APPLICATION_NAME "APM Planner"
#define QGC_APPLICATION_VERSION "v2.0.0 (beta)"

namespace QGC

{
const QString APPNAME = "APMPLANNER";
const QString COMPANYNAME = "3DROBOTICS";
const int APPLICATIONVERSION = 200; // 1.0.9
}

#endif // QGC_CONFIGURATION_H
