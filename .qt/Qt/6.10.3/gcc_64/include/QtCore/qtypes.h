// Copyright (C) 2022 The Qt Company Ltd.
// Copyright (C) 2022 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QTYPES_H
#define QTYPES_H

#include <QtCore/qprocessordetection.h>
#include <QtCore/qsystemdetection.h>
#include <QtCore/qtconfigmacros.h>
#include <QtCore/qassert.h>

#ifdef __cplusplus
#  include <cstddef>
#  if defined(_HAS_STD_BYTE) && _HAS_STD_BYTE == 0
#    error "Qt requires std::byte, but _HAS_STD_BYTE has been set to 0"
#  endif
#  include <cstdint>
#  if defined(__STDCPP_FLOAT16_T__) && __has_include(<stdfloat>)
// P1467 implementation - https://wg21.link/p1467
#    include <stdfloat>
#  endif // defined(__STDCPP_FLOAT16_T__) && __has_include(<stdfloat>)
#  include <type_traits>
#else
#  include <assert.h>
#endif

#if 0
#pragma qt_class(QtTypes)
#pragma qt_class(QIntegerForSize)
#pragma qt_sync_stop_processing
#endif

/*
   Useful type definitions for Qt
*/
typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long ulong;

QT_BEGIN_NAMESPACE

/*
   Size-dependent types (architecture-dependent byte order)

   Make sure to update QMetaType when changing these typedefs
*/

typedef signed char qint8;         /* 8 bit signed */
typedef unsigned char quint8;      /* 8 bit unsigned */
typedef short qint16;              /* 16 bit signed */
typedef unsigned short quint16;    /* 16 bit unsigned */
typedef int qint32;                /* 32 bit signed */
typedef unsigned int quint32;      /* 32 bit unsigned */
// Unlike LL / ULL in C++, for historical reasons, we force the
// result to be of the requested type.
#ifdef __cplusplus
#  define Q_INT64_C(c) static_cast<long long>(c ## LL)     /* signed 64 bit constant */
#  define Q_UINT64_C(c) static_cast<unsigned long long>(c ## ULL) /* unsigned 64 bit constant */
#else
#  define Q_INT64_C(c) ((long long)(c ## LL))               /* signed 64 bit constant */
#  define Q_UINT64_C(c) ((unsigned long long)(c ## ULL))    /* unsigned 64 bit constant */
#endif
typedef long long qint64;           /* 64 bit signed */
typedef unsigned long long quint64; /* 64 bit unsigned */

typedef qint64 qlonglong;
typedef quint64 qulonglong;

#ifdef Q_QDOC // QDoc always needs to see the typedefs
#  define QT_SUPPORTS_INT128 16
#elif defined(QT_COMPILER_SUPPORTS_INT128) && !defined(QT_NO_INT128)
#  define QT_SUPPORTS_INT128 QT_COMPILER_SUPPORTS_INT128
#  if defined(__GLIBCXX__) && defined(__STRICT_ANSI__) // -ansi/-std=c++NN instead of gnu++NN
#    undef QT_SUPPORTS_INT128                          // breaks <type_traits> on libstdc++
#  endif
#  if defined(__clang__) && defined(_MSVC_STL_VERSION) // Clang with MSVC's STL
#    undef QT_SUPPORTS_INT128                          // MSVC's STL doesn't support int128
#  endif
#else
#  undef QT_SUPPORTS_INT128
#endif

#if defined(QT_SUPPORTS_INT128)
__extension__ typedef __int128_t qint128;
__extension__ typedef __uint128_t quint128;

#ifdef __cplusplus
static_assert(std::is_signed_v<qint128>,
              "Qt requires <type_traits> and <limits> to work for q(u)int128.");
#endif

// limits:
#  ifdef __cplusplus /* need to avoid c-style-casts in C++ mode */
#    define QT_C_STYLE_CAST(type, x) static_cast<type>(x)
#  else /* but C doesn't have constructor-style casts */
#    define QT_C_STYLE_CAST(type, x) ((type)(x))
#  endif
#  ifndef Q_UINT128_MAX /* allow qcompilerdetection.h/user override */
#    define Q_UINT128_MAX QT_C_STYLE_CAST(quint128, -1)
#  endif
#  define Q_INT128_MAX QT_C_STYLE_CAST(qint128, Q_UINT128_MAX / 2)
#  define Q_INT128_MIN (-Q_INT128_MAX - 1)

#  ifdef __cplusplus
    namespace QtPrivate::NumberLiterals {
    namespace detail {
        template <quint128 accu, int base>
        constexpr quint128 construct() { return accu; }

        template <quint128 accu, int base, char C, char...Cs>
        constexpr quint128 construct()
        {
            if constexpr (C != '\'') { // ignore digit separators
                const int digitValue = '0' <= C && C <= '9' ? C - '0'      :
                                       'a' <= C && C <= 'z' ? C - 'a' + 10 :
                                       'A' <= C && C <= 'Z' ? C - 'A' + 10 :
                                       /* else */        -1 ;
                static_assert(digitValue >= 0 && digitValue < base,
                              "Invalid character");
                // accu * base + digitValue <= MAX, but without overflow:
                static_assert(accu <= (Q_UINT128_MAX - digitValue) / base,
                              "Overflow occurred");
                return construct<accu * base + digitValue, base, Cs...>();
            } else {
                return construct<accu, base, Cs...>();
            }
        }

        template <char C, char...Cs>
        constexpr quint128 parse0xb()
        {
            constexpr quint128 accu = 0;
            if constexpr (C == 'x' || C == 'X')
                return construct<accu, 16,   Cs...>(); // base 16, skip 'x'
            else if constexpr (C == 'b' || C == 'B')
                return construct<accu, 2,    Cs...>(); // base 2, skip 'b'
            else
                return construct<accu, 8, C, Cs...>(); // base 8, include C
        }

        template <char...Cs>
        constexpr quint128 parse0()
        {
            if constexpr (sizeof...(Cs) == 0) // this was just a literal 0
                return 0;
            else
                return parse0xb<Cs...>();
        }

        template <char C, char...Cs>
        constexpr quint128 parse()
        {
            if constexpr (C == '0')
                return parse0<Cs...>(); // base 2, 8, or 16 (or just a literal 0), skip '0'
            else
                return construct<0, 10, C, Cs...>(); // initial accu 0, base 10, include C
        }
    } // namespace detail
    template <char...Cs>
    constexpr quint128 operator""_quint128() noexcept
    { return QtPrivate::NumberLiterals::detail::parse<Cs...>(); }
    template <char...Cs>
    constexpr qint128 operator""_qint128() noexcept
    { return qint128(QtPrivate::NumberLiterals::detail::parse<Cs...>()); }

    #ifndef Q_UINT128_C // allow qcompilerdetection.h/user override
    #  define Q_UINT128_C(c) ([]{ using namespace QtPrivate::NumberLiterals; return c ## _quint128; }())
    #endif
    #ifndef Q_INT128_C // allow qcompilerdetection.h/user override
    #  define Q_INT128_C(c)  ([]{ using namespace QtPrivate::NumberLiterals; return c ## _qint128;  }())
    #endif

    } // namespace QtPrivate::NumberLiterals
#  endif // __cplusplus
#endif // QT_SUPPORTS_INT128

#ifndef __cplusplus
// In C++ mode, we define below using QIntegerForSize template
static_assert(sizeof(ptrdiff_t) == sizeof(size_t), "Weird ptrdiff_t and size_t definitions");
typedef ptrdiff_t qptrdiff;
typedef ptrdiff_t qsizetype;
typedef ptrdiff_t qintptr;
typedef size_t quintptr;

#define PRIdQPTRDIFF "td"
#define PRIiQPTRDIFF "ti"

#define PRIdQSIZETYPE "td"
#define PRIiQSIZETYPE "ti"

#define PRIdQINTPTR "td"
#define PRIiQINTPTR "ti"

#define PRIuQUINTPTR "zu"
#define PRIoQUINTPTR "zo"
#define PRIxQUINTPTR "zx"
#define PRIXQUINTPTR "zX"
#endif

#if defined(QT_COORD_TYPE)
typedef QT_COORD_TYPE qreal;
#else
typedef double qreal;
#endif

#if defined(__cplusplus)
/*
  quintptr and qptrdiff are guaranteed to be the same size as a pointer, i.e.

      sizeof(void *) == sizeof(quintptr)
      && sizeof(void *) == sizeof(qptrdiff)

  While size_t and qsizetype are not guaranteed to be the same size as a pointer,
  they usually are and we do check for that in qtypes.cpp, just to be sure.
*/
template <int> struct QIntegerForSize;
template <>    struct QIntegerForSize<1> { typedef quint8  Unsigned; typedef qint8  Signed; };
template <>    struct QIntegerForSize<2> { typedef quint16 Unsigned; typedef qint16 Signed; };
template <>    struct QIntegerForSize<4> { typedef quint32 Unsigned; typedef qint32 Signed; };
template <>    struct QIntegerForSize<8> { typedef quint64 Unsigned; typedef qint64 Signed; };
#if defined(QT_SUPPORTS_INT128)
template <>    struct QIntegerForSize<16> { typedef quint128 Unsigned; typedef qint128 Signed; };
#endif
template <class T> struct QIntegerForSizeof: QIntegerForSize<sizeof(T)> { };
typedef QIntegerForSize<Q_PROCESSOR_WORDSIZE>::Signed qregisterint;
typedef QIntegerForSize<Q_PROCESSOR_WORDSIZE>::Unsigned qregisteruint;
typedef QIntegerForSizeof<void *>::Unsigned quintptr;
typedef QIntegerForSizeof<void *>::Signed qptrdiff;
typedef qptrdiff qintptr;
using qsizetype = QIntegerForSizeof<std::size_t>::Signed;

// These custom definitions are necessary as we're not defining our
// datatypes in terms of the language ones, but in terms of integer
// types that have the sime size. For instance, on a 32-bit platform,
// qptrdiff is int, while ptrdiff_t may be aliased to long; therefore
// using %td to print a qptrdiff would be wrong (and raise -Wformat
// warnings), although both int and long have same bit size on that
// platform.
//
// We know that sizeof(size_t) == sizeof(void *) == sizeof(qptrdiff).
#if SIZE_MAX == 0xffffffffULL
#define PRIuQUINTPTR "u"
#define PRIoQUINTPTR "o"
#define PRIxQUINTPTR "x"
#define PRIXQUINTPTR "X"

#define PRIdQPTRDIFF "d"
#define PRIiQPTRDIFF "i"

#define PRIdQINTPTR "d"
#define PRIiQINTPTR "i"

#define PRIdQSIZETYPE "d"
#define PRIiQSIZETYPE "i"
#elif SIZE_MAX == 0xffffffffffffffffULL
#define PRIuQUINTPTR "llu"
#define PRIoQUINTPTR "llo"
#define PRIxQUINTPTR "llx"
#define PRIXQUINTPTR "llX"

#define PRIdQPTRDIFF "lld"
#define PRIiQPTRDIFF "lli"

#define PRIdQINTPTR "lld"
#define PRIiQINTPTR "lli"

#define PRIdQSIZETYPE "lld"
#define PRIiQSIZETYPE "lli"
#else
#error Unsupported platform (unknown value for SIZE_MAX)
#endif

// Define a native float16 type
namespace QtPrivate {
#if defined(__STDCPP_FLOAT16_T__)
#  define QFLOAT16_IS_NATIVE        1
using NativeFloat16Type = std::float16_t;
#elif defined(Q_CC_CLANG) && defined(__FLT16_MAX__) && 0
// disabled due to https://github.com/llvm/llvm-project/issues/56963
#  define QFLOAT16_IS_NATIVE        1
using NativeFloat16Type = decltype(__FLT16_MAX__);
#elif defined(Q_CC_GNU_ONLY) && defined(__FLT16_MAX__) && defined(__ARM_FP16_FORMAT_IEEE)
#  define QFLOAT16_IS_NATIVE        1
using NativeFloat16Type = __fp16;
#elif defined(Q_CC_GNU_ONLY) && defined(__FLT16_MAX__) && defined(__SSE2__)
#  define QFLOAT16_IS_NATIVE        1
using NativeFloat16Type = _Float16;
#else
#  define QFLOAT16_IS_NATIVE        0
using NativeFloat16Type = void;
#endif
} // QtPrivate

#endif // __cplusplus

QT_END_NAMESPACE

#endif // QTYPES_H
