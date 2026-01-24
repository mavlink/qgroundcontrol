#pragma once

#include <optional>

#include <QtCore/QCommandLineParser>
#include <QtCore/QLoggingCategory>
#include <QtCore/QStringList>

Q_DECLARE_LOGGING_CATEGORY(QGCCommandLineParserLog)

namespace QGCCommandLineParser {

/// @brief Result of parsing command-line arguments
///
/// Contains all parsed options and status information.
/// All fields are always present for ABI stability, but test-related
/// command-line options are only available in QGC_UNITTEST_BUILD.
struct CommandLineParseResult
{
    enum class Status {
        Ok,
        Error,
        VersionRequested,
        HelpRequested
    } statusCode = Status::Ok;

    std::unique_ptr<QCommandLineParser> parser;

    std::optional<QString> errorString;
    QString helpText;
    QString versionText;

    QStringList positional;
    QStringList unknownOptions;

    // --- Core options ---
    std::optional<quint8> systemId;
    bool clearSettingsOptions = false;
    bool clearCache = false;
    std::optional<QString> loggingOptions;
    bool logOutput = false;
    bool simpleBootTest = false;

    // --- Test options (command-line parsing only in QGC_UNITTEST_BUILD) ---
    bool runningUnitTests = false;
    QStringList unitTests;
    bool stressUnitTests = false;
    uint stressUnitTestsCount = 0;
    std::optional<QString> unitTestOutput;  ///< Output file for test results (JUnit XML)

    // --- Desktop options (not on Android/iOS) ---
    bool fakeMobile = false;
    bool allowMultiple = false;

    // --- Graphics options ---
    bool useDesktopGL = false;      ///< Windows only: Force Desktop OpenGL
    bool useSwRast = false;         ///< Windows/macOS: Force software OpenGL
    bool quietWindowsAsserts = false;  ///< Windows only: Disable assert dialogs
};

/// @brief Parse the application's command-line arguments
/// @return Parsed result with status and option values
CommandLineParseResult parseCommandLine();

} // namespace QGCCommandLineParser
