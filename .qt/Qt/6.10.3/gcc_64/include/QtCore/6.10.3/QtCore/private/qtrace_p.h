// Copyright (C) 2017 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Rafael Roquetto <rafael.roquetto@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QTRACE_P_H
#define QTRACE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

/*
 * The Qt tracepoints API consists of only five macros:
 *
 *     - Q_TRACE(tracepoint, args...)
 *       Fires 'tracepoint' if it is enabled.
 *
 *     - Q_TRACE_EXIT(tracepoint, args...)
 *       Fires 'tracepoint' if it is enabled when the current scope exists.
 *
 *     - Q_TRACE_SCOPE(tracepoint, args...)
 *       Wrapper around Q_TRACE/_EXIT to trace entry and exit. First it traces
 *       `${tracepoint}_entry` and then `${tracepoint}_exit` on scope exit.
 *
 *     - Q_UNCONDITIONAL_TRACE(tracepoint, args...)
 *       Fires 'tracepoint' unconditionally: no check is performed to query
 *       whether 'tracepoint' is enabled.
 *
 *     - Q_TRACE_ENABLED(tracepoint)
 *       Returns 'true' if 'tracepoint' is enabled; false otherwise.
 *
 * When using LTTNG, Q_TRACE, Q_UNCONDITIONAL_TRACE and Q_TRACE_ENABLED map
 * ultimately to tracepoint(), do_tracepoint() and tracepoint_enabled(),
 * respectively, described on the lttng-ust manpage (man 3 lttng-ust).
 *
 * On ETW, Q_TRACE() and Q_UNCONDITIONAL_TRACE() are equivalent, ultimately
 * amounting to a call to TraceLoggingWrite(), whereas Q_TRACE_ENABLED()
 * wraps around TraceLoggingProviderEnabled().
 *
 * A tracepoint provider is defined in a separate file, that follows the
 * following format:
 *
 *     tracepoint_name(arg_type arg_name, ...)
 *
 * For instance:
 *
 *     qcoreapplication_ctor(int argc, const char * const argv)
 *     qcoreapplication_foo(int argc, const char[10] argv)
 *     qcoreapplication_baz(const char[len] some_string, unsigned int len)
 *     qcoreapplication_qstring(const QString &foo)
 *     qcoreapplication_qrect(const QRect &rect)
 *
 * The provider file is then parsed by src/tools/tracegen, which can be
 * switched to output either ETW, CTF or LTTNG tracepoint definitions. The provider
 * name is deduced to be basename(provider_file).
 *
 * To use the above (inside qtcore), you need to include
 * <providername_tracepoints_p.h>. After that, the following call becomes
 * possible:
 *
 *     Q_TRACE(qcoreapplication_qrect, myRect);
 *
 * Currently, all C++ primitive non-pointer types are supported for
 * arguments. Additionally, char * is supported, and is assumed to
 * be a NULL-terminated string. Finally, the following subset of Qt types also
 * currently supported:
 *
 *      - QString
 *      - QByteArray
 *      - QUrl
 *      - QRect
 *      - QRectF
 *      - QSize
 *      - QSizeF
 *
 * Dynamic arrays are supported using the syntax illustrated by
 * qcoreapplication_baz above.
 *
 * One can also add prefix for the generated providername_tracepoints_p.h file
 * by specifying it inside brackets '{ }' in the tracepoints file. One can
 * for example add forward declaration for a type:
 *
 * {
 *    QT_BEGIN_NAMESPACE
 *    class QEvent;
 *    QT_END_NAMESPACE
 * }
 *
 *  Metadata
 *
 * Metadata is used to add textual information for different types such
 * as enums and flags. How this data is handled depends on the used backend.
 * For ETW, the values are converted to text, for CTF and LTTNG they are used to add
 * CTF enumerations, which are converted to text after tracing.
 *
 * Enumererations are specified using ENUM:
 *
 * ENUM {
 *    Enum0 = 0,
 *    Enum1 = 1,
 *    Enum2,
 *    RANGE(RangeEnum, 3 ... 10),
 * } Name;
 *
 * Name must match to one of the enumerations used in the tracepoints. Range of values
 * can be provided using RANGE(name, first ... last). All values must be unique.
 *
 * Flags are specified using FLAGS:
 *
 * FLAGS {
 *    Default = 0,
 *    Flag0 = 1,
 *    Flag1 = 2,
 *    Flag2 = 4,
 * } Name;
 *
 * Name must match to one of the flags used in the tracepoints. Each value must be
 * power of two and unique.
 */

#include <QtCore/private/qglobal_p.h>
#include <QtCore/qscopeguard.h>

QT_BEGIN_NAMESPACE

#if defined(Q_TRACEPOINT) && !defined(QT_BOOTSTRAPPED)
#  define Q_HAS_TRACEPOINTS 1
#  define Q_TRACE(x, ...) QtPrivate::trace_ ## x(__VA_ARGS__)
#  define Q_TRACE_EXIT(x, ...) \
    const auto qTraceExit_ ## x ## __COUNTER__ = qScopeGuard([&]() { Q_TRACE(x, __VA_ARGS__); });
#  define Q_TRACE_SCOPE(x, ...) \
    Q_TRACE(x ## _entry, __VA_ARGS__); \
    Q_TRACE_EXIT(x ## _exit);
#  define Q_UNCONDITIONAL_TRACE(x, ...) QtPrivate::do_trace_ ## x(__VA_ARGS__)
#  define Q_TRACE_ENABLED(x) QtPrivate::trace_ ## x ## _enabled()
#else
#  define Q_HAS_TRACEPOINTS 0
#  define Q_TRACE(x, ...)
#  define Q_TRACE_EXIT(x, ...)
#  define Q_TRACE_SCOPE(x, ...)
#  define Q_UNCONDITIONAL_TRACE(x, ...)
#  define Q_TRACE_ENABLED(x) false
#endif // defined(Q_TRACEPOINT) && !defined(QT_BOOTSTRAPPED)


/*
 * The Qt tracepoints can also be defined directly in the source files using
 * the following macros. If using these macros, the tracepoints file is automatically
 * generated using the tracepointgen tool. The tool scans the input files for
 * these macros. These macros are ignored during compile time. Both automatic
 * generation and manually specifying tracepoints in a file can't be done at the same
 * time for the same provider.
 *
 *     - Q_TRACE_INSTRUMENT(provider)
 *       Generate entry/exit tracepoints for a function. For example, member function
 *
 *       void SomeClass::method(int param1, float param2)
 *       {
 *          ...
 *       }
 *
 *       converted to use tracepoints:
 *
 *       void Q_TRACE_INSTRUMENT(provider) SomeClass::method(int param1, float param2)
 *       {
 *           Q_TRACE_SCOPE(SomeClass_method, param1, param2);
 *           ...
 *       }
 *
 *       generates following tracepoints in provider.tracepoints file:
 *
 *       SomeClass_method_entry(int param1, float param2)
 *       SomeClass_method_exit()
 *
 *     - Q_TRACE_PARAM_REPLACE(in, out)
 *       Can be used with Q_TRACE_INSTRUMENT to replace parameter type in with type out.
 *       If a parameter type is not supported by the tracegen tool, one can use this to
 *       change it to another supported type.
 *
 *       void Q_TRACE_INSTRUMENT(provider) SomeClass::method(int param1, UserType param2)
 *       {
 *           Q_TRACE_PARAM_REPLACE(UserType, QString);
 *           Q_TRACE_SCOPE(SomeClass_method, param1, param2.toQString());
 *       }
 *
 *     - Q_TRACE_POINT(provider, tracepoint, ...)
 *       Manually specify tracepoint for the provider. 'tracepoint' is the full name
 *       of the tracepoint and ... can be zero or more parameters.
 *
 *       Q_TRACE_POINT(provider, SomeClass_function_entry, int param1, int param2);
 *
 *       generates following tracepoint:
 *
 *       SomeClass_function_entry(int param1, int param2)
 *
 *     - Q_TRACE_PREFIX(provider, prefix)
 *       Provide prefix for the tracepoint. Multiple prefixes can be specified for the same
 *       provider in different files, they are all concatenated into one in the
 *       provider.tracepoints file.
 *
 *       Q_TRACE_PREFIX(provider,
 *                      "QT_BEGIN_NAMESPACE" \
 *                      "class QEvent;"      \
 *                      "QT_END_NAMESPACE")
 *
 *     - Q_TRACE_METADATA(provider, metadata)
 *       Provides metadata for the tracepoint provider.
 *
 *       Q_TRACE_METADATA(qtgui,
 *                       "ENUM {" \
 *                       "Format_Invalid," \
 *                       "Format_Mono," \
 *                       "Format_MonoLSB," \
 *                       "Format_Indexed8," \
 *                        ...
 *                       "} QImage::Format;" \
 *                       );
 *
 *       If the content of enum is empty or contains keyword AUTO, then the tracepointgen tool
 *       tries to find the enumeration from header files.
 *
 *       Q_TRACE_METADATA(qtcore, "ENUM { AUTO, RANGE User ... MaxUser } QEvent::Type;");
 */
#define Q_TRACE_INSTRUMENT(provider)
#define Q_TRACE_PARAM_REPLACE(in, out)
#define Q_TRACE_POINT(provider, tracepoint, ...)
#define Q_TRACE_PREFIX(provider, prefix)
#define Q_TRACE_METADATA(provider, metadata)

QT_END_NAMESPACE

#endif // QTRACE_P_H
