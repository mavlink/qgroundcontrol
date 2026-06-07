// Copyright (C) 2021 The Qt Company Ltd.
// Copyright (C) 2025 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Giuseppe D'Angelo <giuseppe.dangelo@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:data-parser

#ifndef QNUMERIC_H
#define QNUMERIC_H

#if 0
#pragma qt_class(QtNumeric)
#endif

#include <QtCore/qassert.h>
#include <QtCore/qminmax.h>
#include <QtCore/qtconfigmacros.h>
#include <QtCore/qtcoreexports.h>
#include <QtCore/qtypes.h>

#include <cmath>
#include <limits>
#include <QtCore/q20type_traits.h>

// min() and max() may be #defined by windows.h if that is included before, but we need them
// for std::numeric_limits below. You should not use the min() and max() macros, so we just #undef.
#ifdef min
#  undef min
#  undef max
#endif

//
// SIMDe (SIMD Everywhere) can't be used if intrin.h has been included as many definitions
// conflict.   Defining Q_NUMERIC_NO_INTRINSICS allows SIMDe users to use Qt, at the cost of
// falling back to the prior implementations of qMulOverflow and qAddOverflow.
//
#if defined(Q_CC_MSVC) && !defined(Q_NUMERIC_NO_INTRINSICS)
#  include <intrin.h>
#  include <float.h>
#  if defined(Q_PROCESSOR_X86) || defined(Q_PROCESSOR_X86_64)
#    define Q_HAVE_ADDCARRY
#  endif
#  if defined(Q_PROCESSOR_X86_64) || defined(Q_PROCESSOR_ARM_64)
#    define Q_INTRINSIC_MUL_OVERFLOW64
#    define Q_UMULH(v1, v2) __umulh(v1, v2)
#    define Q_SMULH(v1, v2) __mulh(v1, v2)
#    pragma intrinsic(__umulh)
#    pragma intrinsic(__mulh)
#  endif
#endif

QT_BEGIN_NAMESPACE

// To match std::is{inf,nan,finite} functions:
template <typename T>
constexpr typename std::enable_if<std::is_integral<T>::value, bool>::type
qIsInf(T) { return false; }
template <typename T>
constexpr typename std::enable_if<std::is_integral<T>::value, bool>::type
qIsNaN(T) { return false; }
template <typename T>
constexpr typename std::enable_if<std::is_integral<T>::value, bool>::type
qIsFinite(T) { return true; }

// Floating-point types (see qfloat16.h for its overloads).
Q_CORE_EXPORT Q_DECL_CONST_FUNCTION bool qIsInf(double d);
Q_CORE_EXPORT Q_DECL_CONST_FUNCTION bool qIsNaN(double d);
Q_CORE_EXPORT Q_DECL_CONST_FUNCTION bool qIsFinite(double d);
Q_CORE_EXPORT Q_DECL_CONST_FUNCTION int qFpClassify(double val);
Q_CORE_EXPORT Q_DECL_CONST_FUNCTION bool qIsInf(float f);
Q_CORE_EXPORT Q_DECL_CONST_FUNCTION bool qIsNaN(float f);
Q_CORE_EXPORT Q_DECL_CONST_FUNCTION bool qIsFinite(float f);
Q_CORE_EXPORT Q_DECL_CONST_FUNCTION int qFpClassify(float val);

#if QT_CONFIG(signaling_nan)
Q_CORE_EXPORT Q_DECL_CONST_FUNCTION double qSNaN();
#endif
Q_CORE_EXPORT Q_DECL_CONST_FUNCTION double qQNaN();
Q_CORE_EXPORT Q_DECL_CONST_FUNCTION double qInf();

Q_CORE_EXPORT quint32 qFloatDistance(float a, float b);
Q_CORE_EXPORT quint64 qFloatDistance(double a, double b);

#define Q_INFINITY (QT_PREPEND_NAMESPACE(qInf)())
#if QT_CONFIG(signaling_nan)
#  define Q_SNAN (QT_PREPEND_NAMESPACE(qSNaN)())
#endif
#define Q_QNAN (QT_PREPEND_NAMESPACE(qQNaN)())

// Overflow math.
// This provides efficient implementations for int, unsigned, qsizetype and
// size_t. Implementations for 8- and 16-bit types will work but may not be as
// efficient. Implementations for 64-bit may be missing on 32-bit platforms.

// All the GCC and Clang versions we support have constexpr
// builtins for overflowing arithmetic.
#if defined(Q_CC_GNU_ONLY) \
    || defined(Q_CC_CLANG_ONLY) \
    || __has_builtin(__builtin_add_overflow)
# define Q_NUMERIC_USE_GCC_OVERFLOW_BUILTINS
// On 32-bit, Clang < 14 will fail to link if multiplying 64-bit
// quantities (emits an unresolved call to __mulodi4), so we can't use
// the builtin in that case.
# if !(QT_POINTER_SIZE == 4 && defined(Q_CC_CLANG_ONLY) && Q_CC_CLANG_ONLY < 1400)
#  define Q_INTRINSIC_MUL_OVERFLOW64
# endif
#endif

namespace QtPrivate {
// Generic versions of (some) overflowing math functions, private API.
template <typename T>
constexpr inline
typename std::enable_if_t<std::is_unsigned_v<T>, bool>
qAddOverflowGeneric(T v1, T v2, T *r)
{
    // unsigned additions are well-defined
    *r = v1 + v2;
    return v1 > T(v1 + v2);
}

// Wide multiplication.
// It has been isolated in its own function so that it can be tested.
// Note that this implementation requires a T that doesn't undergo
// promotions.
template <typename T>
constexpr inline
typename std::enable_if_t<std::is_same_v<T, decltype(+T{})>, bool>
qMulOverflowWideMultiplication(T v1, T v2, T *r)
{
    // This is a glorified long/school-grade multiplication,
    // that considers each input of N bits as two halves of N/2 bits:
    //
    // v1 = 2^(N/2) * v1_hi + v1_lo
    // v2 = 2^(N/2) * v2_hi + v2_lo
    //
    // Therefore, v1*v2 = 2^N     * v1_hi * v2_hi +
    //                    2^(N/2) * v1_hi * v2_lo +
    //                    2^(N/2) * v1_lo * v2_hi +
    //                            * v1_lo * v2_lo
    //
    // Using the N bits of precision we have we can perform the hi*lo
    // multiplications safely; that is never going to overflow.
    //
    // Then we can sum together these partial results:
    //
    //                 [ v1_hi | v1_lo ] *
    //                 [ v2_hi | v2_lo ] =
    //                 -------------------
    //                 [ v1_lo * v2_lo ] +
    //         [ v1_hi * v2_lo ]         +  // shifted because it's * 2^(N/2)
    //         [ v2_hi * v1_lo ]         +  // shifted because it's * 2^(N/2)
    // [ v1_hi * v2_hi ]                 =  // shifted because it's * 2^N
    // -------------------------------
    // [     high     ][     low       ]    // exact result (in 2^(2N) bits)
    //
    // ... except that this way we'll need to bring some carries, so
    // we'll do a slightly smarter sum.
    //
    // We need high for detecting overflows, even if we are not returning it.

    // Get multiplication by zero out of the way
    if (v1 == 0 || v2 == 0) {
        *r = T(0);
        return false;
    }

    // Extract the absolute values as unsigned
    // (will fix the sign later)
    using U = std::make_unsigned_t<T>;
    const U v1_abs = (v1 >= 0) ? U(v1) : (U(0) - U(v1));
    const U v2_abs = (v2 >= 0) ? U(v2) : (U(0) - U(v2));

    // Masks for N/2 bits
    constexpr std::size_t half_width = (sizeof(U) * 8) / 2;
    const U half_mask = ~U(0) >> half_width;

    // Split in low and half quantities
    const U v1_lo = v1_abs & half_mask;
    const U v1_hi = v1_abs >> half_width;
    const U v2_lo = v2_abs & half_mask;
    const U v2_hi = v2_abs >> half_width;

    // Cross-product; this will never overflow
    const U lo_lo = v1_lo * v2_lo;
    const U lo_hi = v1_lo * v2_hi;
    const U hi_lo = v1_hi * v2_lo;
    const U hi_hi = v1_hi * v2_hi;

    // We could sum directly the cross-products, but then we'd have to
    // keep track of carries. This avoids it.
    const U tmp = (lo_lo >> half_width) + (hi_lo & half_mask) + lo_hi;
    U result_hi = (hi_lo >> half_width) + (tmp >> half_width) + hi_hi;
    U result_lo = (tmp << half_width) | (lo_lo & half_mask);

    if constexpr (std::is_unsigned_v<T>) {
        // If the source was unsigned, we're done; a non-zero high
        // signals overflow.
        *r = result_lo;
        return result_hi != U(0);
    } else {
        // We need to set the correct sign back, and check for overflow.
        const bool isNegative = (v1 < T(0)) != (v2 < T(0));
        if (isNegative) {
            // Result is negative; calculate two's complement of the
            // [high, low] pair, by inverting the bits and adding 1,
            // which is equivalent to negating it in unsigned
            // arithmetic.
            // This operation should be done on the pair as a whole,
            // but we have the individual components, so start by
            // calculating two's complement of low:
            result_lo = U(0) - result_lo;

            // If result_lo is 0, it means that the addition of 1 into
            // it has overflown, so now we have a carry to add into the
            // inverted high:
            result_hi = ~result_hi;
            if (result_lo == 0)
                result_hi += U(1);
        }

        *r = result_lo;
        // Overflow has happened if result_hi is not a sign extension
        // of the sign bit of result_lo. Note the usage of T, not U.
        return result_hi != U(*r >> std::numeric_limits<T>::digits);
    }
}

template <typename T, typename Enable = void>
constexpr inline bool HasLargerInt = false;
template <typename T>
constexpr inline bool HasLargerInt<T, std::void_t<typename QIntegerForSize<sizeof(T) * 2>::Unsigned>> = true;

template <typename T>
constexpr inline
typename std::enable_if_t<(std::is_unsigned_v<T> || std::is_signed_v<T>), bool>
qMulOverflowGeneric(T v1, T v2, T *r)
{
    // This function is a generic fallback for qMulOverflow,
    // called either by constant or non-constant evaluation,
    // if the compiler does not have builtins or intrinsics itself.
    //
    // (For instance, this is never going to be called on GCC or recent
    // Clang, as their builtins will be used in all cases.)
    //
    // If a compiler does have builtins, please amend qMulOverflow
    // directly.

    if constexpr (HasLargerInt<T>) {
        // Use the next biggest type if available
        using LargerInt = QIntegerForSize<sizeof(T) * 2>;
        using Larger = typename std::conditional_t<std::is_signed_v<T>,
                typename LargerInt::Signed, typename LargerInt::Unsigned>;
        Larger lr = Larger(v1) * Larger(v2);
        *r = T(lr);
        return lr > (std::numeric_limits<T>::max)() || lr < (std::numeric_limits<T>::min)();
    } else {
        // Otherwise fall back to a wide multiplication
        return qMulOverflowWideMultiplication(v1, v2, r);
    }
}
} // namespace QtPrivate

template <typename T>
constexpr inline
typename std::enable_if_t<std::is_unsigned_v<T>, bool>
qAddOverflow(T v1, T v2, T *r)
{
#if defined(Q_NUMERIC_USE_GCC_OVERFLOW_BUILTINS)
    return __builtin_add_overflow(v1, v2, r);
#else
    if (q20::is_constant_evaluated())
        return QtPrivate::qAddOverflowGeneric(v1, v2, r);
# if defined(Q_HAVE_ADDCARRY)
    // We can use intrinsics for the unsigned operations with MSVC
    if constexpr (std::is_same_v<T, unsigned>) {
        return _addcarry_u32(0, v1, v2, r);
    } else if constexpr (std::is_same_v<T, quint64>) {
#    if defined(Q_PROCESSOR_X86_64)
        return _addcarry_u64(0, v1, v2, reinterpret_cast<unsigned __int64 *>(r));
#    else
        uint low, high;
        uchar carry = _addcarry_u32(0, unsigned(v1), unsigned(v2), &low);
        carry = _addcarry_u32(carry, v1 >> 32, v2 >> 32, &high);
        *r = (quint64(high) << 32) | low;
        return carry;
#    endif // defined(Q_PROCESSOR_X86_64)
    }
# endif // defined(Q_HAVE_ADDCARRY)
    return QtPrivate::qAddOverflowGeneric(v1, v2, r);
#endif // defined(Q_NUMERIC_USE_GCC_OVERFLOW_BUILTINS)
}

template <typename T>
constexpr inline
typename std::enable_if_t<std::is_signed_v<T>, bool>
qAddOverflow(T v1, T v2, T *r)
{
#if defined(Q_NUMERIC_USE_GCC_OVERFLOW_BUILTINS)
    return __builtin_add_overflow(v1, v2, r);
#else
    // Here's how we calculate the overflow:
    // 1) unsigned addition is well-defined, so we can always execute it
    // 2) conversion from unsigned back to signed is implementation-
    //    defined and in the implementations we use, it's a no-op.
    // 3) signed integer overflow happens if the sign of the two input operands
    //    is the same but the sign of the result is different. In other words,
    //    the sign of the result must be the same as the sign of either
    //    operand.

    using U = typename std::make_unsigned_t<T>;
    *r = T(U(v1) + U(v2));

    // Two's complement equivalent (generates slightly shorter code):
    //  x ^ y             is negative if x and y have different signs
    //  x & y             is negative if x and y are negative
    // (x ^ z) & (y ^ z)  is negative if x and z have different signs
    //                    AND y and z have different signs
    return ((v1 ^ *r) & (v2 ^ *r)) < 0;
#endif // defined(Q_NUMERIC_USE_GCC_OVERFLOW_BUILTINS)
}

template <typename T>
constexpr inline
typename std::enable_if_t<std::is_unsigned_v<T>, bool>
qSubOverflow(T v1, T v2, T *r)
{
#if defined(Q_NUMERIC_USE_GCC_OVERFLOW_BUILTINS)
    return __builtin_sub_overflow(v1, v2, r);
#else
    // unsigned subtractions are well-defined
    *r = v1 - v2;
    return v1 < v2;
#endif // defined(Q_NUMERIC_USE_GCC_OVERFLOW_BUILTINS)
}

template <typename T>
constexpr inline
typename std::enable_if_t<std::is_signed_v<T>, bool>
qSubOverflow(T v1, T v2, T *r)
{
#if defined(Q_NUMERIC_USE_GCC_OVERFLOW_BUILTINS)
    return __builtin_sub_overflow(v1, v2, r);
#else
    // See above for explanation. This is the same with some signs reversed.
    // We can't use qAddOverflow(v1, -v2, r) because it would be UB if
    // v2 == std::numeric_limits<T>::min().

    using U = typename std::make_unsigned_t<T>;
    *r = T(U(v1) - U(v2));

    return ((v1 ^ *r) & (~v2 ^ *r)) < 0;
#endif // defined(Q_NUMERIC_USE_GCC_OVERFLOW_BUILTINS)
}

template <typename T>
constexpr inline
typename std::enable_if_t<std::is_unsigned_v<T> || std::is_signed_v<T>, bool>
qMulOverflow(T v1, T v2, T *r)
{
#if defined(Q_NUMERIC_USE_GCC_OVERFLOW_BUILTINS)
# if defined(Q_INTRINSIC_MUL_OVERFLOW64)
    return __builtin_mul_overflow(v1, v2, r);
# else
    if constexpr (sizeof(T) <= 4)
        return __builtin_mul_overflow(v1, v2, r);
    else
        return QtPrivate::qMulOverflowGeneric(v1, v2, r);
# endif
#else
    if (q20::is_constant_evaluated())
        return QtPrivate::qMulOverflowGeneric(v1, v2, r);

# if defined(Q_INTRINSIC_MUL_OVERFLOW64)
    if constexpr (std::is_unsigned_v<T> && (sizeof(T) == sizeof(quint64))) {
        // T is 64 bit; either unsigned long long,
        // or unsigned long on LP64 platforms.
        *r = v1 * v2;
        return T(Q_UMULH(v1, v2));
    } else if constexpr (std::is_signed_v<T> && (sizeof(T) == sizeof(qint64))) {
        // This is slightly more complex than the unsigned case above: the sign bit
        // of 'low' must be replicated as the entire 'high', so the only valid
        // values for 'high' are 0 and -1. Use unsigned multiply since it's the same
        // as signed for the low bits and use a signed right shift to verify that
        // 'high' is nothing but sign bits that match the sign of 'low'.

        qint64 high = Q_SMULH(v1, v2);
        *r = qint64(quint64(v1) * quint64(v2));
        return (*r >> 63) != high;
    }
# endif // defined(Q_INTRINSIC_MUL_OVERFLOW64)

    return QtPrivate::qMulOverflowGeneric(v1, v2, r);
#endif // defined(Q_NUMERIC_USE_GCC_OVERFLOW_BUILTINS)
}

#undef Q_HAVE_ADDCARRY
#undef Q_NUMERIC_USE_GCC_OVERFLOW_BUILTINS

// Implementations for addition, subtraction or multiplication by a
// compile-time constant. For addition and subtraction, we simply call the code
// that detects overflow at runtime. For multiplication, we compare to the
// maximum possible values before multiplying to ensure no overflow happens.

template <typename T, T V2> constexpr bool qAddOverflow(T v1, std::integral_constant<T, V2>, T *r)
{
    return qAddOverflow(v1, V2, r);
}

template <auto V2, typename T> constexpr bool qAddOverflow(T v1, T *r)
{
    return qAddOverflow(v1, std::integral_constant<T, V2>{}, r);
}

template <typename T, T V2> constexpr bool qSubOverflow(T v1, std::integral_constant<T, V2>, T *r)
{
    return qSubOverflow(v1, V2, r);
}

template <auto V2, typename T> constexpr bool qSubOverflow(T v1, T *r)
{
    return qSubOverflow(v1, std::integral_constant<T, V2>{}, r);
}

template <typename T, T V2> constexpr bool qMulOverflow(T v1, std::integral_constant<T, V2>, T *r)
{
    // Runtime detection for anything smaller than or equal to a register
    // width, as most architectures' multiplication instructions actually
    // produce a result twice as wide as the input registers, allowing us to
    // efficiently detect the overflow.
    if constexpr (sizeof(T) <= sizeof(qregisteruint)) {
        return qMulOverflow(v1, V2, r);

#ifdef Q_INTRINSIC_MUL_OVERFLOW64
    } else if constexpr (sizeof(T) <= sizeof(quint64)) {
        // If we have intrinsics detecting overflow of 64-bit multiplications,
        // then detect overflows through them up to 64 bits.
        return qMulOverflow(v1, V2, r);
#endif

    } else if constexpr (V2 == 0 || V2 == 1) {
        // trivial cases (and simplify logic below due to division by zero)
        *r = v1 * V2;
        return false;
    } else if constexpr (V2 == -1) {
        // multiplication by -1 is valid *except* for signed minimum values
        // (necessary to avoid diving min() by -1, which is an overflow)
        if (v1 < 0 && v1 == (std::numeric_limits<T>::min)())
            return true;
        *r = -v1;
        return false;
    } else {
        // For 64-bit multiplications on 32-bit platforms, let's instead compare v1
        // against the bounds that would overflow.
        constexpr T Highest = (std::numeric_limits<T>::max)() / V2;
        constexpr T Lowest = (std::numeric_limits<T>::min)() / V2;
        if constexpr (Highest > Lowest) {
            if (v1 > Highest || v1 < Lowest)
                return true;
        } else {
            // this can only happen if V2 < 0
            static_assert(V2 < 0);
            if (v1 > Lowest || v1 < Highest)
                return true;
        }

        *r = v1 * V2;
        return false;
    }
}

template <auto V2, typename T> constexpr bool qMulOverflow(T v1, T *r)
{
    if constexpr (V2 == 2)
        return qAddOverflow(v1, v1, r);
    return qMulOverflow(v1, std::integral_constant<T, V2>{}, r);
}

template <typename T>
constexpr inline T qAbs(const T &t)
{
    if constexpr (std::is_integral_v<T> && std::is_signed_v<T>)
        Q_ASSERT(t != std::numeric_limits<T>::min());
    return t >= 0 ? t : -t;
}

namespace QtPrivate {
template <typename T,
          typename std::enable_if_t<std::is_integral_v<T>, bool> = true>
constexpr inline auto qUnsignedAbs(T t)
{
    using U = std::make_unsigned_t<T>;
    return (t >= 0) ? U(t) : U(~U(t) + U(1));
}

template <typename Result,
          typename FP,
          typename std::enable_if_t<std::is_integral_v<Result>, bool> = true,
          typename std::enable_if_t<std::is_floating_point_v<FP>, bool> = true>
constexpr inline Result qCheckedFPConversionToInteger(FP value)
{
#ifdef QT_SUPPORTS_IS_CONSTANT_EVALUATED
    if (!q20::is_constant_evaluated())
        Q_ASSERT(!std::isnan(value));
#endif

    constexpr Result minimal = (std::numeric_limits<Result>::min)();
    constexpr Result maximal = (std::numeric_limits<Result>::max)();

    // We want to check that `value > minimal-1`. `minimal` is
    // precisely representable as FP (it's -2^N), but `minimal-1`
    // may not be. Just rearrange the terms:
    Q_ASSERT(value - FP(minimal) > FP(-1));

    // Symmetrically, `maximal` may not have a precise
    // representation, but `maximal+1` has, so calculate that:
    constexpr FP maximalPlusOne = FP(2) * (maximal / 2 + 1);
    // And check that we're below that:
    Q_ASSERT(value < maximalPlusOne);

    // If both checks passed, the result of truncation is representable
    // as `Result`:
    return Result(value);
}

namespace QRoundImpl {
// gcc < 10 doesn't have __has_builtin
#if defined(Q_PROCESSOR_ARM_64) && (__has_builtin(__builtin_round) || defined(Q_CC_GNU)) && !defined(Q_CC_CLANG)
// ARM64 has a single instruction that can do C++ rounding with conversion to integer.
// Note current clang versions have non-constexpr __builtin_round, ### allow clang this path when they fix it.
constexpr inline double qRound(double d)
{ return __builtin_round(d); }
constexpr inline float qRound(float f)
{ return __builtin_roundf(f); }
#elif defined(__SSE2__) && (__has_builtin(__builtin_copysign) || defined(Q_CC_GNU))
// SSE has binary operations directly on floating point making copysign fast
constexpr inline double qRound(double d)
{ return d + __builtin_copysign(0.5, d); }
constexpr inline float qRound(float f)
{ return f + __builtin_copysignf(0.5f, f); }
#else
constexpr inline double qRound(double d)
{ return d >= 0.0 ? d + 0.5 : d - 0.5; }
constexpr inline float qRound(float d)
{ return d >= 0.0f ? d + 0.5f : d - 0.5f; }
#endif
} // namespace QRoundImpl

// Like qRound, but have well-defined saturating behavior.
// NaN is not handled.
template <typename FP,
          typename std::enable_if_t<std::is_floating_point_v<FP>, bool> = true>
constexpr inline int qSaturateRound(FP value)
{
#ifdef QT_SUPPORTS_IS_CONSTANT_EVALUATED
    if (!q20::is_constant_evaluated())
        Q_ASSERT(!qIsNaN(value));
#endif
    constexpr FP MinBound = FP((std::numeric_limits<int>::min)());
    constexpr FP MaxBound = FP((std::numeric_limits<int>::max)());
    const FP beforeTruncation = QRoundImpl::qRound(value);
    return int(qBound(MinBound, beforeTruncation, MaxBound));
}
} // namespace QtPrivate

constexpr inline int qRound(double d)
{
    return QtPrivate::qCheckedFPConversionToInteger<int>(QtPrivate::QRoundImpl::qRound(d));
}

constexpr inline int qRound(float f)
{
    return QtPrivate::qCheckedFPConversionToInteger<int>(QtPrivate::QRoundImpl::qRound(f));
}

constexpr inline qint64 qRound64(double d)
{
    return QtPrivate::qCheckedFPConversionToInteger<qint64>(QtPrivate::QRoundImpl::qRound(d));
}

constexpr inline qint64 qRound64(float f)
{
    return QtPrivate::qCheckedFPConversionToInteger<qint64>(QtPrivate::QRoundImpl::qRound(f));
}

namespace QtPrivate {
template <typename T>
constexpr inline const T &min(const T &a, const T &b) { return (a < b) ? a : b; }
}

[[nodiscard]] constexpr bool qFuzzyCompare(double p1, double p2) noexcept
{
    return (qAbs(p1 - p2) * 1000000000000. <= QtPrivate::min(qAbs(p1), qAbs(p2)));
}

[[nodiscard]] constexpr bool qFuzzyCompare(float p1, float p2) noexcept
{
    return (qAbs(p1 - p2) * 100000.f <= QtPrivate::min(qAbs(p1), qAbs(p2)));
}

[[nodiscard]] constexpr bool qFuzzyIsNull(double d) noexcept
{
    return qAbs(d) <= 0.000000000001;
}

[[nodiscard]] constexpr bool qFuzzyIsNull(float f) noexcept
{
    return qAbs(f) <= 0.00001f;
}

QT_WARNING_PUSH
QT_WARNING_DISABLE_FLOAT_COMPARE

[[nodiscard]] constexpr bool qIsNull(double d) noexcept
{
    return d == 0.0;
}

[[nodiscard]] constexpr bool qIsNull(float f) noexcept
{
    return f == 0.0f;
}

QT_WARNING_POP

namespace QtPrivate {
/*
    A version of qFuzzyCompare that works for all values (qFuzzyCompare()
    requires that neither argument is numerically 0).

    It's private because we need a fix for the many qFuzzyCompare() uses that
    ignore the precondition, even for older branches.

    See QTBUG-142020 for discussion of a longer-term solution.
*/
template <typename T, typename S>
[[nodiscard]] constexpr bool fuzzyCompare(const T &lhs, const S &rhs) noexcept
{
    static_assert(noexcept(qIsNull(lhs) && qIsNull(rhs) && qFuzzyIsNull(lhs - rhs) && qFuzzyCompare(lhs, rhs)),
                  "The operations qIsNull(), qFuzzyIsNull() and qFuzzyCompare() must be noexcept "
                  "for both argument types!");
    return qIsNull(lhs) || qIsNull(rhs) ? qFuzzyIsNull(lhs - rhs) : qFuzzyCompare(lhs, rhs);
}
} // namespace QtPrivate


inline int qIntCast(double f) { return int(f); }
inline int qIntCast(float f) { return int(f); }

QT_END_NAMESPACE

#endif // QNUMERIC_H
