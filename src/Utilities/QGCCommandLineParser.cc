#include "QGCCommandLineParser.h"

#include <QtCore/QCommandLineOption>
#include <QtCore/QCoreApplication>

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(CommandLineParserLog, "Utilities.QGCCommandLineParser")

namespace QGCCommandLineParser {

// ============================================================================
// Option Name Constants
// ============================================================================

namespace {

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
#endif

#ifdef Q_OS_WIN
// --- Windows-only options ---
constexpr QLatin1StringView kOptDesktop       = QLatin1StringView("desktop");
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
static QStringList normalizeArgs(const QStringList &args)
{
    QStringList out;
    out.reserve(args.size() + 4);

    for (const QString &arg : args) {
#ifdef QGC_UNITTEST_BUILD
        // Handle bare --unittest without a value
        if (arg == QLatin1String("--unittest")) {
            out << arg << QString(); // empty value token prevents "Missing value"
            continue;
        }
#endif

        // Convert --option:value to --option value
        if (arg.startsWith(QLatin1String("--")) && arg.contains(QLatin1Char(':'))) {
            const qsizetype idx = arg.indexOf(QLatin1Char(':'));
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

    qCDebug(CommandLineParserLog) << "Normalized arguments:" << out;
    return out;
}

// ============================================================================
// Command Line Parsing
// ============================================================================

CommandLineParseResult parseCommandLine()
{
    CommandLineParseResult out{};
    out.parser = std::make_unique<QCommandLineParser>();

    QCommandLineParser& parser = *out.parser;
    parser.setSingleDashWordOptionMode(QCommandLineParser::ParseAsLongOptions);
    parser.setOptionsAfterPositionalArgumentsMode(QCommandLineParser::ParseAsOptions);
    parser.setApplicationDescription(QStringLiteral(QGC_APP_DESCRIPTION));

    // --- Standard options ---
    const QCommandLineOption helpOption = parser.addHelpOption();
    const QCommandLineOption versionOption = parser.addVersionOption();

    // --- Core options (always available) ---
    const QCommandLineOption systemIdOpt(
        QString(kOptSystemId),
        QCoreApplication::translate("main", "MAVLink GCS system id."),
        QCoreApplication::translate("main", "id"));
    (void) parser.addOption(systemIdOpt);

    const QCommandLineOption clearSettingsOpt(
        QString(kOptClearSettings),
        QCoreApplication::translate("main", "Clear stored application settings."));
    (void) parser.addOption(clearSettingsOpt);

    const QCommandLineOption clearCacheOpt(
        QString(kOptClearCache),
        QCoreApplication::translate("main", "Clear parameter and airframe caches."));
    (void) parser.addOption(clearCacheOpt);

    const QCommandLineOption loggingOpt(
        QString(kOptLogging),
        QCoreApplication::translate("main", "Enable logging with optional rules string."),
        QCoreApplication::translate("main", "rules"));
    (void) parser.addOption(loggingOpt);

    const QCommandLineOption logOutputOpt(
        QString(kOptLogOutput),
        QCoreApplication::translate("main", "Log to console."));
    (void) parser.addOption(logOutputOpt);

    const QCommandLineOption simpleBootOpt(
        QString(kOptSimpleBoot),
        QCoreApplication::translate("main", "Initialize subsystems and exit."));
    (void) parser.addOption(simpleBootOpt);

#ifdef QGC_UNITTEST_BUILD
    // --- Test options (only in test builds) ---
    const QCommandLineOption unittestOpt(
        QString(kOptUnittest),
        QCoreApplication::translate("main", "Run unit tests (optional filter value)."),
        QCoreApplication::translate("main", "filter"));
    (void) parser.addOption(unittestOpt);

    const QCommandLineOption unittestStressOpt(
        QString(kOptUnittestStress),
        QCoreApplication::translate("main", "Stress unit tests."),
        QCoreApplication::translate("main", "count"));
    (void) parser.addOption(unittestStressOpt);

    const QCommandLineOption unittestOutputOpt(
        QString(kOptUnittestOutput),
        QCoreApplication::translate("main", "Output test results to file (JUnit XML format)."),
        QCoreApplication::translate("main", "file"));
    (void) parser.addOption(unittestOutputOpt);
#endif

#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
    // --- Desktop-only options ---
    const QCommandLineOption fakeMobileOpt(
        QString(kOptFakeMobile),
        QCoreApplication::translate("main", "Run with mobile-style UI."));
    (void) parser.addOption(fakeMobileOpt);

    const QCommandLineOption allowMultipleOpt(
        QString(kOptAllowMultiple),
        QCoreApplication::translate("main", "Bypass single-instance guard."));
    (void) parser.addOption(allowMultipleOpt);
#endif

#if defined(Q_OS_WIN) || defined(Q_OS_MACOS)
    // --- Windows/macOS options ---
    const QCommandLineOption swrastOpt(
        QString(kOptSwrast),
        QCoreApplication::translate("main", "Force software OpenGL."));
    (void) parser.addOption(swrastOpt);
#endif

#ifdef Q_OS_WIN
    // --- Windows-only options ---
    const QCommandLineOption desktopOpt(
        QString(kOptDesktop),
        QCoreApplication::translate("main", "Force Desktop OpenGL."));
    (void) parser.addOption(desktopOpt);

    const QCommandLineOption quietWinAssertOpt(
        QString(kOptNoWinAssertUI),
        QCoreApplication::translate("main", "Disable Windows assert dialog boxes."));
    (void) parser.addOption(quietWinAssertOpt);
#endif

    // --- Parse arguments ---
    const QStringList normalizedArgs = normalizeArgs(QCoreApplication::arguments());
    parser.process(normalizedArgs);

    // --- Validate unknown options ---
    out.unknownOptions = parser.unknownOptionNames();

    // Check for platform-specific option errors on wrong platform
    if (!out.unknownOptions.isEmpty()) {
        // Check for test options in non-test build
        if (out.unknownOptions.contains(QLatin1String("unittest")) ||
            out.unknownOptions.contains(QLatin1String("unittest-stress")) ||
            out.unknownOptions.contains(QLatin1String("unittest-output"))) {
#ifndef QGC_UNITTEST_BUILD
            out.statusCode = CommandLineParseResult::Status::Error;
            out.errorString = QCoreApplication::translate("main",
                "--unittest/--unittest-stress/--unittest-output options are only available in unittest builds.");
            qCWarning(CommandLineParserLog) << out.errorString.value();
            return out;
#endif
        }

#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
        // Mobile platforms don't support desktop options
        if (out.unknownOptions.contains(QLatin1String("fake-mobile")) ||
            out.unknownOptions.contains(QLatin1String("allow-multiple"))) {
            out.statusCode = CommandLineParseResult::Status::Error;
            out.errorString = QCoreApplication::translate("main",
                "--fake-mobile/--allow-multiple are not supported on mobile platforms.");
            qCWarning(CommandLineParserLog) << out.errorString.value();
            return out;
        }
#endif

#ifndef Q_OS_WIN
        // Non-Windows platforms don't support Windows-specific options
        if (out.unknownOptions.contains(QLatin1String("desktop")) ||
            out.unknownOptions.contains(QLatin1String("no-windows-assert-ui"))) {
            out.statusCode = CommandLineParseResult::Status::Error;
            out.errorString = QCoreApplication::translate("main",
                "--desktop/--no-windows-assert-ui are only supported on Windows.");
            qCWarning(CommandLineParserLog) << out.errorString.value();
            return out;
        }
#endif

#if !defined(Q_OS_WIN) && !defined(Q_OS_MACOS)
        // Non-Windows/macOS platforms don't support swrast
        if (out.unknownOptions.contains(QLatin1String("swrast"))) {
            out.statusCode = CommandLineParseResult::Status::Error;
            out.errorString = QCoreApplication::translate("main",
                "--swrast is only supported on Windows and macOS.");
            qCWarning(CommandLineParserLog) << out.errorString.value();
            return out;
        }
#endif

        // Generic unknown options error
        out.statusCode = CommandLineParseResult::Status::Error;
        out.errorString = QCoreApplication::translate("main", "Unknown options: %1")
            .arg(out.unknownOptions.join(QLatin1String(", ")));
        qCWarning(CommandLineParserLog) << out.errorString.value();
        return out;
    }

    // --- Validate positional arguments ---
    out.positional = parser.positionalArguments();
    if (!out.positional.isEmpty()) {
        out.statusCode = CommandLineParseResult::Status::Error;
        out.errorString = QCoreApplication::translate("main", "Unexpected positional arguments: %1")
            .arg(out.positional.join(QLatin1String(", ")));
        qCWarning(CommandLineParserLog) << out.errorString.value();
        return out;
    }

    // --- Handle help/version requests ---
    if (parser.isSet(helpOption)) {
        out.statusCode = CommandLineParseResult::Status::HelpRequested;
        out.helpText = parser.helpText();
        return out;
    }

    if (parser.isSet(versionOption)) {
        out.statusCode = CommandLineParseResult::Status::VersionRequested;
        out.versionText = QCoreApplication::applicationVersion();
        return out;
    }

    // --- Parse core options ---
    if (parser.isSet(systemIdOpt)) {
        const QString systemIdStr = parser.value(systemIdOpt);
        bool ok = false;
        const uint systemId = systemIdStr.toUInt(&ok);
        if (!ok || (systemId < 1) || (systemId > 255)) {
            out.statusCode = CommandLineParseResult::Status::Error;
            out.errorString = QCoreApplication::translate("main", "Invalid System ID (must be 1-255): %1")
                .arg(systemIdStr);
            qCWarning(CommandLineParserLog) << out.errorString.value();
            return out;
        }
        out.systemId = static_cast<quint8>(systemId);
        qCDebug(CommandLineParserLog) << "System ID:" << systemId;
    }

    out.clearSettingsOptions = parser.isSet(clearSettingsOpt);
    out.clearCache = parser.isSet(clearCacheOpt);
    if (parser.isSet(loggingOpt)) {
        out.loggingOptions = parser.value(loggingOpt);
        qCDebug(CommandLineParserLog) << "Logging options:" << out.loggingOptions.value();
    }
    out.logOutput = parser.isSet(logOutputOpt);
    out.simpleBootTest = parser.isSet(simpleBootOpt);

#ifdef QGC_UNITTEST_BUILD
    // --- Parse test options ---
    if (parser.isSet(unittestOpt)) {
        out.runningUnitTests = true;
        const QStringList vals = parser.values(unittestOpt);
        // Filter out empty values (from bare --unittest)
        for (const QString& val : vals) {
            if (!val.isEmpty()) {
                out.unitTests.append(val);
            }
        }
        qCDebug(CommandLineParserLog) << "Unit tests:" << (out.unitTests.isEmpty() ? QStringLiteral("all") : out.unitTests.join(QLatin1String(", ")));
    }

    if (parser.isSet(unittestStressOpt)) {
        out.runningUnitTests = true;
        out.stressUnitTests = true;
        const QString stress = parser.value(unittestStressOpt);
        if (stress.isEmpty()) {
            out.stressUnitTestsCount = 20;
        } else {
            bool ok = false;
            const uint count = stress.toUInt(&ok);
            if (!ok || (count == 0)) {
                out.statusCode = CommandLineParseResult::Status::Error;
                out.errorString = QCoreApplication::translate("main", "Invalid stress test count (must be > 0): %1")
                    .arg(stress);
                qCWarning(CommandLineParserLog) << out.errorString.value();
                return out;
            }
            out.stressUnitTestsCount = count;
        }
        qCDebug(CommandLineParserLog) << "Stress test iterations:" << out.stressUnitTestsCount;
    }

    if (parser.isSet(unittestOutputOpt)) {
        out.unitTestOutput = parser.value(unittestOutputOpt);
        qCDebug(CommandLineParserLog) << "Test output file:" << out.unitTestOutput.value();
    }
#endif

    // --- Parse desktop options ---
#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
    out.fakeMobile = parser.isSet(fakeMobileOpt);
    out.allowMultiple = parser.isSet(allowMultipleOpt);
#else
    out.fakeMobile = false;
    out.allowMultiple = false;
#endif

    // --- Parse graphics options ---
#ifdef Q_OS_WIN
    out.useDesktopGL = parser.isSet(desktopOpt);
    out.quietWindowsAsserts = parser.isSet(quietWinAssertOpt);
#else
    out.useDesktopGL = false;
    out.quietWindowsAsserts = false;
#endif

#if defined(Q_OS_WIN) || defined(Q_OS_MACOS)
    out.useSwRast = parser.isSet(swrastOpt);
#else
    out.useSwRast = false;
#endif

    out.statusCode = CommandLineParseResult::Status::Ok;
    qCDebug(CommandLineParserLog) << "Command line parsing completed successfully";

    return out;
}

} // namespace QGCCommandLineParser
