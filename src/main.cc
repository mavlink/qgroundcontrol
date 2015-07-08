/*=====================================================================

QGroundControl Open Source Ground Control Station

(c) 2009 - 2011 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

This file is part of the QGROUNDCONTROL project

    QGROUNDCONTROL is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    QGROUNDCONTROL is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.

======================================================================*/

/**
 * @file
 *   @brief Main executable
 *   @author Lorenz Meier <mavteam@student.ethz.ch>
 *
 */

#include <QtGlobal>
#include <QApplication>
#include <QSslSocket>
#ifndef __mobile__
#include <QSerialPortInfo>
#endif
#include <QProcessEnvironment>
#include "QGCApplication.h"
#include "MainWindow.h"
#include "configuration.h"
#ifdef QT_DEBUG
#ifndef __mobile__
#include "UnitTest.h"
#endif
#include "CmdLineOptParser.h"
#ifdef Q_OS_WIN
#include <crtdbg.h>
#endif
#endif

/* SDL does ugly things to main() */
#ifdef main
#undef main
#endif

#ifndef __mobile__
Q_DECLARE_METATYPE(QSerialPortInfo)
#endif

#ifdef Q_OS_WIN

/// @brief Message handler which is installed using qInstallMsgHandler so you do not need
/// the MSFT debug tools installed to see qDebug(), qWarning(), qCritical and qAbort
void msgHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    const char symbols[] = { 'I', 'E', '!', 'X' };
    QString output = QString("[%1] at %2:%3 - \"%4\"").arg(symbols[type]).arg(context.file).arg(context.line).arg(msg);
    std::cerr << output.toStdString() << std::endl;
    if( type == QtFatalMsg ) abort();
}

/// @brief CRT Report Hook installed using _CrtSetReportHook. We install this hook when
/// we don't want asserts to pop a dialog on windows.
int WindowsCrtReportHook(int reportType, char* message, int* returnValue)
{
    Q_UNUSED(reportType);

    std::cerr << message << std::endl;  // Output message to stderr
    *returnValue = 0;                   // Don't break into debugger
    return true;                        // We handled this fully ourselves
}

#endif

/**
 * @brief Starts the application
 *
 * @param argc Number of commandline arguments
 * @param argv Commandline arguments
 * @return exit code, 0 for normal exit and !=0 for error cases
 */

int main(int argc, char *argv[])
{

#ifdef Q_OS_MAC
#ifndef __ios__
    // Prevent Apple's app nap from screwing us over
    // tip: the domain can be cross-checked on the command line with <defaults domains>
    QProcess::execute("defaults write org.qgroundcontrol.qgroundcontrol NSAppSleepDisabled -bool YES");
#endif
#endif

    // install the message handler
#ifdef Q_OS_WIN
    qInstallMessageHandler(msgHandler);
#endif

    // The following calls to qRegisterMetaType are done to silence debug output which warns
    // that we use these types in signals, and without calling qRegisterMetaType we can't queue
    // these signals. In general we don't queue these signals, but we do what the warning says
    // anyway to silence the debug output.
#ifndef __ios__
    qRegisterMetaType<QSerialPort::SerialPortError>();
#endif
    qRegisterMetaType<QAbstractSocket::SocketError>();
#ifndef __mobile__
    qRegisterMetaType<QSerialPortInfo>();
#endif
    
    // We statically link to the google QtLocation plugin

#ifdef Q_OS_WIN
    // In Windows, the compiler doesn't see the use of the class created by Q_IMPORT_PLUGIN
#pragma warning( disable : 4930 4101 )
#endif

    Q_IMPORT_PLUGIN(QGeoServiceProviderFactoryQGC)

    bool runUnitTests = false;          // Run unit tests

#ifdef QT_DEBUG
    // We parse a small set of command line options here prior to QGCApplication in order to handle the ones
    // which need to be handled before a QApplication object is started.

    bool quietWindowsAsserts = false;   // Don't let asserts pop dialog boxes

    CmdLineOpt_t rgCmdLineOptions[] = {
        { "--unittest",             &runUnitTests,          QString() },
        { "--no-windows-assert-ui", &quietWindowsAsserts,   QString() },
        // Add additional command line option flags here
    };

    ParseCmdLineOptions(argc, argv, rgCmdLineOptions, sizeof(rgCmdLineOptions)/sizeof(rgCmdLineOptions[0]), false);

    if (quietWindowsAsserts) {
#ifdef Q_OS_WIN
        _CrtSetReportHook(WindowsCrtReportHook);
#endif
    }

#ifdef Q_OS_WIN
    if (runUnitTests) {
        // Don't pop up Windows Error Reporting dialog when app crashes. This prevents TeamCity from
        // hanging.
        DWORD dwMode = SetErrorMode(SEM_NOGPFAULTERRORBOX);
        SetErrorMode(dwMode | SEM_NOGPFAULTERRORBOX);
    }
#endif
#endif // QT_DEBUG

    QGCApplication* app = new QGCApplication(argc, argv, runUnitTests);
    Q_CHECK_PTR(app);

    // There appears to be a threading issue in qRegisterMetaType which can cause it to throw a qWarning
    // about duplicate type converters. This is caused by a race condition in the Qt code. Still working
    // with them on tracking down the bug. For now we register the type which is giving us problems here
    // while we only have the main thread. That should prevent it from hitting the race condition later
    // on in the code.
    qRegisterMetaType<QList<QPair<QByteArray,QByteArray> > >();

    app->_initCommon();

    int exitCode;

#ifndef __mobile__
#ifdef QT_DEBUG
    if (runUnitTests) {
        if (!app->_initForUnitTests()) {
            return -1;
        }

        // Run the test
        int failures = UnitTest::run(rgCmdLineOptions[0].optionArg);
        if (failures == 0) {
            qDebug() << "ALL TESTS PASSED";
        } else {
            qDebug() << failures << " TESTS FAILED!";
        }
        exitCode = -failures;
    } else
#endif
#endif
    {
        if (!app->_initForNormalAppBoot()) {
            return -1;
        }
        exitCode = app->exec();
    }

    delete app;

    qDebug() << "After app delete";

    return exitCode;
}
