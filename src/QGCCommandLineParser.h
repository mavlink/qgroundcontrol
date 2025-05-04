/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QCommandLineParser>
#include <QtCore/QStringList>
#include <optional>

namespace QGCCommandLineParser {

struct CommandLineParseResult {
    QCommandLineParser parser;

    enum class Status {
        Ok,
        Error,
        VersionRequested,
        HelpRequested
    } statusCode = Status::Ok;

    std::optional<QString> errorString;

    bool runningUnitTests        = false;
    QStringList unitTests;

    bool stressUnitTests         = false;
    unsigned stressUnitTestsCount = 0;

    bool clearSettingsOptions    = false;
    bool clearCache              = false;

    bool logging                 = false;
    QString loggingOptions;

    bool fakeMobile              = false;
    bool logOutput               = false;

    bool simpleBootTest          = false;

#ifdef Q_OS_WIN
    bool quietWindowsAsserts     = false;
#endif
};

/// Parse the application's command-line arguments into parseResult.
/// After calling, inspect parseResult.statusCode and other members.
void parseCommandLine(CommandLineParseResult &parseResult);

} // namespace QGCCommandLineParser
