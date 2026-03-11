#pragma once

#include <optional>

#include <QtCore/QtGlobal>

namespace QGCCommandLineParser {
    struct CommandLineParseResult;
}

namespace Platform {

/// @brief Initialize platform: run safety checks and configure environment
/// @param argc Argument count (for error dialogs)
/// @param argv Argument values (for error dialogs)
/// @param args Parsed command line arguments
/// @return Exit code if initialization failed, std::nullopt on success
/// @note Call before constructing QGCApplication
std::optional<int> initialize(int argc, char* argv[],
                               const QGCCommandLineParser::CommandLineParseResult& args);

/// @brief Complete platform setup after application exists
/// @note Call after Q(Core)Application exists and logging is installed
void setupPostApp();

#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
/// @brief Check if running as root (dangerous on Linux)
/// @return true if running as root
bool isRunningAsRoot();

/// @brief Show error dialog and return exit code when running as root
/// @return Exit code (-1)
int showRootError(int argc, char *argv[]);
#endif

#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
/// @brief Show error dialog when another instance is already running
/// @return Exit code (-1)
int showMultipleInstanceError(int argc, char *argv[]);

/// @brief Check if another instance is already running (single instance guard)
/// @param allowMultiple If true, skip the check and allow multiple instances
/// @return true if this instance can run, false if another instance is running
bool checkSingleInstance(bool allowMultiple);
#endif

} // namespace Platform
