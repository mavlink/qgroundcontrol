#pragma once

#include <QtCore/QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(PlatformLog)

namespace QGCCommandLineParser {
    struct CommandLineParseResult;
}

namespace Platform {

// Call before constructing Q(Core)Application.
void setupPreApp(const QGCCommandLineParser::CommandLineParseResult &cli);

// Call after Q(Core)Application exists and logging is installed.
void setupPostApp();

} // namespace Platform
