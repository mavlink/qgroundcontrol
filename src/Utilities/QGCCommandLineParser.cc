/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "QGCCommandLineParser.h"

#include <QtCore/QCommandLineOption>
#include <QtCore/QCoreApplication>

namespace QGCCommandLineParser {

static const QString kOptSystemId        = QStringLiteral("system-id");
static const QString kOptClearSettings   = QStringLiteral("clear-settings");
static const QString kOptClearCache      = QStringLiteral("clear-cache");
static const QString kOptLogging         = QStringLiteral("logging");
static const QString kOptLogOutput       = QStringLiteral("log-output");
static const QString kOptSimpleBoot      = QStringLiteral("simple-boot-test");
static const QString kOptFakeMobile      = QStringLiteral("fake-mobile");
static const QString kOptAllowMultiple   = QStringLiteral("allow-multiple");
static const QString kOptUnittest        = QStringLiteral("unittest");
static const QString kOptUnittestStress  = QStringLiteral("unittest-stress");
static const QString kOptDesktop         = QStringLiteral("desktop");
static const QString kOptSwrast          = QStringLiteral("swrast");
static const QString kOptNoWinAssertUI   = QStringLiteral("no-windows-assert-ui");

static QStringList normalizeArgs(const QStringList &args)
{
    QStringList out;
    out.reserve(args.size() + 4);
    for (const QString &arg : args) {
        if (arg == QStringLiteral("--unittest")) {
            out << arg << QString(); // empty value token prevents "Missing value"
            continue;
        }

        if (arg.startsWith("--") && arg.contains(':')) {
            const int idx = arg.indexOf(':');
            const QString opt = arg.left(idx);
            const QString val = arg.mid(idx + 1);
            out << opt;
            if (!val.isEmpty()) {
                out << val;
            } else if (opt == QStringLiteral("--unittest")) {
                out << QString();
            }
        } else {
            out << arg;
        }
    }
    return out;
}

CommandLineParseResult parseCommandLine()
{
    CommandLineParseResult out{};
    out.parser = std::make_unique<QCommandLineParser>();

    QCommandLineParser& parser = *out.parser;
    parser.setSingleDashWordOptionMode(QCommandLineParser::ParseAsLongOptions);
    parser.setOptionsAfterPositionalArgumentsMode(QCommandLineParser::ParseAsOptions);
    parser.setApplicationDescription(QStringLiteral(QGC_APP_DESCRIPTION));

    const QCommandLineOption helpOption = parser.addHelpOption();
    const QCommandLineOption versionOption = parser.addVersionOption();

    const QCommandLineOption systemIdOpt(
        kOptSystemId,
        QCoreApplication::translate("main", "MAVLink GCS system id."),
        QCoreApplication::translate("main", "id"));
    (void) parser.addOption(systemIdOpt);

    const QCommandLineOption clearSettingsOpt(
        kOptClearSettings,
        QCoreApplication::translate("main", "Clear stored application settings."));
    (void) parser.addOption(clearSettingsOpt);

    const QCommandLineOption clearCacheOpt(
        kOptClearCache,
        QCoreApplication::translate("main", "Clear parameter and airframe caches."));
    (void) parser.addOption(clearCacheOpt);

    const QCommandLineOption loggingOpt(
        kOptLogging,
        QCoreApplication::translate("main", "Enable logging with optional rules string."),
        QCoreApplication::translate("main", "rules"));
    (void) parser.addOption(loggingOpt);

    const QCommandLineOption logOutputOpt(
        kOptLogOutput,
        QCoreApplication::translate("main", "Log to console."));
    (void) parser.addOption(logOutputOpt);

    const QCommandLineOption simpleBootOpt(
        kOptSimpleBoot,
        QCoreApplication::translate("main", "Initialize subsystems and exit."));
    (void) parser.addOption(simpleBootOpt);

#if defined(QGC_UNITTEST_BUILD)
    const QCommandLineOption unittestOpt(
        kOptUnittest,
        QCoreApplication::translate("main", "Run unit tests (optional filter value)."),
        QCoreApplication::translate("main", "filter"));
    (void) parser.addOption(unittestOpt);

    const QCommandLineOption unittestStressOpt(
        kOptUnittestStress,
        QCoreApplication::translate("main", "Stress unit tests."),
        QCoreApplication::translate("main", "count"));
    (void) parser.addOption(unittestStressOpt);
#endif

#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
    const QCommandLineOption fakeMobileOpt(
        kOptFakeMobile,
        QCoreApplication::translate("main", "Run with mobile-style UI."));
    (void) parser.addOption(fakeMobileOpt);

    const QCommandLineOption allowMultipleOpt(
        kOptAllowMultiple,
        QCoreApplication::translate("main", "Bypass single-instance guard."));
    (void) parser.addOption(allowMultipleOpt);
#endif

#if defined(Q_OS_WIN) || defined(Q_OS_MACOS)
    const QCommandLineOption swrastOpt(
        kOptSwrast,
        QCoreApplication::translate("main", "Force software OpenGL."));
    (void) parser.addOption(swrastOpt);
#endif

#ifdef Q_OS_WIN
    const QCommandLineOption desktopOpt(
        kOptDesktop,
        QCoreApplication::translate("main", "Force Desktop OpenGL."));
    (void) parser.addOption(desktopOpt);

    const QCommandLineOption quietWinAssertOpt(
        kOptNoWinAssertUI,
        QCoreApplication::translate("main", "Disable Windows assert dialog boxes."));
    (void) parser.addOption(quietWinAssertOpt);
#endif

    const QStringList normalizedArgs = normalizeArgs(QCoreApplication::arguments());
    parser.process(normalizedArgs);

    out.unknownOptions = parser.unknownOptionNames();
#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
    if (out.unknownOptions.contains(kOptFakeMobile) || out.unknownOptions.contains(kOptAllowMultiple)) {
        out.statusCode = CommandLineParseResult::Status::Error;
        out.errorString = QCoreApplication::translate("main", "--%1/--%2 are not supported on this platform.").arg(kOptFakeMobile, kOptAllowMultiple);
        return out;
    }
#endif
#ifndef Q_OS_WIN
    if (out.unknownOptions.contains(kOptDesktop) || out.unknownOptions.contains(kOptSwrast) || out.unknownOptions.contains(kOptNoWinAssertUI)) {
        out.statusCode = CommandLineParseResult::Status::Error;
        out.errorString = QCoreApplication::translate("main", "--%1/--%2/--%3 are only supported on Windows.").arg(kOptDesktop, kOptSwrast, kOptNoWinAssertUI);
        return out;
    }
#endif
#if !defined(QGC_UNITTEST_BUILD)
    if (out.unknownOptions.contains(kOptUnittest) || out.unknownOptions.contains(kOptUnittestStress)) {
        out.statusCode = CommandLineParseResult::Status::Error;
        out.errorString = QCoreApplication::translate("main", "--%1/--%2 options are only available in unittest builds.").arg(kOptUnittest, kOptUnittestStress);
        return out;
    }
#endif
    if (!out.unknownOptions.isEmpty()) {
        out.statusCode = CommandLineParseResult::Status::Error;
        out.errorString = QCoreApplication::translate("main", "Unknown options: %1").arg(out.unknownOptions.join(", "));
        return out;
    }

    out.positional = parser.positionalArguments();
    if (!out.positional.isEmpty()) {
        out.statusCode = CommandLineParseResult::Status::Error;
        out.errorString = QCoreApplication::translate("main", "Unexpected positional arguments: %1").arg(out.positional.join(", "));
        return out;
    }

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

    if (parser.isSet(systemIdOpt)) {
        const QString systemIdStr = parser.value(systemIdOpt);
        bool ok = false;
        const uint systemId = systemIdStr.toUInt(&ok);
        if (!ok || (systemId < 1) || (systemId > 255)) {
            out.statusCode = CommandLineParseResult::Status::Error;
            out.errorString = QCoreApplication::translate("main", "Invalid System ID (must be 1-255): %1").arg(systemIdStr);
            return out;
        }
        out.systemId = static_cast<quint8>(systemId);
    }

    out.clearSettingsOptions = parser.isSet(clearSettingsOpt);
    out.clearCache = parser.isSet(clearCacheOpt);
    if (parser.isSet(loggingOpt)) {
        out.loggingOptions = parser.value(loggingOpt);
    }
    out.logOutput = parser.isSet(logOutputOpt);
    out.simpleBootTest = parser.isSet(simpleBootOpt);

#if defined(QGC_UNITTEST_BUILD)
    if (parser.isSet(unittestOpt)) {
        out.runningUnitTests = true;
        const QStringList vals = parser.values(unittestOpt);
        if (vals.isEmpty()) { // || ((vals.size() == 1) && vals.first().isEmpty())
            // No ":name" given → run all tests
            out.unitTests.clear();
        } else {
            // One or more ":name" values provided
            out.unitTests = vals;
        }
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
                out.errorString = QCoreApplication::translate("main", "Invalid stress unit test count: %1").arg(count);
                return out;
            }
            out.stressUnitTestsCount = count;
        }
    }
#endif

#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
    out.fakeMobile = parser.isSet(fakeMobileOpt);
    out.allowMultiple = parser.isSet(allowMultipleOpt);
#endif

#ifdef Q_OS_WIN
    out.useDesktopGL = parser.isSet(desktopOpt);
    out.quietWindowsAsserts = parser.isSet(quietWinAssertOpt);
#endif

#if defined(Q_OS_WIN) || defined(Q_OS_MACOS)
    out.useSwRast = parser.isSet(swrastOpt);
#endif

    out.statusCode = CommandLineParseResult::Status::Ok;

    return out;
}

} // namespace QGCCommandLineParser
