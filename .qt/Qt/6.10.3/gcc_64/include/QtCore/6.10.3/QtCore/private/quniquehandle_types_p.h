// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QUNIQUEHANDLE_TYPES_P_H
#define QUNIQUEHANDLE_TYPES_P_H

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

#include <QtCore/qnamespace.h>
#include <QtCore/private/quniquehandle_p.h>

#include <cstdio>

QT_BEGIN_NAMESPACE

namespace QtUniqueHandleTraits {

#ifdef Q_OS_WIN

struct InvalidHandleTraits
{
    using Type = Qt::HANDLE;
    static Type invalidValue() noexcept
    {
        return Qt::HANDLE(-1); // AKA INVALID_HANDLE_VALUE
    }
    Q_CORE_EXPORT static bool close(Type handle) noexcept;
};

struct NullHandleTraits
{
    using Type = Qt::HANDLE;
    static Type invalidValue() noexcept { return nullptr; }
    Q_CORE_EXPORT static bool close(Type handle) noexcept;
};

#endif

struct FileDescriptorHandleTraits
{
    using Type = int;
    static constexpr Type invalidValue() noexcept { return -1; }
    Q_CORE_EXPORT static bool close(Type handle);
};

struct FILEHandleTraits
{
    using Type = FILE *;
    static constexpr Type invalidValue() noexcept { return nullptr; }
    Q_CORE_EXPORT static bool close(Type handle);
};

} // namespace QtUniqueHandleTraits

#ifdef Q_OS_WIN

using QUniqueWin32Handle = QUniqueHandle<QtUniqueHandleTraits::InvalidHandleTraits>;
using QUniqueWin32NullHandle = QUniqueHandle<QtUniqueHandleTraits::NullHandleTraits>;

#endif

#ifdef Q_OS_UNIX

using QUniqueFileDescriptorHandle = QUniqueHandle<QtUniqueHandleTraits::FileDescriptorHandleTraits>;

#endif

using QUniqueFILEHandle = QUniqueHandle<QtUniqueHandleTraits::FILEHandleTraits>;

QT_END_NAMESPACE

#endif
