// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#if 0
#pragma qt_class(QtStdLibDetection)
#pragma qt_sync_skip_header_check
#pragma qt_sync_stop_processing
#endif

#ifndef QSTDLIBDETECTION_H
#define QSTDLIBDETECTION_H

#include <QtCore/qtconfiginclude.h>

#ifdef __cplusplus

/* If <version> exists, qtconfiginclude.h will have included it. */
/* If not, we need to include _something_, and <utility> is included by qcompilerdetection.h, too */
#if !__has_include(<version>)
#  include <utility>
#endif

/*
   The std lib, must be one of: (Q_STL_x)

     LIBCPP       - libc++ (shipped with Clang, e.g.)
     LIBSTDCPP    - libstdc++ (shipped with GCC, e.g.)
     MSSTL        - Microsoft STL
     DINKUMWARE   - Dinkumware (shipped with QNX, VxWorks, Integrity, origin of MSSTL)
     STLPORT      - STLport (merged with SGI)
     SGI          - The original STL
     ROGUEWAVE    - RogueWave ((used to be) popular on ARM?)

   Not included:
     EASTL        - EASTL (this is not a drop-in STL, e.g. it doesn't have <vector>-style headers)

   Should be sorted most to least authoritative.
*/

#if defined(_LIBCPP_VERSION) /* libc++ */
#  define Q_STL_LIBCPP
#elif defined(_GLIBCXX_RELEASE) /* libstdc++ */
#  define Q_STL_LIBSTDCPP
#elif defined(_MSVC_STL_VERSION) /* MSSTL (must be before Dinkumware) */
#  define Q_STL_MSSTL
#elif defined(_YVALS) || defined(_CPPLIB_VER) /* Dinkumware */
#  define Q_STL_DINKUMWARE
#elif defined(_STLPORT_VERSION) /* STLport, cf. _stlport_version.h */
#  define Q_STL_STLPORT
#elif defined(__SGI_STL) /* must be after STLport, which mimics as SGI STL */
#  define Q_STL_SGI
#elif defined(_RWSTD_VER) /* RogueWave, at least as contributed to Apache stdcxx, cf. rw/_config.h */
#  define Q_STL_ROGUEWAVE
#else
#  error Unknown std library implementation, please file a report at bugreports.qt.io.
#endif

#endif // __cplusplus

#endif // QSTDLIBDETECTION_H
