/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#if defined (QGC_UTM_ADAPTER)
#include <QtCore/QDebug>

inline QDebug operator<<(QDebug debug, const std::string &s) {
    return debug << QString::fromStdString(s);
}

#define UTMSP_LOG_DEBUG    if(isDebugMode()) {} else QMessageLogger(__FILE__, __LINE__, Q_FUNC_INFO).debug
#define UTMSP_LOG_INFO     QMessageLogger(__FILE__, __LINE__, Q_FUNC_INFO).info
#define UTMSP_LOG_WARNING  QMessageLogger(__FILE__, __LINE__, Q_FUNC_INFO).warning
#define UTMSP_LOG_ERROR    QMessageLogger(__FILE__, __LINE__, Q_FUNC_INFO).critical
#define UTMSP_LOG_FATAL    QMessageLogger(__FILE__, __LINE__, Q_FUNC_INFO).fatal

#else
#include <iostream>
#include <string>
#include <sstream>
class UTMSPLogger {
public:
    UTMSPLogger(const std::string& level) : logLevel(level) {}

    ~UTMSPLogger() {
        if (logLevel != "Debug" || isDebugMode()) {
            std::cout << logLevel << ": " << ss.str() << std::endl;
        }
    }

    template <typename T>
    UTMSPLogger& operator<<(const T& msg) {
        ss << msg;
        return *this;
    }

private:
    std::string logLevel;
    std::stringstream ss;
};

#define UTMSP_LOG_DEBUG()   UTMSPLogger("Debug")
#define UTMSP_LOG_INFO()    UTMSPLogger("Info")
#define UTMSP_LOG_WARNING() UTMSPLogger("Warning")
#define UTMSP_LOG_ERROR()   UTMSPLogger("Error")
#define UTMSP_LOG_FATAL()   UTMSPLogger("Fatal"), std::abort()

#endif

inline bool isDebugMode() {
#if defined(QT_DEBUG)
    return false;
#else
    return true;
#endif
}
