#pragma once

#include <optional>

#include <QtCore/QCommandLineParser>
#include <QtCore/QStringList>


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
    std::optional<QString> labelFilter;     ///< Filter tests by label (comma-separated)
    bool listTests = false;                 ///< List available tests and exit

    // --- Desktop options (not on Android/iOS) ---
    bool fakeMobile = false;
    bool allowMultiple = false;

    // --- Graphics options ---
    bool useDesktopGL = false;      ///< Windows only: Force Desktop OpenGL
    bool useSwRast = false;         ///< Windows/macOS: Force software OpenGL
    bool quietWindowsAsserts = false;  ///< Windows only: Disable assert dialogs
};

/// @brief Parse command-line arguments (requires existing QCoreApplication)
/// @return Parsed result with status and option values
/// @note Prefer using parse() which manages the QCoreApplication lifecycle
CommandLineParseResult parseCommandLine();

/// @brief Parse command-line arguments with automatic QCoreApplication management
/// @param argc Argument count from main()
/// @param argv Argument values from main()
/// @return Parsed result with status and option values
/// @note Creates a temporary QCoreApplication internally for parsing
CommandLineParseResult parse(int argc, char* argv[]);

/// @brief Handle early exit conditions (help, version, error)
/// @param result The parse result to check
/// @return Exit code if main() should return immediately, std::nullopt to continue
std::optional<int> handleParseResult(const CommandLineParseResult& result);

/// @brief Application execution mode
enum class AppMode {
    Gui,        ///< Normal GUI application
    Test,       ///< Run unit tests
    BootTest,   ///< Initialize and exit (for CI validation)
    ListTests   ///< List available tests and exit
};

/// @brief Determine the application mode from parsed arguments
/// @param args Parsed command line arguments
/// @return The mode the application should run in
AppMode determineAppMode(const CommandLineParseResult& args);

#ifdef QGC_UNITTEST_BUILD
/// @brief Developer helper: Override argc/argv for debugging specific tests.
///
/// To use: set enabled=true in the implementation, modify the arguments,
/// and rebuild. This allows quick iteration on specific tests without
/// typing the full command line.
void overrideCommandLine(int& argc, char**& argv);
#endif

} // namespace QGCCommandLineParser
