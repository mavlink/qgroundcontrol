#include "Platform.h"
#include "qgc_version.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QProcessEnvironment>
#include <QtQuick/QQuickWindow>
#include <QtQuick/QSGRendererInterface>

#include <cstdio>

#include "QGCCommandLineParser.h"

#if !defined(Q_OS_IOS) && !defined(Q_OS_ANDROID)
    #include "RunGuard.h"
    #include "SignalHandler.h"
#endif

#if (defined(Q_OS_LINUX) || defined(Q_OS_FREEBSD)) && !defined(Q_OS_ANDROID)
    #include <unistd.h>
    #include <sys/types.h>
    #include <sys/wait.h>
#endif

#if defined(Q_OS_MACOS)
    #include <CoreFoundation/CoreFoundation.h>
#elif defined(Q_OS_WIN)
    #include <qt_windows.h>
    #include <iostream>
    #if defined(_MSC_VER)
        #include <crtdbg.h>
        #include <stdlib.h>
    #endif
#endif

#if defined(Q_OS_WIN)
// dbghelp.h needs the Windows types from qt_windows.h above.
#include <cstdarg>
#include <cwchar>
#include <dbghelp.h>
#endif

namespace {

#if (defined(Q_OS_LINUX) || defined(Q_OS_FREEBSD)) && !defined(Q_OS_ANDROID)
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

#if defined(_DEBUG)
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
#endif // _DEBUG

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

using MiniDumpWriteDumpFn = BOOL(WINAPI*)(HANDLE, DWORD, HANDLE, MINIDUMP_TYPE, PMINIDUMP_EXCEPTION_INFORMATION,
                                          PMINIDUMP_USER_STREAM_INFORMATION, PMINIDUMP_CALLBACK_INFORMATION);
MiniDumpWriteDumpFn g_miniDumpWriteDump = nullptr;
wchar_t g_crashDumpPath[MAX_PATH] = {};
wchar_t g_crashDumpPartPath[MAX_PATH] = {};

// The crashed thread may hold the loader or heap lock, or have almost no stack left after an
// overflow, so a thread started with QGC writes the minidump while the filter waits for it.
constexpr DWORD CRASH_DUMP_WAIT_MS = 20000;
bool g_crashDumpReady = false;
HANDLE g_crashDumpRequested = nullptr;
HANDLE g_crashDumpWritten = nullptr;
EXCEPTION_POINTERS* g_crashException = nullptr;
DWORD g_crashThreadId = 0;
volatile LONG g_crashHandled = 0;

// Static so the filter itself needs little stack.
wchar_t g_crashModulePath[MAX_PATH] = {};
wchar_t g_crashMessage[2 * MAX_PATH] = {};
char g_crashNarrow[6 * MAX_PATH] = {};

template <std::size_t Size>
bool formatWide(wchar_t (&buffer)[Size], const wchar_t* format, ...)
{
    va_list args;
    va_start(args, format);
#if defined(_MSC_VER)
    const int length = _vsnwprintf_s(buffer, Size, _TRUNCATE, format, args);
#else
    const int length = std::vswprintf(buffer, Size, format, args);
    buffer[Size - 1] = L'\0';
#endif
    va_end(args);
    return length >= 0;
}

template <std::size_t Size>
void writeCrashMessage(const wchar_t* message, char (&narrow)[Size])
{
    (void) OutputDebugStringW(message);

    const HANDLE h = GetStdHandle(STD_ERROR_HANDLE);
    if (!h || (h == INVALID_HANDLE_VALUE)) {
        return;
    }
    const int length = WideCharToMultiByte(CP_UTF8, 0, message, -1, narrow, static_cast<int>(Size), nullptr, nullptr);
    if (length > 1) {
        DWORD ignored = 0;
        (void) WriteFile(h, narrow, static_cast<DWORD>(length - 1), &ignored, nullptr);
    }
}

// Runs on the dump thread, whose own buffer keeps it apart from the filter's messages.
void writeCrashDump()
{
    char narrow[6 * MAX_PATH] = {};
    const HANDLE file =
        CreateFileW(g_crashDumpPartPath, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        writeCrashMessage(L"QGC: no crash dump, cannot create a file in QGC_CRASH_DUMP_DIR\n", narrow);
        return;
    }

    MINIDUMP_EXCEPTION_INFORMATION exceptionInfo = {};
    exceptionInfo.ThreadId = g_crashThreadId;
    exceptionInfo.ExceptionPointers = g_crashException;
    exceptionInfo.ClientPointers = FALSE;
    const BOOL written =
        g_miniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), file,
                            static_cast<MINIDUMP_TYPE>(MiniDumpWithUnloadedModules | MiniDumpWithThreadInfo),
                            g_crashException ? &exceptionInfo : nullptr, nullptr, nullptr);
    DWORD error = written ? ERROR_SUCCESS : GetLastError();
    (void) CloseHandle(file);

    // Only a complete dump gets the .dmp name that CI collects.
    bool kept = false;
    if (written) {
        kept = MoveFileExW(g_crashDumpPartPath, g_crashDumpPath, MOVEFILE_REPLACE_EXISTING) != FALSE;
        if (!kept) {
            error = GetLastError();
        }
    }

    wchar_t message[2 * MAX_PATH] = {};
    if (kept) {
        (void) formatWide(message, L"QGC: wrote crash dump %ls\n", g_crashDumpPath);
    } else {
        (void) DeleteFileW(g_crashDumpPartPath);
        (void) formatWide(message, L"QGC: crash dump failed with error 0x%08lX\n", static_cast<unsigned long>(error));
    }
    writeCrashMessage(message, narrow);
}

DWORD WINAPI crashDumpThread(LPVOID)
{
    if (WaitForSingleObject(g_crashDumpRequested, INFINITE) == WAIT_OBJECT_0) {
        writeCrashDump();
        (void) SetEvent(g_crashDumpWritten);
    }
    return 0;
}

// Resolved before any crash: the exception filter must not load libraries or start threads.
void setupCrashDumps()
{
    wchar_t directory[MAX_PATH] = {};
    const DWORD length = GetEnvironmentVariableW(L"QGC_CRASH_DUMP_DIR", directory, MAX_PATH);
    if ((length == 0) || (length >= MAX_PATH)) {
        return;
    }
    if (!formatWide(g_crashDumpPath, L"%ls\\QGC-crash-%lu.dmp", directory,
                    static_cast<unsigned long>(GetCurrentProcessId())) ||
        !formatWide(g_crashDumpPartPath, L"%ls.part", g_crashDumpPath)) {
        g_crashDumpPath[0] = L'\0';
        return;
    }

    const HMODULE dbghelp = LoadLibraryExW(L"dbghelp.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (dbghelp) {
        g_miniDumpWriteDump = reinterpret_cast<MiniDumpWriteDumpFn>(
            reinterpret_cast<void (*)()>(GetProcAddress(dbghelp, "MiniDumpWriteDump")));
    }
    if (!g_miniDumpWriteDump) {
        return;
    }

    g_crashDumpRequested = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    g_crashDumpWritten = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (!g_crashDumpRequested || !g_crashDumpWritten) {
        return;
    }
    const HANDLE thread = CreateThread(nullptr, 256 * 1024, crashDumpThread, nullptr, 0, nullptr);
    if (!thread) {
        return;
    }
    (void) CloseHandle(thread);
    g_crashDumpReady = true;
}

LONG WINAPI WindowsUnhandledExceptionFilter(EXCEPTION_POINTERS* ep)
{
    // A second crashing thread lets the first finish its report, then ends the process.
    if (InterlockedExchange(&g_crashHandled, 1) != 0) {
        if (g_crashDumpReady) {
            (void) WaitForSingleObject(g_crashDumpWritten, CRASH_DUMP_WAIT_MS);
        }
        return EXCEPTION_EXECUTE_HANDLER;
    }

    const EXCEPTION_RECORD* const record = ep ? ep->ExceptionRecord : nullptr;
    const DWORD code = record ? record->ExceptionCode : 0;
    const ULONG_PTR address = record ? reinterpret_cast<ULONG_PTR>(record->ExceptionAddress) : 0;

    const wchar_t* moduleName = L"<unknown module>";
    ULONG_PTR offset = address;
    HMODULE module = nullptr;
    if ((address != 0) &&
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                           reinterpret_cast<LPCWSTR>(address), &module) &&
        (GetModuleFileNameW(module, g_crashModulePath, MAX_PATH) > 0)) {
        moduleName = g_crashModulePath;
        offset = address - reinterpret_cast<ULONG_PTR>(module);
    }

    (void) formatWide(g_crashMessage, L"QGC: unhandled SEH 0x%08lX at 0x%llX in %ls+0x%llX\n",
                      static_cast<unsigned long>(code), static_cast<unsigned long long>(address), moduleName,
                      static_cast<unsigned long long>(offset));
    writeCrashMessage(g_crashMessage, g_crashNarrow);

    if (g_crashDumpReady) {
        g_crashException = ep;
        g_crashThreadId = GetCurrentThreadId();
        if (SetEvent(g_crashDumpRequested) &&
            (WaitForSingleObject(g_crashDumpWritten, CRASH_DUMP_WAIT_MS) == WAIT_TIMEOUT)) {
            writeCrashMessage(L"QGC: crash dump unfinished after 20 s; exiting without it\n", g_crashNarrow);
        }
    } else if (g_crashDumpPath[0] != L'\0') {
        writeCrashMessage(L"QGC: no crash dump, the dump writer could not start\n", g_crashNarrow);
    }

    return EXCEPTION_EXECUTE_HANDLER;
}

void setWindowsErrorModes(bool quietWindowsAsserts)
{
    (void) SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX);
    setupCrashDumps();
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
#if (defined(Q_OS_LINUX) || defined(Q_OS_FREEBSD)) && !defined(Q_OS_ANDROID)
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
    if (qEnvironmentVariable("QSG_RHI_BACKEND").compare(QLatin1String("d3d12"), Qt::CaseInsensitive) == 0) {
        // Qt 6.10 does not reliably select D3D12 from QSG_RHI_BACKEND on Windows. Make the test/diagnostic override
        // explicit before the scene graph is initialized; the default path remains Qt's D3D11 backend.
        QQuickWindow::setGraphicsApi(QSGRendererInterface::Direct3D12);
    }
    setWindowsErrorModes(args.quietWindowsAsserts);
#endif

#ifdef Q_OS_MACOS
    disableAppNapViaInfoDict();
#endif

#ifdef QGC_UNITTEST_BUILD
    if ((args.runningUnitTests || args.listTests) && !args.onscreen) {
        if (!qEnvironmentVariableIsSet("QT_QPA_PLATFORM")) {
            (void) qputenv("QT_QPA_PLATFORM", "offscreen");
        }
    }
#endif

    // --- Qt attributes ---
    if (args.useSwRast) {
#ifdef Q_OS_WIN
        // Qt 6 no longer ships opengl32sw.dll, so stay on D3D and pick the built-in WARP adapter instead.
        (void) qputenv("QSG_RHI_PREFER_SOFTWARE_RENDERER", "1");
#else
        // RHI defaults to Metal on macOS; AA_UseSoftwareOpenGL only bites once the scene graph is on GL.
        QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);
        QCoreApplication::setAttribute(Qt::AA_UseSoftwareOpenGL);
#endif
    }
#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID) && \
    (defined(QGC_HAS_GST_GLMEMORY_GPU_PATH) || defined(QGC_HAS_GST_DMABUF_GPU_PATH))
    // GL is the only working desktop-Linux GStreamer zero-copy backend (GLMemory and DMABuf/EGLImage both import into a
    // GL RHI; Vulkan import dormant); pin it unless the user set QSG_RHI_BACKEND. No QRhi::probe — needs GuiPrivate (not
    // linked here) and GL is always present on Linux.
    else if (!qEnvironmentVariableIsSet("QSG_RHI_BACKEND")) {
        QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);
    }
    // GStreamer's zero-copy imports need Qt on EGL, but Qt's xcb integration picks GLX first (the AppImage runs xcb
    // even in Wayland sessions). Qt falls back to GLX when EGL fails to initialize; the user's value wins.
    if (!qEnvironmentVariableIsSet("QT_XCB_GL_INTEGRATION")) {
        (void) qputenv("QT_XCB_GL_INTEGRATION", "xcb_egl");
    }
#endif

    // GStreamer's GL/DMABuf zero-copy paths both need QOpenGLContext::globalShareContext(), which this attribute enables.
#if defined(QGC_HAS_GST_GLMEMORY_GPU_PATH) || defined(QGC_HAS_GST_DMABUF_GPU_PATH)
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
#endif
    QCoreApplication::setAttribute(Qt::AA_CompressTabletEvents);

    return std::nullopt;
}

void Platform::setupPostApp()
{
#if !defined(Q_OS_IOS) && !defined(Q_OS_ANDROID)
    SignalHandler* signalHandler = new SignalHandler(QCoreApplication::instance());
    (void) signalHandler->setupSignalHandlers();
#endif
}

#if (defined(Q_OS_LINUX) || defined(Q_OS_FREEBSD)) && !defined(Q_OS_ANDROID)
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
    // The native alert is GUI-only; also write to stderr so a CLI/headless launch sees the reason.
    fprintf(stderr, "Error: %s\n", message.toLocal8Bit().constData());
    CFStringRef cfMessage = CFStringCreateWithCString(nullptr, message.toUtf8().constData(), kCFStringEncodingUTF8);
    CFUserNotificationDisplayAlert(0, kCFUserNotificationStopAlertLevel,
                                   nullptr, nullptr, nullptr,
                                   CFSTR("Error"), cfMessage,
                                   nullptr, nullptr, nullptr, nullptr);
    CFRelease(cfMessage);
#elif defined(Q_OS_WIN)
    // MessageBoxW is GUI-only; also write to stderr so a CLI/headless launch sees the reason.
    fprintf(stderr, "Error: %s\n", message.toLocal8Bit().constData());
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
