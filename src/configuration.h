#ifndef QGC_CONFIGURATION_H
#define QGC_CONFIGURATION_H

#include <QString>

/** @brief Polling interval in ms */
#define SERIAL_POLL_INTERVAL 9

/** @brief Heartbeat emission rate, in Hertz (times per second) */
#define MAVLINK_HEARTBEAT_DEFAULT_RATE 1

#define WITH_TEXT_TO_SPEECH 1

#define QGC_APPLICATION_NAME "QGroundControl"
#define QGC_APPLICATION_VERSION "v. 0.9.0 (Alpha RC1)"

namespace QGC

{
const QString APPNAME = "QGROUNDCONTROL";
const QString COMPANYNAME = "QGROUNDCONTROL";
const int APPLICATIONVERSION = 90; // 0.9.0
}

#endif // QGC_CONFIGURATION_H
