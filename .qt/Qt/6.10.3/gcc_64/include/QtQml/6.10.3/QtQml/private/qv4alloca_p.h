// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QV4_ALLOCA_H
#define QV4_ALLOCA_H

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

#include <QtCore/private/qglobal_p.h>

#include <stdlib.h>
#if __has_include(<alloca.h>)
#  include <alloca.h>
#endif
#if __has_include(<malloc.h>)
#  include <malloc.h>
#endif

#ifdef Q_CC_MSVC
// This does not matter unless compiling in strict standard mode.
#  define alloca _alloca
#endif

// Define Q_ALLOCA_VAR macro to be used instead of #ifdeffing
// the occurrences of alloca() in case it's not supported.
// Q_ALLOCA_DECLARE and Q_ALLOCA_ASSIGN macros separate
// memory allocation from the declaration and RAII.
#define Q_ALLOCA_VAR(type, name, size) \
    Q_ALLOCA_DECLARE(type, name); \
    Q_ALLOCA_ASSIGN(type, name, size)

#ifdef alloca

#define Q_ALLOCA_DECLARE(type, name) \
    type *name = 0

#define Q_ALLOCA_ASSIGN(type, name, size) \
    name = static_cast<type*>(alloca(size))

#else
#  include <memory>

#define Q_ALLOCA_DECLARE(type, name) \
    std::unique_ptr<char[]> _qt_alloca_##name; \
    type *name = nullptr

#define Q_ALLOCA_ASSIGN(type, name, size) \
    do { \
        _qt_alloca_##name.reset(new char[size]); \
        name = reinterpret_cast<type*>(_qt_alloca_##name.get()); \
    } while (false)

#endif

#endif
