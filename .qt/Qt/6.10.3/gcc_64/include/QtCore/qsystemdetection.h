// Copyright (C) 2019 The Qt Company Ltd.
// Copyright (C) 2019 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#if 0
#pragma qt_class(QtSystemDetection)
#pragma qt_sync_skip_header_check
#pragma qt_sync_stop_processing
#endif

#ifndef QSYSTEMDETECTION_H
#define QSYSTEMDETECTION_H

/*
   The operating system, must be one of: (Q_OS_x)

     DARWIN   - Any Darwin system (macOS, iOS, watchOS, tvOS)
     MACOS    - macOS
     IOS      - iOS
     WATCHOS  - watchOS
     TVOS     - tvOS
     VISIONOS - visionOS
     WIN32    - Win32 (Windows 2000/XP/Vista/7 and Windows Server 2003/2008)
     CYGWIN   - Cygwin
     SOLARIS  - Sun Solaris
     HPUX     - HP-UX
     LINUX    - Linux [has variants]
     FREEBSD  - FreeBSD [has variants]
     NETBSD   - NetBSD
     OPENBSD  - OpenBSD
     INTERIX  - Interix
     AIX      - AIX
     HURD     - GNU Hurd
     QNX      - QNX [has variants]
     QNX6     - QNX RTP 6.1
     LYNX     - LynxOS
     BSD4     - Any BSD 4.4 system
     UNIX     - Any UNIX BSD/SYSV system
     ANDROID  - Android platform
     HAIKU    - Haiku
     WEBOS    - LG WebOS
     WASM     - WebAssembly

   The following operating systems have variants:
     LINUX    - both Q_OS_LINUX and Q_OS_ANDROID are defined when building for Android
              - only Q_OS_LINUX is defined if building for other Linux systems
     MACOS    - both Q_OS_BSD4 and Q_OS_IOS are defined when building for iOS
              - both Q_OS_BSD4 and Q_OS_MACOS are defined when building for macOS
     FREEBSD  - Q_OS_FREEBSD is defined only when building for FreeBSD with a BSD userland
              - Q_OS_FREEBSD_KERNEL is always defined on FreeBSD, even if the userland is from GNU
*/

#if defined(__APPLE__) && (defined(__GNUC__) || defined(__xlC__) || defined(__xlc__))
#  include <TargetConditionals.h>
#  define Q_OS_APPLE
#  if defined(TARGET_OS_MAC) && TARGET_OS_MAC
#    define Q_OS_DARWIN
#    define Q_OS_BSD4
#    if defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE
#      define QT_PLATFORM_UIKIT
#      if defined(TARGET_OS_WATCH) && TARGET_OS_WATCH
#        define Q_OS_WATCHOS
#      elif defined(TARGET_OS_TV) && TARGET_OS_TV
#        define Q_OS_TVOS
#      elif defined(TARGET_OS_VISION) && TARGET_OS_VISION
#        define Q_OS_VISIONOS
#      else
#        // TARGET_OS_IOS is only available in newer SDKs,
#        // so assume any other iOS-based platform is iOS for now
#        define Q_OS_IOS
#      endif
#    else
#      // TARGET_OS_OSX is only available in newer SDKs,
#      // so assume any non iOS-based platform is macOS for now
#      define Q_OS_MACOS
#    endif
#  else
#    error "Qt has not been ported to this Apple platform - see http://www.qt.io/developers"
#  endif
#elif defined(__WEBOS__)
#  define Q_OS_WEBOS
#  define Q_OS_LINUX
#elif defined(__ANDROID__) || defined(ANDROID)
#  define Q_OS_ANDROID
#  define Q_OS_LINUX
#elif defined(__CYGWIN__)
#  define Q_OS_CYGWIN
#elif !defined(SAG_COM) && (!defined(WINAPI_FAMILY) || WINAPI_FAMILY==WINAPI_FAMILY_DESKTOP_APP) && (defined(WIN64) || defined(_WIN64) || defined(__WIN64__))
#  define Q_OS_WIN32
#  define Q_OS_WIN64
#elif !defined(SAG_COM) && (defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__))
#    define Q_OS_WIN32
#elif defined(__sun) || defined(sun)
#  define Q_OS_SOLARIS
#elif defined(hpux) || defined(__hpux)
#  define Q_OS_HPUX
#elif defined(__EMSCRIPTEN__)
#  define Q_OS_WASM
#elif defined(__linux__) || defined(__linux)
#  define Q_OS_LINUX
#elif defined(__FreeBSD__) || defined(__DragonFly__) || defined(__FreeBSD_kernel__)
#  ifndef __FreeBSD_kernel__
#    define Q_OS_FREEBSD
#  endif
#  define Q_OS_FREEBSD_KERNEL
#  define Q_OS_BSD4
#elif defined(__NetBSD__)
#  define Q_OS_NETBSD
#  define Q_OS_BSD4
#elif defined(__OpenBSD__)
#  define Q_OS_OPENBSD
#  define Q_OS_BSD4
#elif defined(__INTERIX)
#  define Q_OS_INTERIX
#  define Q_OS_BSD4
#elif defined(_AIX)
#  define Q_OS_AIX
#elif defined(__Lynx__)
#  define Q_OS_LYNX
#elif defined(__GNU__)
#  define Q_OS_HURD
#elif defined(__QNXNTO__)
#  define Q_OS_QNX
#elif defined(__INTEGRITY)
#  define Q_OS_INTEGRITY
#elif defined(__rtems__)
#  define Q_OS_RTEMS
#elif defined(__vxworks)
#  define Q_OS_VXWORKS
#elif defined(__HAIKU__)
#  define Q_OS_HAIKU
#elif defined(__MAKEDEPEND__)
#else
#  error "Qt has not been ported to this OS - see http://www.qt-project.org/"
#endif

#if defined(Q_OS_WIN32) || defined(Q_OS_WIN64)
#  define Q_OS_WINDOWS
#  define Q_OS_WIN
// On Windows, pointers to dllimport'ed variables are not constant expressions,
// so to keep to certain initializations (like QMetaObject) constexpr, we need
// to use functions instead.
#  define QT_NO_DATA_RELOCATION
#endif

#if defined(Q_OS_WIN)
#  undef Q_OS_UNIX
#elif !defined(Q_OS_UNIX)
#  define Q_OS_UNIX
#endif

// Compatibility synonyms
#ifdef Q_OS_DARWIN
#  pragma clang diagnostic push
#  pragma clang diagnostic ignored "-Wunknown-pragmas"
#  define Q_OS_MAC // FIXME: Deprecate
#  ifdef __LP64__
#    define Q_OS_DARWIN64
#    pragma clang deprecated(Q_OS_DARWIN64, "use Q_OS_DARWIN and QT_POINTER_SIZE/Q_PROCESSOR_* instead")
#    define Q_OS_MAC64
#    pragma clang deprecated(Q_OS_MAC64, "use Q_OS_DARWIN and QT_POINTER_SIZE/Q_PROCESSOR_* instead")
#  else
#    define Q_OS_DARWIN32
#    pragma clang deprecated(Q_OS_DARWIN32, "use Q_OS_DARWIN and QT_POINTER_SIZE/Q_PROCESSOR_* instead")
#    define Q_OS_MAC32
#    pragma clang deprecated(Q_OS_MAC32, "use Q_OS_DARWIN and QT_POINTER_SIZE/Q_PROCESSOR_* instead")
#  endif
#  ifdef Q_OS_MACOS
#    define Q_OS_MACX
#    pragma clang deprecated(Q_OS_MACX, "use Q_OS_MACOS instead")
#    define Q_OS_OSX
#    pragma clang deprecated(Q_OS_OSX, "use Q_OS_MACOS instead")
#  endif
#  pragma clang diagnostic pop
#endif

#ifdef Q_OS_DARWIN
#  include <Availability.h>
#  include <AvailabilityMacros.h>

#  define QT_DARWIN_PLATFORM_SDK_EQUAL_OR_ABOVE(macos, ios, tvos, watchos) \
    ((defined(__MAC_OS_X_VERSION_MAX_ALLOWED) && macos != __MAC_NA && __MAC_OS_X_VERSION_MAX_ALLOWED >= macos) || \
     (defined(__IPHONE_OS_VERSION_MAX_ALLOWED) && ios != __IPHONE_NA && __IPHONE_OS_VERSION_MAX_ALLOWED >= ios) || \
     (defined(__TV_OS_VERSION_MAX_ALLOWED) && tvos != __TVOS_NA && __TV_OS_VERSION_MAX_ALLOWED >= tvos) || \
     (defined(__WATCH_OS_VERSION_MAX_ALLOWED) && watchos != __WATCHOS_NA && __WATCH_OS_VERSION_MAX_ALLOWED >= watchos))

#  define QT_DARWIN_DEPLOYMENT_TARGET_BELOW(macos, ios, tvos, watchos) \
    ((defined(__MAC_OS_X_VERSION_MIN_REQUIRED) && macos != __MAC_NA && __MAC_OS_X_VERSION_MIN_REQUIRED < macos) || \
     (defined(__IPHONE_OS_VERSION_MIN_REQUIRED) && ios != __IPHONE_NA && __IPHONE_OS_VERSION_MIN_REQUIRED < ios) || \
     (defined(__TV_OS_VERSION_MIN_REQUIRED) && tvos != __TVOS_NA && __TV_OS_VERSION_MIN_REQUIRED < tvos) || \
     (defined(__WATCH_OS_VERSION_MIN_REQUIRED) && watchos != __WATCHOS_NA && __WATCH_OS_VERSION_MIN_REQUIRED < watchos))

#  define QT_MACOS_IOS_PLATFORM_SDK_EQUAL_OR_ABOVE(macos, ios) \
      QT_DARWIN_PLATFORM_SDK_EQUAL_OR_ABOVE(macos, ios, __TVOS_NA, __WATCHOS_NA)
#  define QT_MACOS_PLATFORM_SDK_EQUAL_OR_ABOVE(macos) \
      QT_DARWIN_PLATFORM_SDK_EQUAL_OR_ABOVE(macos, __IPHONE_NA, __TVOS_NA, __WATCHOS_NA)
#  define QT_IOS_PLATFORM_SDK_EQUAL_OR_ABOVE(ios) \
      QT_DARWIN_PLATFORM_SDK_EQUAL_OR_ABOVE(__MAC_NA, ios, __TVOS_NA, __WATCHOS_NA)
#  define QT_TVOS_PLATFORM_SDK_EQUAL_OR_ABOVE(tvos) \
      QT_DARWIN_PLATFORM_SDK_EQUAL_OR_ABOVE(__MAC_NA, __IPHONE_NA, tvos, __WATCHOS_NA)
#  define QT_WATCHOS_PLATFORM_SDK_EQUAL_OR_ABOVE(watchos) \
      QT_DARWIN_PLATFORM_SDK_EQUAL_OR_ABOVE(__MAC_NA, __IPHONE_NA, __TVOS_NA, watchos)

#  define QT_MACOS_IOS_DEPLOYMENT_TARGET_BELOW(macos, ios) \
      QT_DARWIN_DEPLOYMENT_TARGET_BELOW(macos, ios, __TVOS_NA, __WATCHOS_NA)
#  define QT_MACOS_DEPLOYMENT_TARGET_BELOW(macos) \
      QT_DARWIN_DEPLOYMENT_TARGET_BELOW(macos, __IPHONE_NA, __TVOS_NA, __WATCHOS_NA)
#  define QT_IOS_DEPLOYMENT_TARGET_BELOW(ios) \
      QT_DARWIN_DEPLOYMENT_TARGET_BELOW(__MAC_NA, ios, __TVOS_NA, __WATCHOS_NA)
#  define QT_TVOS_DEPLOYMENT_TARGET_BELOW(tvos) \
      QT_DARWIN_DEPLOYMENT_TARGET_BELOW(__MAC_NA, __IPHONE_NA, tvos, __WATCHOS_NA)
#  define QT_WATCHOS_DEPLOYMENT_TARGET_BELOW(watchos) \
      QT_DARWIN_DEPLOYMENT_TARGET_BELOW(__MAC_NA, __IPHONE_NA, __TVOS_NA, watchos)

#else // !Q_OS_DARWIN

#define QT_DARWIN_PLATFORM_SDK_EQUAL_OR_ABOVE(macos, ios, tvos, watchos) (0)
#define QT_MACOS_IOS_PLATFORM_SDK_EQUAL_OR_ABOVE(macos, ios) (0)
#define QT_MACOS_PLATFORM_SDK_EQUAL_OR_ABOVE(macos) (0)
#define QT_IOS_PLATFORM_SDK_EQUAL_OR_ABOVE(ios) (0)
#define QT_TVOS_PLATFORM_SDK_EQUAL_OR_ABOVE(tvos) (0)
#define QT_WATCHOS_PLATFORM_SDK_EQUAL_OR_ABOVE(watchos) (0)

#endif // Q_OS_DARWIN

#ifdef __LSB_VERSION__
#  if __LSB_VERSION__ < 40
#    error "This version of the Linux Standard Base is unsupported"
#  endif
#ifndef QT_LINUXBASE
#  define QT_LINUXBASE
#endif
#endif

#if defined (__ELF__)
#  define Q_OF_ELF
#endif
#if defined (__MACH__) && defined (__APPLE__)
#  define Q_OF_MACH_O
#endif

#if defined(__SIZEOF_INT128__)
// Compiler used in VxWorks SDK declares __SIZEOF_INT128__ but VxWorks doesn't support this type,
// so we can't rely solely on compiler here.
// MSVC STL used by MSVC and clang-cl does not support int128
#if !defined(Q_OS_VXWORKS) && !defined(_MSC_VER)
#  define QT_COMPILER_SUPPORTS_INT128 __SIZEOF_INT128__
#endif
#endif // defined(__SIZEOF_INT128__)

#endif // QSYSTEMDETECTION_H
