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

namespace QGCCommandLineParser {

struct CommandLineParseResult {

    enum class Status {
        Ok,
        Error,
        VersionRequested,
        HelpRequested
    } statusCode = Status::Ok;

    QString errorString;
    QString helpText;
    QString versionText;

    bool runningUnitTests = false;
    QStringList unitTests;

    bool stressUnitTests = false;
    unsigned stressUnitTestsCount = 0;

    bool clearSettingsOptions = false;
    bool clearCache = false;

    bool logging = false;
    QString loggingOptions;

    bool fakeMobile = false;
    bool logOutput = false;

    bool simpleBootTest = false;

    bool allowMultiple = false; // bypass RunGuard
    bool hasSystemId = false;
    QString systemIdStr;

#ifdef Q_OS_WIN
    bool quietWindowsAsserts = false; // only honored in debug builds
    bool useDesktopGL = false;
    bool useSwRast = false;
#endif

    QStringList positional;
    QStringList unknownOptions;

    QCommandLineParser parser;
};

/// Parse the application's command-line arguments into result.
/// After calling, inspect parseResult.statusCode and other members.
void parseCommandLine(const QStringList &args, CommandLineParseResult &result);

} // namespace QGCCommandLineParser
