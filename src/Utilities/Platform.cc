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

#ifdef Q_OS_MAC

#include <CoreFoundation/CoreFoundation.h>

void Platform::disableAppNapViaInfoDict()
{
    CFBundleRef bundle = CFBundleGetMainBundle();
    if (!bundle) {
        return;
    }

    CFMutableDictionaryRef infoDict = (CFMutableDictionaryRef) CFBundleGetInfoDictionary(bundle);
    if (infoDict) {
        CFDictionarySetValue(infoDict, CFSTR("NSAppSleepDisabled"), kCFBooleanTrue);
    }
}

#elif defined(Q_OS_WIN)

#include <qt_windows.h>

#ifdef Q_CC_MSVC
#include <crtdbg.h>

static int __cdecl WindowsCrtReportHook(int reportType, char *message, int *returnValue)
{
    Q_UNUSED(message);

    // For asserts, suppress any interactive break/GUI. Keep process running.
    if (reportType == _CRT_ASSERT) {
        if (returnValue) {
            *returnValue = 0; // continue execution
        }
        return TRUE; // handled: no further CRT processing
    }

    // For warnings/errors, let CRT continue with our non-GUI report mode.
    return FALSE;
}

#endif

static void WindowsInvalidParameterHandler(const wchar_t *expression,
                                           const wchar_t *function,
                                           const wchar_t *file,
                                           unsigned int line,
                                           uintptr_t pReserved)
{
    Q_UNUSED(expression);
    Q_UNUSED(function);
    Q_UNUSED(file);
    Q_UNUSED(line);
    Q_UNUSED(pReserved);
    // Do nothing
}

static void WindowsSigIntHandler()
{
    qDebug() << "Ctrl-C received.";
    QCoreApplication::quit();
}

static BOOL WINAPI WindowsConsoleCtrlHandler(DWORD CEvent)
{
    switch (CEvent) {
    case CTRL_C_EVENT:
        WindowsSigIntHandler();
        return TRUE;
    default:
        break;
    }

    return FALSE;
}

void Platform::setWindowsNativeFunctions(bool quietWindowsAsserts)
{
    if (quietWindowsAsserts) {
        (void) _set_invalid_parameter_handler(WindowsInvalidParameterHandler);
        (void) SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX);
#ifdef Q_CC_MSVC
        (void) _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_DEBUG);
        (void) _CrtSetReportHook2(_CRT_RPTHOOK_INSTALL, WindowsCrtReportHook);
#endif
    }

    (void) SetConsoleCtrlHandler((PHANDLER_ROUTINE) WindowsConsoleCtrlHandler, TRUE);

    // TODO: Consider SetUnhandledExceptionFilter(windowsFaultHandler), _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT), etc.
    // (void) _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
    // (void) _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
    // (void) _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
    // (void) _CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDERR);
    // (void) _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);
    // (void) _CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDERR);
    // (void) _CrtSetReportHook2(_CRT_RPTHOOK_REMOVE, WindowsCrtReportHook);
}

#endif
