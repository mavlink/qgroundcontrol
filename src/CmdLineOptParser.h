#pragma once

#include <QtCore/QCommandLineParser>

struct CommandLineParseResult
{
    enum class Status {
        Ok,
        Error,
        VersionRequested,
        HelpRequested
    };
    Status statusCode = Status::Ok;
    std::optional<QString> errorString = std::nullopt;

    bool runningUnitTests = false;
    QStringList unitTests = {};
    bool stressUnitTests = false;
    int stressUnitTestsCount = 0;
    bool clearSettingsOptions = false;
    bool clearCache = false;
    bool logging = false;
    QString loggingOptions = "";
    bool fakeMobile = false;
    bool logOutput = false;
    bool quietWindowsAsserts = false;
};

CommandLineParseResult parseCommandLine(QCommandLineParser &parser);
