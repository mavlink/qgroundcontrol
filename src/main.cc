/****************************************************************************
 *
 * (c) 2009-2024 QGroundControl Project
 *
 ****************************************************************************/

#include <QtQuick/QQuickWindow>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMessageBox>
#include <QDebug>
#include <QFile>
#include <QTextStream>

#ifdef Q_OS_MACOS
#include <QtCore/QProcessEnvironment>
#endif

#ifdef Q_OS_WIN
#include <windows.h>
#include <cstdio>
#include <iostream>
#ifdef QT_DEBUG
#include <crtdbg.h>
#endif
#endif

#include "QGCApplication.h"
#include "QGCLogging.h"
#include "CmdLineOptParser.h"
#include "SettingsManager.h"
#include "MavlinkSettings.h"

#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
#include "RunGuard.h"
#endif

#ifdef Q_OS_ANDROID
#include "AndroidInterface.h"
#endif

#ifdef Q_OS_LINUX
#ifndef Q_OS_ANDROID
#include "SignalHandler.h"
#endif
#endif

#ifdef QGC_UNITTEST_BUILD
#include "UnitTestList.h"
#endif

//-----------------------------------------------------------------------------
// Log everything to file
//-----------------------------------------------------------------------------

static void logHandler(QtMsgType type,
                       const QMessageLogContext &context,
                       const QString &msg)
{
    Q_UNUSED(context);

    QString logPath = QStringLiteral("C:/projects/qgroundcontrol/build/Release/qgc_startup.log");
    QFile file(logPath);

    if(file.open(QIODevice::Append | QIODevice::Text))
    {
        QTextStream out(&file);

        switch(type)
        {
        case QtDebugMsg:
            out << "[DEBUG] ";
            break;

        case QtInfoMsg:
            out << "[INFO ] ";
            break;

        case QtWarningMsg:
            out << "[WARN ] ";
            break;

        case QtCriticalMsg:
            out << "[ERROR] ";
            break;

        case QtFatalMsg:
            out << "[FATAL] ";
            break;
        }

        out << msg << Qt::endl;
        out.flush();
        file.flush();
        file.close();
    }

#ifdef Q_OS_WIN
    OutputDebugStringW((msg + "\n").toStdWString().c_str());
#endif

    fprintf(stderr,"%s\n",msg.toLocal8Bit().constData());
    fflush(stderr);
}

//-----------------------------------------------------------------------------

#ifdef QT_DEBUG
#ifdef Q_OS_WIN

int WindowsCrtReportHook(int reportType, char* message, int* returnValue)
{
    Q_UNUSED(reportType);

    std::cerr << message << std::endl;

    *returnValue = 0;

    return true;
}

#endif
#endif

//-----------------------------------------------------------------------------

int main(int argc, char *argv[])
{

#ifdef Q_OS_WIN

    AttachConsole(ATTACH_PARENT_PROCESS);

    freopen("CONOUT$","w",stdout);
    freopen("CONOUT$","w",stderr);

#endif

    qInstallMessageHandler(logHandler);

    qInfo() << "======================================";
    qInfo() << "VoladorGroundControl Startup";
    qInfo() << "======================================";
    qInfo() << "[MAIN] Entered main()";

    bool runUnitTests = false;
    bool simpleBootTest = false;

    QString systemIdStr;
    bool hasSystemId = false;
    bool bypassRunGuard = false;

    bool stressUnitTests = false;
    bool quietWindowsAsserts = false;
    QString unitTestOptions;

    CmdLineOpt_t rgCmdLineOptions[] =
    {
#ifdef QT_DEBUG
        { "--unittest",             &runUnitTests,        &unitTestOptions },
        { "--unittest-stress",      &stressUnitTests,     &unitTestOptions },
        { "--no-windows-assert-ui", &quietWindowsAsserts, nullptr },
#endif
        { "--allow-multiple",       &bypassRunGuard,      nullptr },
        { "--system-id",            &hasSystemId,         &systemIdStr },
        { "--simple-boot-test",     &simpleBootTest,      nullptr },
    };

    ParseCmdLineOptions(
        argc,
        argv,
        rgCmdLineOptions,
        std::size(rgCmdLineOptions),
        false);

#ifdef QT_DEBUG
#ifdef Q_OS_WIN

    if(quietWindowsAsserts)
        _CrtSetReportHook(WindowsCrtReportHook);

#endif
#endif

    qInfo() << "[MAIN] Creating QGCApplication";

    QGCApplication app(
        argc,
        argv,
        runUnitTests,
        simpleBootTest);

#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)

    const QString runguardString =
            QStringLiteral("%1 RunGuardKey")
            .arg(QGC_APP_NAME);

    qInfo() << "[MAIN] RunGuard Key =" << runguardString;

    RunGuard guard(runguardString);

    if(!bypassRunGuard && !guard.tryToRun())
    {
        qCritical() << "[MAIN] RunGuard FAILED";

        QMessageBox::critical(
                    nullptr,
                    QObject::tr("Error"),
                    QObject::tr(
                        "Another instance of %1 is already running.")
                        .arg(QGC_APP_NAME));

        return -1;
    }

    qInfo() << "[MAIN] RunGuard OK";

#endif

    qInfo() << "[MAIN] Calling app.init()";

    app.init();

    qInfo() << "[MAIN] app.init() returned";

    int exitCode = 0;

#ifdef QGC_UNITTEST_BUILD

    if(runUnitTests)
    {
        qInfo() << "[MAIN] Running Unit Tests";

        exitCode =
                runTests(
                    stressUnitTests,
                    unitTestOptions);
    }
    else

#endif
    {
        if(!simpleBootTest)
        {
            qInfo() << "[MAIN] Entering Qt Event Loop";

            exitCode = app.exec();

            qInfo() << "[MAIN] app.exec() returned"
                    << exitCode;
        }
        else
        {
            qWarning()
                    << "[MAIN] simpleBootTest enabled";
        }
    }

    qInfo() << "[MAIN] Calling shutdown";

    app.shutdown();

    qInfo() << "[MAIN] Shutdown complete";

    return exitCode;
}