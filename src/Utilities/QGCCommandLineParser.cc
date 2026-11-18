#include "QGCCommandLineParser.h"

#include <cstdlib>
#include <limits>

#include <QtCore/QCommandLineOption>
#include <QtCore/QCoreApplication>

#include "QGCLoggingCategory.h"
#include "qgc_version.h"

QGC_LOGGING_CATEGORY(QGCCommandLineParserLog, "Utilities.QGCCommandLineParser")

namespace QGCCommandLineParser {

// ============================================================================
// Option Name Constants
// ============================================================================

namespace {

// --- Options that QCommandLineParser::addHelpOption() and addVersionOption() add ---
constexpr QLatin1StringView kOptHelp = QLatin1StringView("help");
constexpr QLatin1StringView kOptHelpAll = QLatin1StringView("help-all");
constexpr QLatin1StringView kOptVersion = QLatin1StringView("version");

// --- Core options ---
constexpr QLatin1StringView kOptSystemId      = QLatin1StringView("system-id");
constexpr QLatin1StringView kOptClearSettings = QLatin1StringView("clear-settings");
constexpr QLatin1StringView kOptClearCache    = QLatin1StringView("clear-cache");
constexpr QLatin1StringView kOptLogging       = QLatin1StringView("logging");
constexpr QLatin1StringView kOptLogOutput     = QLatin1StringView("log-output");
constexpr QLatin1StringView kOptSimpleBoot    = QLatin1StringView("simple-boot-test");

#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
// --- Desktop-only options ---
constexpr QLatin1StringView kOptFakeMobile    = QLatin1StringView("fake-mobile");
constexpr QLatin1StringView kOptAllowMultiple = QLatin1StringView("allow-multiple");
#endif

#ifdef QGC_UNITTEST_BUILD
// --- Test options ---
constexpr QLatin1StringView kOptUnittest       = QLatin1StringView("unittest");
constexpr QLatin1StringView kOptUnittestStress = QLatin1StringView("unittest-stress");
constexpr QLatin1StringView kOptUnittestOutput = QLatin1StringView("unittest-output");
constexpr QLatin1StringView kOptUnittestShard = QLatin1StringView("unittest-shard");
constexpr QLatin1StringView kOptUnittestLabel  = QLatin1StringView("label");
constexpr QLatin1StringView kOptListTests      = QLatin1StringView("list-tests");
constexpr QLatin1StringView kOptOnscreen       = QLatin1StringView("onscreen");
#endif

#ifdef Q_OS_WIN
// --- Windows-only options ---
constexpr QLatin1StringView kOptNoWinAssertUI = QLatin1StringView("no-windows-assert-ui");
#endif

#if defined(Q_OS_WIN) || defined(Q_OS_MACOS)
// --- Windows/macOS options ---
constexpr QLatin1StringView kOptSwrast = QLatin1StringView("swrast");
#endif

} // anonymous namespace

// ============================================================================
// Argument Normalization
// ============================================================================

/// @brief Normalizes command-line arguments
/// Converts colon-separated syntax (--option:value) to standard format (--option value)
/// and handles special cases like --unittest without a value.
/// Arguments already in the --option=value form are passed on unchanged.
QStringList normalizeArgs(const QStringList &args)
{
    QStringList out;
    out.reserve(args.size() + 4);

    for (qsizetype i = 0; i < args.size(); ++i) {
        const QString &arg = args.at(i);
#ifdef QGC_UNITTEST_BUILD
        if (arg == QLatin1String("--unittest")) {
            const bool valueFollows = ((i + 1) < args.size()) && !args.at(i + 1).startsWith(QLatin1Char('-'));
            out << arg;
            if (!valueFollows) {
                out << QString();
            }
            continue;
        }
#endif

        // Convert --option:value to --option value. The first colon ends the option name; a colon
        // after '=' is part of the value of an --option=value argument (a Windows path, a URL).
        const qsizetype idx = arg.indexOf(QLatin1Char(':'));
        const qsizetype equalsIdx = arg.indexOf(QLatin1Char('='));
        if (arg.startsWith(QLatin1String("--")) && (idx >= 0) && ((equalsIdx < 0) || (idx < equalsIdx))) {
            const QString opt = arg.left(idx);
            const QString val = arg.mid(idx + 1);
            out << opt;
            if (!val.isEmpty()) {
                out << val;
            }
#ifdef QGC_UNITTEST_BUILD
            else if (opt == QLatin1String("--unittest")) {
                out << QString();
            }
#endif
        } else {
            out << arg;
        }
    }

    qCDebug(QGCCommandLineParserLog) << "Normalized arguments:" << out;
    return out;
}

// ============================================================================
// Command Line Parsing
// ============================================================================

namespace {

/// The error for options the parser does not know. It says so when the option exists in another build or on
/// another platform.
QString unknownOptionsError(const QStringList& unknownOptions)
{
#ifndef QGC_UNITTEST_BUILD
    if (unknownOptions.contains(QLatin1String("unittest")) ||
        unknownOptions.contains(QLatin1String("unittest-stress")) ||
        unknownOptions.contains(QLatin1String("unittest-output")) ||
        unknownOptions.contains(QLatin1String("unittest-shard")) ||
        unknownOptions.contains(QLatin1String("list-tests"))) {
        return QCoreApplication::translate("main",
                                           "--unittest/--unittest-stress/--unittest-output/--unittest-shard/"
                                           "--list-tests options are only available in unittest builds.");
    }
#endif

#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
    if (unknownOptions.contains(QLatin1String("fake-mobile")) ||
        unknownOptions.contains(QLatin1String("allow-multiple"))) {
        return QCoreApplication::translate("main",
                                           "--fake-mobile/--allow-multiple are not supported on mobile platforms.");
    }
#endif

#ifndef Q_OS_WIN
    if (unknownOptions.contains(QLatin1String("no-windows-assert-ui"))) {
        return QCoreApplication::translate("main", "--no-windows-assert-ui is only supported on Windows.");
    }
#endif

#if !defined(Q_OS_WIN) && !defined(Q_OS_MACOS)
    if (unknownOptions.contains(QLatin1String("swrast"))) {
        return QCoreApplication::translate("main", "--swrast is only supported on Windows and macOS.");
    }
#endif

    return QCoreApplication::translate("main", "Unknown options: %1").arg(unknownOptions.join(QLatin1String(", ")));
}

/// Reads the positional arguments and the options that @a parser parsed into @a out.
/// @return The error text of a command line that is rejected
std::optional<QString> readParsedOptions(const QCommandLineParser& parser, CommandLineParseResult& out)
{
    // --- Validate positional arguments ---
    out.positional = parser.positionalArguments();
    if (!out.positional.isEmpty()) {
        return QCoreApplication::translate("main", "Unexpected positional arguments: %1")
            .arg(out.positional.join(QLatin1String(", ")));
    }

    // --- Parse core options ---
    if (parser.isSet(kOptSystemId)) {
        const QString systemIdStr = parser.value(kOptSystemId);
        const std::optional<quint32> systemId = parseSystemId(systemIdStr);
        if (!systemId) {
            return QCoreApplication::translate("main", "Invalid System ID (must be 1-4294967295): %1").arg(systemIdStr);
        }
        out.systemId = systemId;
        qCDebug(QGCCommandLineParserLog) << "System ID:" << *systemId;
    }

    out.clearSettingsOptions = parser.isSet(kOptClearSettings);
    out.clearCache = parser.isSet(kOptClearCache);
    if (parser.isSet(kOptLogging)) {
        out.loggingOptions = parser.value(kOptLogging);
        qCDebug(QGCCommandLineParserLog) << "Logging options:" << out.loggingOptions.value();
    }
    out.logOutput = parser.isSet(kOptLogOutput);
    out.simpleBootTest = parser.isSet(kOptSimpleBoot);

#ifdef QGC_UNITTEST_BUILD
    // --- Parse test options ---
    if (parser.isSet(kOptUnittest)) {
        out.runningUnitTests = true;
        const QStringList vals = parser.values(kOptUnittest);
        // Filter out empty values (from bare --unittest)
        for (const QString& val : vals) {
            if (!val.isEmpty()) {
                out.unitTests.append(val);
            }
        }
        qCDebug(QGCCommandLineParserLog) << "Unit tests:" << (out.unitTests.isEmpty() ? QStringLiteral("all") : out.unitTests.join(QLatin1String(", ")));
    }

    if (parser.isSet(kOptUnittestStress)) {
        out.runningUnitTests = true;
        out.stressUnitTests = true;
        const QString stress = parser.value(kOptUnittestStress);
        if (stress.isEmpty()) {
            out.stressUnitTestsCount = 20;
        } else {
            bool ok = false;
            const uint count = stress.toUInt(&ok);
            if (!ok || (count == 0)) {
                return QCoreApplication::translate("main", "Invalid stress test count (must be > 0): %1").arg(stress);
            }
            out.stressUnitTestsCount = count;
        }
        qCDebug(QGCCommandLineParserLog) << "Stress test iterations:" << out.stressUnitTestsCount;
    }

    if (parser.isSet(kOptUnittestOutput)) {
        out.unitTestOutput = parser.value(kOptUnittestOutput);
        qCDebug(QGCCommandLineParserLog) << "Test output file:" << out.unitTestOutput.value();
    }

    if (parser.isSet(kOptUnittestShard)) {
        const QString shardValue = parser.value(kOptUnittestShard);
        const std::optional<std::pair<uint, uint>> shard = parseUnitTestShard(shardValue);
        if (!shard) {
            return QCoreApplication::translate(
                       "main", "Invalid unit test shard (expected <index>/<count> with 1 <= index <= count): %1")
                .arg(shardValue);
        }
        if (out.unitTests.size() != 1) {
            return QCoreApplication::translate("main", "--unittest-shard requires exactly one --unittest:<name>.");
        }
        out.unitTestShardIndex = shard->first;
        out.unitTestShardCount = shard->second;
        qCDebug(QGCCommandLineParserLog) << "Test shard:" << shard->first << "of" << shard->second;
    }

    if (parser.isSet(kOptUnittestLabel)) {
        out.labelFilter = parser.value(kOptUnittestLabel);
        qCDebug(QGCCommandLineParserLog) << "Label filter:" << out.labelFilter.value();
    }

    out.listTests = parser.isSet(kOptListTests);
    if (out.listTests) {
        qCDebug(QGCCommandLineParserLog) << "List tests requested";
    }

    out.onscreen = parser.isSet(kOptOnscreen);
#endif

    // --- Parse desktop options ---
#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
    out.fakeMobile = parser.isSet(kOptFakeMobile);
    out.allowMultiple = parser.isSet(kOptAllowMultiple);
#endif

    // --- Parse graphics options ---
#ifdef Q_OS_WIN
    out.quietWindowsAsserts = parser.isSet(kOptNoWinAssertUI);
#endif

#if defined(Q_OS_WIN) || defined(Q_OS_MACOS)
    out.useSwRast = parser.isSet(kOptSwrast);
#endif

    return std::nullopt;
}

}  // namespace

CommandLineParseResult parseCommandLine()
{
    // Set application info (needed for --help and --version)
    QCoreApplication::setApplicationName(QLatin1String(QGC_APP_NAME));
    QCoreApplication::setApplicationVersion(QLatin1String(QGC_APP_VERSION_STR));

    return parseArguments(QCoreApplication::arguments());
}

CommandLineParseResult parseArguments(const QStringList& arguments)
{
    CommandLineParseResult out{};
    if (arguments.isEmpty()) {
        // Not even the program name. QCommandLineParser has no error text for such a list, so the result has none.
        out.statusCode = CommandLineParseResult::Status::Error;
        qCWarning(QGCCommandLineParserLog) << "The argument list is empty: it must hold at least the program name";
        return out;
    }

    out.parser = std::make_unique<QCommandLineParser>();

    QCommandLineParser& parser = *out.parser;
    parser.setSingleDashWordOptionMode(QCommandLineParser::ParseAsLongOptions);
    parser.setOptionsAfterPositionalArgumentsMode(QCommandLineParser::ParseAsOptions);
    parser.setApplicationDescription(QStringLiteral(QGC_APP_DESCRIPTION));

    // --- Standard options ---
    (void) parser.addHelpOption();
    (void) parser.addVersionOption();

    (void) parser.addOptions({
        // --- Core options (always available) ---
        {QString(kOptSystemId), QCoreApplication::translate("main", "MAVLink GCS system id."),
         QCoreApplication::translate("main", "id")},
        {QString(kOptClearSettings), QCoreApplication::translate("main", "Clear stored application settings.")},
        {QString(kOptClearCache), QCoreApplication::translate("main", "Clear parameter and airframe caches.")},
        {QString(kOptLogging), QCoreApplication::translate("main", "Enable logging with optional rules string."),
         QCoreApplication::translate("main", "rules")},
        {QString(kOptLogOutput), QCoreApplication::translate("main", "Log to console.")},
        {QString(kOptSimpleBoot), QCoreApplication::translate("main", "Initialize subsystems and exit.")},
#ifdef QGC_UNITTEST_BUILD
        // --- Test options (only in test builds) ---
        {QString(kOptUnittest), QCoreApplication::translate("main", "Run unit tests (optional filter value)."),
         QCoreApplication::translate("main", "filter")},
        {QString(kOptUnittestStress), QCoreApplication::translate("main", "Stress unit tests."),
         QCoreApplication::translate("main", "count")},
        {QString(kOptUnittestOutput),
         QCoreApplication::translate("main", "Output test results to file (JUnit XML format)."),
         QCoreApplication::translate("main", "file")},
        {QString(kOptUnittestShard),
         QCoreApplication::translate("main", "Run one shard of a single test's functions (1-based index/count)."),
         QCoreApplication::translate("main", "index/count")},
        {QString(kOptUnittestLabel),
         QCoreApplication::translate("main",
                                     "Filter tests by label (unit, integration, vehicle, missionmanager, etc.)."),
         QCoreApplication::translate("main", "labels")},
        {QString(kOptListTests), QCoreApplication::translate("main", "List available unit tests and exit.")},
        {QString(kOptOnscreen),
         QCoreApplication::translate("main", "Show test windows on screen instead of running offscreen.")},
#endif
#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
        // --- Desktop-only options ---
        {QString(kOptFakeMobile), QCoreApplication::translate("main", "Run with mobile-style UI.")},
        {QString(kOptAllowMultiple), QCoreApplication::translate("main", "Bypass single-instance guard.")},
#endif
#if defined(Q_OS_WIN) || defined(Q_OS_MACOS)
        // --- Windows/macOS options ---
        {QString(kOptSwrast), QCoreApplication::translate("main", "Force software rendering.")},
#endif
#ifdef Q_OS_WIN
        // --- Windows-only options ---
        {QString(kOptNoWinAssertUI), QCoreApplication::translate("main", "Disable Windows assert dialog boxes.")},
#endif
    });

    // --- Parse arguments ---
    std::optional<QString> error;
    if (parser.parse(normalizeArgs(arguments))) {
        // --- Handle help/version requests ---
        // Before any other check and the version first, as QCommandLineParser::process() answers them
        if (parser.isSet(kOptVersion)) {
            out.statusCode = CommandLineParseResult::Status::VersionRequested;
            out.versionText = QCoreApplication::applicationVersion();
            return out;
        }

        if (parser.isSet(kOptHelp) || parser.isSet(kOptHelpAll)) {
            out.statusCode = CommandLineParseResult::Status::HelpRequested;
            out.helpText = parser.helpText();
            return out;
        }

        error = readParsedOptions(parser, out);
    } else {
        out.unknownOptions = parser.unknownOptionNames();
        // Without an unknown option: an option without its value, or a value for an option that takes none
        error = out.unknownOptions.isEmpty() ? parser.errorText() : unknownOptionsError(out.unknownOptions);
    }

    if (error) {
        out.statusCode = CommandLineParseResult::Status::Error;
        out.errorString = error;
        qCWarning(QGCCommandLineParserLog) << *error;
        return out;
    }

    qCDebug(QGCCommandLineParserLog) << "Command line parsing completed successfully";

    return out;
}

CommandLineParseResult parse(int argc, char* argv[])
{
#ifdef QGC_UNITTEST_BUILD
    overrideCommandLine(argc, argv);
#endif
    const QCoreApplication app(argc, argv);
    CommandLineParseResult result = parseCommandLine();

    if ((result.statusCode == CommandLineParseResult::Status::HelpRequested) ||
        (result.statusCode == CommandLineParseResult::Status::VersionRequested)) {
        // Qt's help names the executable and, for --help-all, the generic options of the application object: it
        // takes both from the QCoreApplication, so the request is answered while this one exists.
        (void) handleParseResult(result);
    }

    return result;
}

std::optional<int> handleParseResult(const CommandLineParseResult& result)
{
    switch (result.statusCode) {
    case CommandLineParseResult::Status::Error: {
        const QString msg = result.errorString.value_or(QStringLiteral("Unknown error"));
        QCommandLineParser::showMessageAndExit(QCommandLineParser::MessageType::Error, msg + QLatin1Char('\n'), 1);
        Q_UNREACHABLE();
    }
    case CommandLineParseResult::Status::HelpRequested:
        if (result.parser) {
            if (result.parser->isSet(kOptHelp)) {
                result.parser->showHelp(EXIT_SUCCESS);
            }
            // --help-all: only process() shows the help that includes Qt's generic options. The first argument is
            // the program name, which the parser skips.
            result.parser->process(QStringList{QString(), QLatin1String("--") + kOptHelpAll});
        }
        return 0;
    case CommandLineParseResult::Status::VersionRequested:
        if (result.parser) {
            result.parser->showVersion();
        }
        return 0;
    case CommandLineParseResult::Status::Ok:
        return std::nullopt;
    }
    Q_UNREACHABLE();
}

AppMode determineAppMode(const CommandLineParseResult& args)
{
#ifdef QGC_UNITTEST_BUILD
    if (args.listTests) {
        return AppMode::ListTests;
    }
    if (args.runningUnitTests) {
        return AppMode::Test;
    }
#endif
    if (args.simpleBootTest) {
        return AppMode::BootTest;
    }
    return AppMode::Gui;
}

std::optional<quint32> parseSystemId(const QString& value)
{
    bool ok = false;
    const qulonglong systemId = value.toULongLong(&ok);
    if (!ok || (systemId < 1) || (systemId > std::numeric_limits<quint32>::max())) {
        return std::nullopt;
    }
    return static_cast<quint32>(systemId);
}

std::optional<std::pair<uint, uint>> parseUnitTestShard(const QString& value)
{
    const QStringList parts = value.split(QLatin1Char('/'));
    if (parts.size() != 2) {
        return std::nullopt;
    }
    bool indexOk = false;
    bool countOk = false;
    const uint index = parts.at(0).toUInt(&indexOk);
    const uint count = parts.at(1).toUInt(&countOk);
    if (!indexOk || !countOk || (index == 0) || (index > count)) {
        return std::nullopt;
    }
    return std::make_pair(index, count);
}

#ifdef QGC_UNITTEST_BUILD
void overrideCommandLine(int& argc, char**& argv)
{
    constexpr bool enabled = false;  // Set to true to enable
    if constexpr (!enabled) {
        return;
    }

    static char arg0[256];
    static char arg1[] = "--unittest:FTPManagerTest";
    static char arg2[] = "--logging:Vehicle.FTPManager";
    static char* newArgv[] = { arg0, arg1, arg2, nullptr };

    qstrncpy(arg0, argv[0], sizeof(arg0));
    argc = 3;
    argv = newArgv;
}
#endif

} // namespace QGCCommandLineParser
