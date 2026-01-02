#pragma once

#include <optional>

#include <QtCore/QCommandLineParser>
#include <QtCore/QStringList>

namespace QGCCommandLineParser {

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

    std::optional<quint8> systemId;
    bool clearSettingsOptions = false;
    bool clearCache = false;
    std::optional<QString> loggingOptions;
    bool logOutput = false;
    bool simpleBootTest = false;

    bool runningUnitTests = false;
    QStringList unitTests;
    bool stressUnitTests = false;
    uint stressUnitTestsCount = 0;

    bool fakeMobile = false;
    bool allowMultiple = false;

    bool useDesktopGL = false;
    bool useSwRast = false;
    bool quietWindowsAsserts = false;
};

/// Parse the application's command-line arguments into result.
CommandLineParseResult parseCommandLine();

} // namespace QGCCommandLineParser
