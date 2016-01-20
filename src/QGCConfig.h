#ifndef QGC_CONFIGURATION_H
#define QGC_CONFIGURATION_H

#include <QString>

/** @brief Polling interval in ms */
#define SERIAL_POLL_INTERVAL 4

#define WITH_TEXT_TO_SPEECH 1

// If you need to make an incompatible changes to stored settings, bump this version number
// up by 1. This will caused store settings to be cleared on next boot.
#define QGC_SETTINGS_VERSION 7

extern const char* QGC_APPLICATION_NAME; // "QGroundControl"
extern const char* QGC_ORG_NAME; //"QGroundControl.org"
extern const char* QGC_ORG_DOMAIN; //"org.qgroundcontrol"

#endif // QGC_CONFIGURATION_H
