/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "Platform.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QProcessEnvironment>

#include "QGCCommandLineParser.h"

#ifdef Q_OS_ANDROID
    #include "AndroidInterface.h"
#endif

#if !defined(Q_OS_IOS) && !defined(Q_OS_ANDROID)
    #include "SignalHandler.h"
#endif

#if defined(Q_OS_MACOS)
    #include <CoreFoundation/CoreFoundation.h>
#elif defined(Q_OS_WIN)
    #include <qt_windows.h>
    #include <iostream>
    #include <iterator>  // std::size
    #include <cwchar>    // swprintf
    #if defined(_MSC_VER)
        #include <DbgHelp.h>
        #include <crtdbg.h>
        #include <stdlib.h>
        #include <cstdio> // _snwprintf_s
    #endif
#endif

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

#if defined(Q_OS_WIN)

#if defined(_MSC_VER)
bool g_quietWindowsAsserts = false;

bool GetWindowsCrtAssertLogPath(char* path, DWORD pathLen)
{
    const char logFileName[] = "qgc-crt-assert-stack.log";
    const DWORD length = GetModuleFileNameA(nullptr, path, pathLen);
    if ((length == 0) || (length >= pathLen)) {
        return false;
    }

    DWORD insertPos = length;
    while ((insertPos > 0) && (path[insertPos - 1] != '\\') && (path[insertPos - 1] != '/')) {
        --insertPos;
    }
    path[insertPos] = '\0';

    const DWORD logFileNameLength = static_cast<DWORD>(std::size(logFileName) - 1);
    if ((insertPos + logFileNameLength) >= pathLen) {
        return false;
    }

    for (DWORD i = 0; i <= logFileNameLength; ++i) {
        path[insertPos + i] = logFileName[i];
    }
    return true;
}

void AppendWindowsDiagnosticLogLine(const char* line)
{
    char logPath[MAX_PATH] = {};
    if (!GetWindowsCrtAssertLogPath(logPath, static_cast<DWORD>(std::size(logPath)))) {
        return;
    }

    HANDLE file = CreateFileA(logPath, FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (!file || (file == INVALID_HANDLE_VALUE)) {
        return;
    }

    DWORD ignored = 0;
    (void) WriteFile(file, line, static_cast<DWORD>(lstrlenA(line)), &ignored, nullptr);
    (void) WriteFile(file, "\r\n", 2, &ignored, nullptr);
    (void) CloseHandle(file);
}

void WriteWindowsDiagnosticLine(const char* line)
{
    if (!line) {
        return;
    }

    std::cerr << line << std::endl;
    (void) OutputDebugStringA(line);
    (void) OutputDebugStringA("\n");
    AppendWindowsDiagnosticLogLine(line);
}

void DumpWindowsStackTrace()
{
    HANDLE process = GetCurrentProcess();
    static bool symbolsInitialized = false;
    if (!symbolsInitialized) {
        (void) SymSetOptions(SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES | SYMOPT_UNDNAME);
        symbolsInitialized = SymInitialize(process, nullptr, TRUE) == TRUE;
    }

    void* frames[64] = {};
    const USHORT frameCount = CaptureStackBackTrace(0, static_cast<DWORD>(std::size(frames)), frames, nullptr);
    WriteWindowsDiagnosticLine("QGC: CRT assert stack (most recent call first)");

    char symbolStorage[sizeof(SYMBOL_INFO) + MAX_SYM_NAME] = {};
    auto* symbol = reinterpret_cast<SYMBOL_INFO*>(symbolStorage);
    symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
    symbol->MaxNameLen = MAX_SYM_NAME;

    for (USHORT i = 0; i < frameCount; ++i) {
        const auto address = reinterpret_cast<DWORD64>(frames[i]);
        DWORD64 symbolDisplacement = 0;
        DWORD lineDisplacement = 0;
        IMAGEHLP_LINE64 line = {};
        line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);

        char buffer[2048] = {};
        const bool haveSymbol = symbolsInitialized && SymFromAddr(process, address, &symbolDisplacement, symbol);
        const bool haveLine = symbolsInitialized && SymGetLineFromAddr64(process, address, &lineDisplacement, &line);
        if (haveSymbol && haveLine) {
            (void) std::snprintf(buffer, std::size(buffer),
                                 "QGC:   #%02hu 0x%p %s+0x%llX %s:%lu",
                                 i,
                                 frames[i],
                                 symbol->Name,
                                 static_cast<unsigned long long>(symbolDisplacement),
                                 line.FileName,
                                 static_cast<unsigned long>(line.LineNumber));
        } else if (haveSymbol) {
            (void) std::snprintf(buffer, std::size(buffer),
                                 "QGC:   #%02hu 0x%p %s+0x%llX",
                                 i,
                                 frames[i],
                                 symbol->Name,
                                 static_cast<unsigned long long>(symbolDisplacement));
        } else {
            (void) std::snprintf(buffer, std::size(buffer),
                                 "QGC:   #%02hu 0x%p",
                                 i,
                                 frames[i]);
        }
        WriteWindowsDiagnosticLine(buffer);
    }
}

int __cdecl WindowsCrtReportHook(int reportType, char* message, int* returnValue)
{
    if (message) {
        WriteWindowsDiagnosticLine(message);
    }
    if (reportType == _CRT_ASSERT) {
        DumpWindowsStackTrace();
        if (g_quietWindowsAsserts) {
            if (returnValue) {
                *returnValue = 0;
            }
            return 1; // handled
        }
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
    g_quietWindowsAsserts = quietWindowsAsserts;
    (void) _CrtSetReportHook2(_CRT_RPTHOOK_INSTALL, WindowsCrtReportHook);

    if (quietWindowsAsserts) {
        (void) _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_DEBUG);
        (void) _CrtSetReportMode(_CRT_ERROR,  _CRTDBG_MODE_DEBUG);
        (void) _CrtSetReportMode(_CRT_WARN,   _CRTDBG_MODE_DEBUG);
        (void) _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
        (void) _set_error_mode(_OUT_TO_STDERR);
    }
#else
    Q_UNUSED(quietWindowsAsserts);
#endif
}
#endif // Q_OS_WIN

} // namespace

void Platform::setupPreApp(const QGCCommandLineParser::CommandLineParseResult &cli)
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
    QCoreApplication::setAttribute(Qt::AA_CompressTabletEvents);
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
