/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QLoggingCategory>
#include <QtCore/QString>
#include <QtCore/QtSystemDetection>

Q_DECLARE_LOGGING_CATEGORY(PlatformLog)

namespace QGCCommandLineParser {
    struct CommandLineParseResult;
}

namespace Platform {

#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
bool isUserRoot();
#if !defined(QGC_NO_SERIAL_LINK)
void checkSerialPortPermissions(QString &errorMessage);
#endif
#endif

// Call before constructing Q(Core)Application.
void setupPreApp(const QGCCommandLineParser::CommandLineParseResult &cli);

// Call after Q(Core)Application exists and logging is installed.
void setupPostApp();

} // namespace Platform
