// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QT_EGL_P_H
#define QT_EGL_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

// q(data/text)stream.h must be included before any header file that defines Status
#include <QtCore/qdatastream.h>
#include <QtCore/qtextstream.h>
#include <QtCore/private/qglobal_p.h>

#ifdef QT_EGL_NO_X11
# ifndef EGL_NO_X11
#  define EGL_NO_X11
# endif
# ifndef MESA_EGL_NO_X11_HEADERS
#  define MESA_EGL_NO_X11_HEADERS // MESA
# endif
# if !defined(Q_OS_INTEGRITY)
#  define WIN_INTERFACE_CUSTOM   // NV
# endif // Q_OS_INTEGRITY
#else // QT_EGL_NO_X11
// If one has an eglplatform.h with https://github.com/KhronosGroup/EGL-Registry/pull/130
// that needs USE_X11 to be defined.
# define USE_X11
#endif

#ifdef QT_EGL_WAYLAND
# define WAYLAND // NV
#endif // QT_EGL_WAYLAND

#include <EGL/egl.h>
#include <EGL/eglext.h>

#ifndef QT_EGL_NO_X11
#undef Status
#undef None
#undef Bool
#undef CursorShape
#undef KeyPress
#undef KeyRelease
#undef FocusIn
#undef FocusOut
#undef FontChange
#undef Expose
#undef Unsorted
#endif

#include <stdint.h>

QT_BEGIN_NAMESPACE

namespace QtInternal {

template <class FromType, class ToType>
struct QtEglConverter
{
    static inline ToType convert(FromType v)
    { return v; }
};

template <>
struct QtEglConverter<uint32_t, uintptr_t>
{
    static inline uintptr_t convert(uint32_t v)
    { return v; }
};

#if QT_POINTER_SIZE > 4
template <>
struct QtEglConverter<uintptr_t, uint32_t>
{
    static inline uint32_t convert(uintptr_t v)
    { return uint32_t(v); }
};
#endif

template <>
struct QtEglConverter<uint32_t, void *>
{
    static inline void *convert(uint32_t v)
    { return reinterpret_cast<void *>(uintptr_t(v)); }
};

template <>
struct QtEglConverter<void *, uint32_t>
{
    static inline uint32_t convert(void *v)
    { return uintptr_t(v); }
};

} // QtInternal

template <class ToType, class FromType>
static inline ToType qt_egl_cast(FromType from)
{ return QtInternal::QtEglConverter<FromType, ToType>::convert(from); }

QT_END_NAMESPACE

#endif // QT_EGL_P_H
