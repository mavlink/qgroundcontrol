// Copyright (C) 2017 The Qt Company Ltd.
// Copyright (C) 2015 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QGLOBAL_P_H
#define QGLOBAL_P_H

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

#include "qglobal.h"
#include "qglobal_p.h"      // include self to avoid syncqt warning - no-op

#ifndef QT_BOOTSTRAPPED
#include <QtCore/private/qtcoreglobal_p.h>
#endif

#if defined(Q_CC_MSVC)
// By default, dynamic initialization uses subsection "$XCU", which is
// equivalent to #pragma init_seg(user). Additionally, #pragma
// init_seg(compiler) and init_seg(lib) use "$XCC" and "$XCL" respectively. So
// place us between "compiler" and "lib".
#  define QT_SUPPORTS_INIT_PRIORITY     1

// warning C4075: initializers put in unrecognized initialization area
#  define Q_DECL_INIT_PRIORITY(nn)      \
    __pragma(warning(disable: 4075)) \
    __pragma(init_seg(".CRT$XCK" QT_STRINGIFY(nn))) Q_DECL_UNUSED
#elif defined(Q_OS_QNX)
// init_priority fails on QNX and we didn't bother investigating why
#  define QT_SUPPORTS_INIT_PRIORITY     0
#elif defined(Q_OS_WIN) || defined(Q_OF_ELF)
#  define QT_SUPPORTS_INIT_PRIORITY     1
// priorities 0 to 1000 are reserved to the runtime;
// we use above 2000 in case someone REALLY needs to go before us
#  define Q_DECL_INIT_PRIORITY(nn)      __attribute__((init_priority(2000 + nn), used))
#elif defined(QT_SHARED)
// it doesn't support this exactly, but we can work around it
#  define QT_SUPPORTS_INIT_PRIORITY     -1
#  define Q_DECL_INIT_PRIORITY(nn)      Q_DECL_UNUSED
#else
#  define QT_SUPPORTS_INIT_PRIORITY     0
#endif

#if defined(__cplusplus)
QT_BEGIN_NAMESPACE

Q_NORETURN Q_CORE_EXPORT void qAbort();

QT_END_NAMESPACE

#if !__has_builtin(__builtin_available)
#include <initializer_list>
#include <QtCore/qoperatingsystemversion.h>
#include <QtCore/qversionnumber.h>

QT_BEGIN_NAMESPACE

struct qt_clang_builtin_available_os_version_data {
    QOperatingSystemVersion::OSType type;
    const char *version;
};

static inline bool qt_clang_builtin_available(
    const std::initializer_list<qt_clang_builtin_available_os_version_data> &versions)
{
    for (auto it = versions.begin(); it != versions.end(); ++it) {
        if (QOperatingSystemVersion::currentType() == it->type) {
            const auto current = QOperatingSystemVersion::current();
            return QVersionNumber(
                current.majorVersion(),
                current.minorVersion(),
                current.microVersion()) >= QVersionNumber::fromString(
                    QString::fromLatin1(it->version));
        }
    }

    // Result is true if the platform is not any of the checked ones; this matches behavior of
    // LLVM __builtin_available and @available constructs
    return true;
}

QT_END_NAMESPACE

#define QT_AVAILABLE_OS_VER(os, ver) \
    QT_PREPEND_NAMESPACE(qt_clang_builtin_available_os_version_data){\
        QT_PREPEND_NAMESPACE(QOperatingSystemVersion)::os, #ver}
#define QT_AVAILABLE_CAT(L, R) QT_AVAILABLE_CAT_(L, R)
#define QT_AVAILABLE_CAT_(L, R) L ## R
#define QT_AVAILABLE_EXPAND(...) QT_AVAILABLE_OS_VER(__VA_ARGS__)
#define QT_AVAILABLE_SPLIT(os_ver) QT_AVAILABLE_EXPAND(QT_AVAILABLE_CAT(QT_AVAILABLE_SPLIT_, os_ver))
#define QT_AVAILABLE_SPLIT_macOS MacOS,
#define QT_AVAILABLE_SPLIT_iOS IOS,
#define QT_AVAILABLE_SPLIT_tvOS TvOS,
#define QT_AVAILABLE_SPLIT_watchOS WatchOS,
#define QT_BUILTIN_AVAILABLE0(e) \
    QT_PREPEND_NAMESPACE(qt_clang_builtin_available)({})
#define QT_BUILTIN_AVAILABLE1(a, e) \
    QT_PREPEND_NAMESPACE(qt_clang_builtin_available)({QT_AVAILABLE_SPLIT(a)})
#define QT_BUILTIN_AVAILABLE2(a, b, e) \
    QT_PREPEND_NAMESPACE(qt_clang_builtin_available)({QT_AVAILABLE_SPLIT(a), \
                                                      QT_AVAILABLE_SPLIT(b)})
#define QT_BUILTIN_AVAILABLE3(a, b, c, e) \
    QT_PREPEND_NAMESPACE(qt_clang_builtin_available)({QT_AVAILABLE_SPLIT(a), \
                                                      QT_AVAILABLE_SPLIT(b), \
                                                      QT_AVAILABLE_SPLIT(c)})
#define QT_BUILTIN_AVAILABLE4(a, b, c, d, e) \
    QT_PREPEND_NAMESPACE(qt_clang_builtin_available)({QT_AVAILABLE_SPLIT(a), \
                                                      QT_AVAILABLE_SPLIT(b), \
                                                      QT_AVAILABLE_SPLIT(c), \
                                                      QT_AVAILABLE_SPLIT(d)})
#define QT_BUILTIN_AVAILABLE_ARG(arg0, arg1, arg2, arg3, arg4, arg5, ...) arg5
#define QT_BUILTIN_AVAILABLE_CHOOSER(...) QT_BUILTIN_AVAILABLE_ARG(__VA_ARGS__, \
    QT_BUILTIN_AVAILABLE4, \
    QT_BUILTIN_AVAILABLE3, \
    QT_BUILTIN_AVAILABLE2, \
    QT_BUILTIN_AVAILABLE1, \
    QT_BUILTIN_AVAILABLE0, )
#define __builtin_available(...) QT_BUILTIN_AVAILABLE_CHOOSER(__VA_ARGS__)(__VA_ARGS__)
#endif // !__has_builtin(__builtin_available)
#endif // defined(__cplusplus)

#endif // QGLOBAL_P_H
