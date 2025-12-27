/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "Platform.h"

#if defined(Q_OS_MACOS)
    #include <CoreFoundation/CoreFoundation.h>
#elif defined(Q_OS_WIN)
    #include <qt_windows.h>
    #include <iostream>
    #include <iterator>  // std::size
    #include <cwchar>    // swprintf
    #if defined(_MSC_VER)
        #include <crtdbg.h>
        #include <stdlib.h>
        #include <cstdio> // _snwprintf_s
    #endif
#endif

#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
    #include <sys/types.h>
    #include <unistd.h>
#if !defined(QGC_NO_SERIAL_LINK)
    #include <grp.h>
    #include <pwd.h>
    #include <sys/stat.h>
#endif
#endif

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QProcessEnvironment>

#ifdef Q_OS_ANDROID
    #include "AndroidInterface.h"
#endif

#include "QGCCommandLineParser.h"
#include "QGCLoggingCategory.h"

#if !defined(Q_OS_IOS) && !defined(Q_OS_ANDROID)
    #include "SignalHandler.h"
#endif

QGC_LOGGING_CATEGORY(PlatformLog, "qgc.utilities.platform")

namespace {

#if defined(Q_OS_MACOS)
void disableAppNapViaInfoDict()
{
    CFBundleRef bundle = CFBundleGetMainBundle();
    if (!bundle) {
        return;
    }
    CFMutableDictionaryRef infoDict = const_cast<CFMutableDictionaryRef>(CFBundleGetInfoDictionary(bundle));
    if (infoDict) {
        CFDictionarySetValue(infoDict, CFSTR("NSAppSleepDisabled"), kCFBooleanTrue);
    }
}
#endif // Q_OS_MACOS

/*---------------------------------------------------------------------------*/

#if defined(Q_OS_WIN)

#if defined(_MSC_VER)
int __cdecl WindowsCrtReportHook(int reportType, char* message, int* returnValue)
{
    if (message) {
        std::cerr << message << std::endl;
    }
    if (reportType == _CRT_ASSERT) {
        if (returnValue) {
            *returnValue = 0;
        }
        return 1; // handled
    }
    return 0; // let CRT continue
}

void __cdecl WindowsPurecallHandler()
{
    (void) OutputDebugStringW(L"QGC: _purecall\n");
}

void WindowsInvalidParameterHandler([[maybe_unused]] const wchar_t* expression,
                                    [[maybe_unused]] const wchar_t* function,
                                    [[maybe_unused]] const wchar_t* file,
                                    [[maybe_unused]] unsigned int line,
                                    [[maybe_unused]] uintptr_t pReserved)
{

}
#endif // _MSC_VER

LPTOP_LEVEL_EXCEPTION_FILTER g_prevUef = nullptr;

LONG WINAPI WindowsUnhandledExceptionFilter(EXCEPTION_POINTERS* ep)
{
    const DWORD code = (ep && ep->ExceptionRecord) ? ep->ExceptionRecord->ExceptionCode : 0;
    wchar_t buf[128] = {};
#if defined(_MSC_VER)
    (void) _snwprintf_s(buf, _TRUNCATE, L"QGC: unhandled SEH 0x%08lX\n", static_cast<unsigned long>(code));
#else
    (void) swprintf(buf, static_cast<int>(std::size(buf)), L"QGC: unhandled SEH 0x%08lX\n", static_cast<unsigned long>(code));
#endif
    (void) OutputDebugStringW(buf);

    const HANDLE h = GetStdHandle(STD_ERROR_HANDLE);
    if (h && (h != INVALID_HANDLE_VALUE)) {
        DWORD ignored = 0;
        const char narrow[] = "QGC: unhandled SEH\n";
        (void) WriteFile(h, narrow, (DWORD)sizeof(narrow) - 1, &ignored, nullptr);
    }

    return EXCEPTION_EXECUTE_HANDLER;
}

void setWindowsErrorModes(bool quietWindowsAsserts)
{
    (void) SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX);
    g_prevUef = SetUnhandledExceptionFilter(WindowsUnhandledExceptionFilter);

#if defined(_MSC_VER)
    (void) _set_invalid_parameter_handler(WindowsInvalidParameterHandler);
    (void) _set_purecall_handler(WindowsPurecallHandler);

    if (quietWindowsAsserts) {
        (void) _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_DEBUG);
        (void) _CrtSetReportMode(_CRT_ERROR,  _CRTDBG_MODE_DEBUG);
        (void) _CrtSetReportMode(_CRT_WARN,   _CRTDBG_MODE_DEBUG);
        (void) _CrtSetReportHook2(_CRT_RPTHOOK_INSTALL, WindowsCrtReportHook);
        (void) _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
        (void) _set_error_mode(_OUT_TO_STDERR);
    }
#else
    Q_UNUSED(quietWindowsAsserts);
#endif
}
#endif // Q_OS_WIN

} // namespace

/*---------------------------------------------------------------------------*/

namespace Platform {

#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
bool isUserRoot()
{
    return (::getuid() == 0);
}

#if !defined(QGC_NO_SERIAL_LINK)
void checkSerialPortPermissions(QString &errorMessage)
{
    // Method 1: Check actual group membership using system calls
    bool hasDialoutAccess = false;
    bool hasUucp = false;
    bool hasTty = false;

    // Get the current user's effective UID
    const uid_t uid = getuid();

    // Get user info
    const struct passwd *pw = getpwuid(uid);
    if (!pw) {
        qCWarning(PlatformLog) << "Cannot determine current user";
        return;
    }

    const QString username = QString::fromLocal8Bit(pw->pw_name);
    const gid_t primary_gid = pw->pw_gid;

    // Get number of supplementary groups
    const int ngroups = getgroups(0, nullptr);
    if (ngroups > 0) {
        QList<gid_t> groups(ngroups);
        if (getgroups(ngroups, groups.data()) != -1) {
            // Add primary group to the list
            groups.append(primary_gid);

            // Check each group
            for (const gid_t gid : groups) {
                const struct group *grp = getgrgid(gid);
                if (grp) {
                    const QString groupName = QString::fromLocal8Bit(grp->gr_name);
                    if (groupName == QStringLiteral("dialout")) {
                        hasDialoutAccess = true;
                    } else if (groupName == QStringLiteral("uucp")) {
                        hasUucp = true;
                    } else if (groupName == QStringLiteral("tty")) {
                        hasTty = true;
                    }
                }
            }
        }
    }

    // Method 2: Also check by trying to access a serial port
    bool canAccessSerialDevices = false;
    const QDir devDir(QStringLiteral("/dev"));

    // Check common serial device patterns
    const QStringList serialPatterns = {
        QStringLiteral("ttyUSB*"),
        QStringLiteral("ttyACM*"),
        QStringLiteral("ttyS*"),
        QStringLiteral("rfcomm*")
    };

    for (const QString &pattern : serialPatterns) {
        const QStringList devices = devDir.entryList(QStringList() << pattern, QDir::System);
        for (const QString &device : devices) {
            const QString devicePath = QStringLiteral("/dev/") + device;
            const QFileInfo deviceInfo(devicePath);

            // Check if it's a character device (serial ports are character devices)
            if (!deviceInfo.exists()) {
                continue;
            }

            // On Linux, we can check if it's a character device by checking the file type
            struct stat st;
            if (stat(devicePath.toLocal8Bit().constData(), &st) == 0) {
                if (!S_ISCHR(st.st_mode)) {
                    continue;  // Not a character device
                }
            }

            // Safer and fast: test permission bits only
            if (deviceInfo.isReadable() && deviceInfo.isWritable()) {
                canAccessSerialDevices = true;
                break;
            } else {
                qCDebug(PlatformLog) << "Cannot access serial device:" << devicePath
                                     << "Permissions:" << deviceInfo.permissions()
                                     << "Owner:" << deviceInfo.owner()
                                     << "Group:" << deviceInfo.group();
            }
        }

        if (canAccessSerialDevices) {
            break;
        }
    }

    // Method 3: Check ModemManager interference
    bool modemManagerRunning = false;
    QProcess mmCheck;
    mmCheck.setProgram(QStringLiteral("/usr/bin/systemctl"));
    mmCheck.setArguments(QStringList() << QStringLiteral("is-active") << QStringLiteral("ModemManager"));
    mmCheck.start(QIODevice::ReadOnly);
    if (mmCheck.waitForStarted(500)) {
        if (mmCheck.waitForFinished(1000)) {
            if (mmCheck.exitStatus() == QProcess::NormalExit && mmCheck.exitCode() == 0) {
                const QString output = mmCheck.readAllStandardOutput().trimmed();
                modemManagerRunning = (output == QStringLiteral("active"));
            }
        } else {
            qCDebug(PlatformLog) << "systemctl timeout:" << mmCheck.errorString();
        }
    } else {
        // systemctl not available or failed to start - try fallback
        qCDebug(PlatformLog) << "systemctl not available, trying pgrep fallback";
        QProcess psCheck;
        psCheck.setProgram(QStringLiteral("/usr/bin/pgrep"));
        psCheck.setArguments(QStringList() << QStringLiteral("ModemManager"));
        psCheck.start(QIODevice::ReadOnly);
        if (psCheck.waitForStarted(500)) {
            if (psCheck.waitForFinished(1000)) {
                modemManagerRunning = !psCheck.readAllStandardOutput().isEmpty();
            }
        } else {
            qCDebug(PlatformLog) << "pgrep also not available or failed to start";
        }
    }

    // Build list of missing groups and check which groups exist on system
    QStringList neededGroups;
    bool hasDialoutGroup = false;
    bool hasUucpGroup = false;

    if (!hasDialoutAccess && !hasUucp && !hasTty) {
        // User has no serial access groups at all

        // Check which groups exist on the system
        QFile groupFile(QStringLiteral("/etc/group"));
        if (groupFile.open(QIODevice::ReadOnly)) {
            QTextStream stream(&groupFile);
            QString line;

            while (stream.readLineInto(&line)) {
                if (line.startsWith(QStringLiteral("dialout:"))) {
                    hasDialoutGroup = true;
                } else if (line.startsWith(QStringLiteral("uucp:"))) {
                    hasUucpGroup = true;
                }
            }
            groupFile.close();
        }

        // Prefer dialout if it exists, otherwise use uucp
        if (hasDialoutGroup) {
            neededGroups << QStringLiteral("dialout");
        } else if (hasUucpGroup) {
            neededGroups << QStringLiteral("uucp");
        } else {
            // Default to dialout if neither exists (shouldn't happen)
            neededGroups << QStringLiteral("dialout");
        }
    }

    // Determine if we need to show a warning
    QString warningMessage;

    if (!canAccessSerialDevices && !neededGroups.isEmpty()) {
        // Build distribution-specific instructions
        QString distroName = QStringLiteral("Linux");

        // Try to determine distribution
        QFile osRelease("/etc/os-release");
        if (osRelease.open(QIODevice::ReadOnly)) {
            QTextStream stream(&osRelease);
            QString line;
            while (stream.readLineInto(&line)) {
                if (line.startsWith("NAME=")) {
                    distroName = line.mid(5).remove('"').trimmed();
                    break;
                }
            }
            osRelease.close();
        }

        QString instructions;
        // Build instructions based on distribution
        if (distroName.contains(QStringLiteral("Ubuntu")) || distroName.contains(QStringLiteral("Debian"))) {
            instructions = QStringLiteral(
                "If you are using %1, execute the following commands to fix these issues:<br/>"
                "<pre>sudo usermod -a -G %2 $USER<br/>"
            ).arg(distroName, neededGroups.constFirst());

            if (modemManagerRunning) {
                instructions += QStringLiteral("sudo apt-get remove modemmanager<br/>");
            }

        } else if (distroName.contains(QStringLiteral("Fedora")) || distroName.contains(QStringLiteral("Red Hat")) || distroName.contains(QStringLiteral("CentOS"))) {
            instructions = QStringLiteral(
                "If you are using %1, execute the following commands to fix these issues:<br/>"
                "<pre>sudo usermod -a -G %2 $USER<br/>"
            ).arg(distroName, neededGroups.constFirst());

            if (modemManagerRunning) {
                instructions += QStringLiteral("sudo systemctl disable ModemManager<br/>"
                                              "sudo systemctl stop ModemManager<br/>");
            }

        } else if (distroName.contains(QStringLiteral("Arch")) || distroName.contains(QStringLiteral("Manjaro"))) {
            const QString groupName = hasUucpGroup ? QStringLiteral("uucp") : QStringLiteral("dialout");
            instructions = QStringLiteral(
                "If you are using %1, execute the following commands to fix these issues:<br/>"
                "<pre>sudo usermod -a -G %2 $USER<br/>"
            ).arg(distroName, groupName);

            if (modemManagerRunning) {
                instructions += QStringLiteral("sudo systemctl disable ModemManager<br/>"
                                              "sudo systemctl stop ModemManager<br/>");
            }

        } else {
            // Generic instructions
            instructions = QStringLiteral(
                "To fix these issues:<br/>"
                "<pre>sudo usermod -a -G %1 $USER<br/>")
                .arg(neededGroups.constFirst());

            if (modemManagerRunning) {
                instructions += QStringLiteral("# Disable ModemManager (command varies by distribution)<br/>");
            }
        }

        instructions += QStringLiteral("# Then log out and log back in for changes to take effect</pre>");

        warningMessage = QCoreApplication::translate("platform",
            "The current user (%1) does not have the correct permissions to access serial devices. "
            "You need to be a member of the '%2' group.<br/><br/>"
        ).arg(username, neededGroups.constFirst());

        if (modemManagerRunning) {
            warningMessage += QCoreApplication::translate("platform", "Additionally, ModemManager is running and may interfere with serial devices.<br/><br/>");
        }

        warningMessage += instructions;

    } else if (modemManagerRunning && !canAccessSerialDevices) {
        // User has permissions but ModemManager might be interfering
        warningMessage = QCoreApplication::translate("platform",
            "ModemManager is running and may interfere with serial device access. "
            "Consider disabling it if you experience connection issues.<br/><br/>"
            "To disable ModemManager:<br/>"
            "<pre>sudo systemctl disable ModemManager<br/>"
            "sudo systemctl stop ModemManager</pre>");
    }

    if (warningMessage.isEmpty() && !canAccessSerialDevices) {
        // User has group membership but still can't access devices
        // This might be because no devices are connected, which is fine
        qCDebug(PlatformLog) << "User" << username << "has serial group membership but no accessible devices found";
    }
}
#endif // !defined(QGC_NO_SERIAL_LINK)
#endif // defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)

/*---------------------------------------------------------------------------*/

void setupPreApp(const QGCCommandLineParser::CommandLineParseResult &cli)
{
#ifdef Q_OS_UNIX
    if (!qEnvironmentVariableIsSet("QT_ASSUME_STDERR_HAS_CONSOLE")) {
        (void) qputenv("QT_ASSUME_STDERR_HAS_CONSOLE", "1");
    }
    if (!qEnvironmentVariableIsSet("QT_FORCE_STDERR_LOGGING")) {
        (void) qputenv("QT_FORCE_STDERR_LOGGING", "1");
    }
#endif

#ifdef Q_OS_WIN
    // (void) qputenv("QT_OPENGL_BUGLIST", ":/opengl/resources/opengl/buglist.json");
    if (!qEnvironmentVariableIsSet("QT_WIN_DEBUG_CONSOLE")) {
        (void) qputenv("QT_WIN_DEBUG_CONSOLE", "attach");
    }
    setWindowsErrorModes(cli.quietWindowsAsserts);
#endif

#ifdef Q_OS_MACOS
    disableAppNapViaInfoDict();
#endif

    if (cli.useDesktopGL) {
        QCoreApplication::setAttribute(Qt::AA_UseDesktopOpenGL);
    }

    if (cli.useSwRast) {
        QCoreApplication::setAttribute(Qt::AA_UseSoftwareOpenGL);
    }

    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
    QCoreApplication::setAttribute(Qt::AA_CompressHighFrequencyEvents);
    QCoreApplication::setAttribute(Qt::AA_CompressTabletEvents);
}

void setupPostApp()
{
#if !defined(Q_OS_IOS) && !defined(Q_OS_ANDROID)
    SignalHandler* signalHandler = new SignalHandler(QCoreApplication::instance());
    (void) signalHandler->setupSignalHandlers();
#endif

#ifdef Q_OS_ANDROID
    AndroidInterface::checkStoragePermissions();
#endif
}

} // namespace Platform
