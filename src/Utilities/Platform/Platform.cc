#include "Platform.h"
#include "qgc_version.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QProcessEnvironment>

#include "QGCCommandLineParser.h"

#ifdef Q_OS_ANDROID
    #include "AndroidInterface.h"
#endif

#if !defined(Q_OS_IOS) && !defined(Q_OS_ANDROID)
    #include "RunGuard.h"
    #include "SignalHandler.h"
#endif

#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
    #include <cstdio>
    #include <unistd.h>
    #include <sys/types.h>
    #include <sys/wait.h>
#endif

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

namespace {

#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
static void showLinuxErrorDialog(const QByteArray& msg)
{
    // Try to show a GUI dialog — important for AppImage users where stderr is invisible.
    // Fork a child and attempt dialog tools in order of preference; no shell is invoked.
    const pid_t pid = fork();
    if (pid == 0) {
        const QByteArray zenityText = QByteArrayLiteral("--text=") + msg;
        execlp("zenity", "zenity", "--error", "--title=Error", zenityText.constData(), nullptr);
        execlp("kdialog", "kdialog", "--error", msg.constData(), nullptr);
        execlp("xmessage", "xmessage", "-center", msg.constData(), nullptr);
        _exit(1);
    } else if (pid > 0) {
        int status = 0;
        (void) waitpid(pid, &status, 0);
    }
    // Always write to stderr as well
    fprintf(stderr, "Error: %s\n", msg.constData());
}
#endif // Q_OS_LINUX

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

std::optional<int> Platform::initialize(int argc, char* argv[],
                                         const QGCCommandLineParser::CommandLineParseResult& args)
{
    // --- Safety checks (may cause early exit) ---
#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
    if (isRunningAsRoot()) {
        return showRootError(argc, argv);
    }
#endif

#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
    const bool allowMultiple = args.allowMultiple || args.runningUnitTests || args.listTests;
    if (!checkSingleInstance(allowMultiple)) {
        return showMultipleInstanceError(argc, argv);
    }
#else
    Q_UNUSED(argc);
    Q_UNUSED(argv);
#endif

    // --- Environment setup ---
#ifdef Q_OS_UNIX
#ifndef Q_OS_ANDROID
    // On Android, skip these — either env var triggers shouldLogToStderr(),
    // which bypasses Qt's __android_log_print path to logcat.
    if (!qEnvironmentVariableIsSet("QT_ASSUME_STDERR_HAS_CONSOLE")) {
        (void) qputenv("QT_ASSUME_STDERR_HAS_CONSOLE", "1");
    }
    if (!qEnvironmentVariableIsSet("QT_FORCE_STDERR_LOGGING")) {
        (void) qputenv("QT_FORCE_STDERR_LOGGING", "1");
    }
#endif
#endif

#ifdef Q_OS_WIN
    if (!qEnvironmentVariableIsSet("QT_WIN_DEBUG_CONSOLE")) {
        (void) qputenv("QT_WIN_DEBUG_CONSOLE", "attach");
    }
    setWindowsErrorModes(args.quietWindowsAsserts);
#endif

#ifdef Q_OS_MACOS
    disableAppNapViaInfoDict();
#endif

    // --- Unit test mode: run headless ---
#ifdef QGC_UNITTEST_BUILD
    if (args.runningUnitTests || args.listTests) {
        if (!qEnvironmentVariableIsSet("QT_QPA_PLATFORM")) {
            (void) qputenv("QT_QPA_PLATFORM", "offscreen");
        }
    }
#endif

    // --- Qt attributes ---
    if (args.useDesktopGL) {
        QCoreApplication::setAttribute(Qt::AA_UseDesktopOpenGL);
    }

    if (args.useSwRast) {
        QCoreApplication::setAttribute(Qt::AA_UseSoftwareOpenGL);
    }

    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
    QCoreApplication::setAttribute(Qt::AA_CompressTabletEvents);

    return std::nullopt;
}

void Platform::setupPostApp()
{
#if !defined(Q_OS_IOS) && !defined(Q_OS_ANDROID)
    SignalHandler* signalHandler = new SignalHandler(QCoreApplication::instance());
    (void) signalHandler->setupSignalHandlers();
#endif

#ifdef Q_OS_ANDROID
    AndroidInterface::checkStoragePermissions();
#endif
}

#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
bool Platform::isRunningAsRoot()
{
    return ::getuid() == 0;
}

int Platform::showRootError([[maybe_unused]] int argc, [[maybe_unused]] char *argv[])
{
    const QString message = QCoreApplication::translate("main",
        "You are running %1 as root. "
        "You should not do this since it will cause other issues with %1. "
        "%1 will now exit.").arg(QLatin1String(QGC_APP_NAME));
    showLinuxErrorDialog(message.toLocal8Bit());
    return -1;
}
#endif

#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
int Platform::showMultipleInstanceError([[maybe_unused]] int argc, [[maybe_unused]] char *argv[])
{
    const QString message = QCoreApplication::translate("main",
        "A second instance of %1 is already running. "
        "Please close the other instance and try again.").arg(QLatin1String(QGC_APP_NAME));
#if defined(Q_OS_MACOS)
    CFStringRef cfMessage = CFStringCreateWithCString(nullptr, message.toUtf8().constData(), kCFStringEncodingUTF8);
    CFUserNotificationDisplayAlert(0, kCFUserNotificationStopAlertLevel,
                                   nullptr, nullptr, nullptr,
                                   CFSTR("Error"), cfMessage,
                                   nullptr, nullptr, nullptr, nullptr);
    CFRelease(cfMessage);
#elif defined(Q_OS_WIN)
    MessageBoxW(nullptr, message.toStdWString().c_str(), L"Error", MB_OK | MB_ICONERROR);
#else
    showLinuxErrorDialog(message.toLocal8Bit());
#endif
    return -1;
}

bool Platform::checkSingleInstance(bool allowMultiple)
{
    if (allowMultiple) {
        return true;
    }

    static const QString runguardString = QStringLiteral("%1 RunGuardKey").arg(QLatin1String(QGC_APP_NAME));
    static RunGuard guard(runguardString);
    return guard.tryToRun();
}
#endif
