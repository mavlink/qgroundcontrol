#ifndef QGC_CONFIGURATION_H
#define QGC_CONFIGURATION_H

#include <QString>

/** @brief Polling interval in ms */
#define SERIAL_POLL_INTERVAL 4

/** @brief Heartbeat emission rate, in Hertz (times per second) */
#define MAVLINK_HEARTBEAT_DEFAULT_RATE 1
#define WITH_TEXT_TO_SPEECH 1

// If you need to make an incompatible changes to stored settings, bump this version number
// up by 1. This will caused store settings to be cleared on next boot.
#define QGC_SETTINGS_VERSION 6

#define QGC_APPLICATION_NAME "QGroundControl"
#define QGC_ORG_NAME "QGroundControl.org"
#define QGC_ORG_DOMAIN "org.qgroundcontrol"

#define QGC_APPLICATION_VERSION_MAJOR 2
#define QGC_APPLICATION_VERSION_MINOR 7

// The following #definess can be overriden from the command line so that automated build systems can
// add additional build identification.

// Only comes from command line
//#define QGC_APPLICATION_VERSION_COMMIT "..."

#ifndef QGC_APPLICATION_VERSION_BUILDNUMBER
#define QGC_APPLICATION_VERSION_BUILDNUMBER 1
#endif

#ifndef QGC_APPLICATION_VERSION_BUILDTYPE
#define QGC_APPLICATION_VERSION_BUILDTYPE "(Stable)"
#endif

#endif // QGC_CONFIGURATION_H
