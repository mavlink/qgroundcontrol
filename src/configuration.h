#ifndef QGC_CONFIGURATION_H
#define QGC_CONFIGURATION_H

#include <QString>

/** @brief Polling interval in ms */
#define SERIAL_POLL_INTERVAL 9

/** @brief Heartbeat emission rate, in Hertz (times per second) */
#define MAVLINK_HEARTBEAT_DEFAULT_RATE 1

#define WITH_TEXT_TO_SPEECH 1

#define QGC_APPLICATION_NAME "QGroundControl"
#define QGC_APPLICATION_VERSION "v. 1.0.1 (Alpha RC18)"

namespace QGC

{
const QString APPNAME = "QGROUNDCONTROL";
const QString COMPANYNAME = "QGROUNDCONTROL";
const int APPLICATIONVERSION = 101; // 1.0.1
}

#endif // QGC_CONFIGURATION_H
