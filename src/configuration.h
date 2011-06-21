#ifndef QGC_CONFIGURATION_H
#define QGC_CONFIGURATION_H

#include <QString>

/** @brief Polling interval in ms */
#define SERIAL_POLL_INTERVAL 7

/** @brief Heartbeat emission rate, in Hertz (times per second) */
#define MAVLINK_HEARTBEAT_DEFAULT_RATE 1

#define WITH_TEXT_TO_SPEECH 1

#define QGC_APPLICATION_NAME "QGroundControl"
#define QGC_APPLICATION_VERSION "v. 0.8.3 (Alpha RC7)"

namespace QGC

{
const QString APPNAME = "QGROUNDCONTROL";
const QString COMPANYNAME = "OPENMAV";
const int APPLICATIONVERSION = 83; // 0.8.0
}

#endif // QGC_CONFIGURATION_H
