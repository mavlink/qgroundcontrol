// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:header-decls-only

#ifndef QTFORMAT_IMPL_H
#define QTFORMAT_IMPL_H

#if 0
#pragma qt_no_master_include
#pragma qt_sync_skip_header_check
#endif

#include <QtCore/qsystemdetection.h>
#include <QtCore/qtconfigmacros.h>

// Users can disable std::format support in their
// projects by using this definition.
#if !defined(QT_NO_STD_FORMAT_SUPPORT) && defined(__cpp_lib_format) && __cpp_lib_format >= 202106L

#include <format>

// If this macro is defined, std::format support is actually available.
// Use it to provide the implementation!
// Note that any out-of-line helper function should not depend on this
// definition, as it should be unconditionally available even in C++17 builds
// to keep BC.
#define QT_SUPPORTS_STD_FORMAT  1

#endif // __cpp_lib_format && !QT_NO_STD_FORMAT_SUPPORT

#endif // QTFORMAT_IMPL_H
