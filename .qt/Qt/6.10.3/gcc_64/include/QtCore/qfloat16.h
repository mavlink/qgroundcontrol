// Copyright (C) 2022 The Qt Company Ltd.
// Copyright (C) 2016 by Southwest Research Institute (R)
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QFLOAT16_H
#define QFLOAT16_H

#include <QtCore/qcompare.h>
#include <QtCore/qglobal.h>
#include <QtCore/qhashfunctions.h>
#include <QtCore/qmath.h>
#include <QtCore/qnamespace.h>
#include <QtCore/qtconfigmacros.h>
#include <QtCore/qtformat_impl.h>
#include <QtCore/qtypes.h>

#include <limits>
#include <string.h>
#include <type_traits>

#if defined(QT_COMPILER_SUPPORTS_F16C) && defined(__AVX2__) && !defined(__F16C__)
// All processors that support AVX2 do support F16C too, so we could enable the
// feature unconditionally if __AVX2__ is defined. However, all currently
// supported compilers except Microsoft's are able to define __F16C__ on their
// own when the user enables the feature, so we'll trust them.
#  if defined(Q_CC_MSVC) && !defined(Q_CC_CLANG)
#    define __F16C__        1
#  endif
#endif

#if defined(QT_COMPILER_SUPPORTS_F16C) && defined(__F16C__)
#include <immintrin.h>
#endif

QT_BEGIN_NAMESPACE

#if 0
#pragma qt_class(QFloat16)
#pragma qt_no_master_include
#endif

#ifndef QT_NO_DATASTREAM
class QDataStream;
#endif
class QTextStream;

class qfloat16
{
    struct Wrap
    {
        // To let our private constructor work, without other code seeing
        // ambiguity when constructing from int, double &c.
        quint16 b16;
        constexpr inline explicit Wrap(int value) : b16(quint16(value)) {}
    };

    template <typename T>
    using if_type_is_integral = std::enable_if_t<std::is_integral_v<std::remove_reference_t<T>>, bool>;

public:
    using NativeType = QtPrivate::NativeFloat16Type;

    static constexpr bool IsNative = QFLOAT16_IS_NATIVE;
    using NearestFloat = std::conditional_t<IsNative, NativeType, float>;

    constexpr inline qfloat16() noexcept : b16(0) {}
    explicit qfloat16(Qt::Initialization) noexcept { }

#if QFLOAT16_IS_NATIVE
    constexpr inline qfloat16(NativeType f) : nf(f) {}
    constexpr operator NativeType() const noexcept { return nf; }
#else
    inline qfloat16(float f) noexcept;
    inline operator float() const noexcept;
#endif
    template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T> && !std::is_same_v<T, NearestFloat>>>
    constexpr explicit qfloat16(T value) noexcept : qfloat16(NearestFloat(value)) {}

    // Support for qIs{Inf,NaN,Finite}:
    bool isInf() const noexcept { return (b16 & 0x7fff) == 0x7c00; }
    bool isNaN() const noexcept { return (b16 & 0x7fff) > 0x7c00; }
    bool isFinite() const noexcept { return (b16 & 0x7fff) < 0x7c00; }
    Q_CORE_EXPORT int fpClassify() const noexcept;
    // Can't specialize std::copysign() for qfloat16
    qfloat16 copySign(qfloat16 sign) const noexcept
    { return qfloat16(Wrap((sign.b16 & 0x8000) | (b16 & 0x7fff))); }
    // Support for std::numeric_limits<qfloat16>

#ifdef __STDCPP_FLOAT16_T__
private:
    using Bounds = std::numeric_limits<NativeType>;
public:
    static constexpr qfloat16 _limit_epsilon()    noexcept { return Bounds::epsilon(); }
    static constexpr qfloat16 _limit_min()        noexcept { return Bounds::min(); }
    static constexpr qfloat16 _limit_denorm_min() noexcept { return Bounds::denorm_min(); }
    static constexpr qfloat16 _limit_max()        noexcept { return Bounds::max(); }
    static constexpr qfloat16 _limit_lowest()     noexcept { return Bounds::lowest(); }
    static constexpr qfloat16 _limit_infinity()   noexcept { return Bounds::infinity(); }
    static constexpr qfloat16 _limit_quiet_NaN()  noexcept { return Bounds::quiet_NaN(); }
#if QT_CONFIG(signaling_nan)
    static constexpr qfloat16 _limit_signaling_NaN() noexcept { return Bounds::signaling_NaN(); }
#endif
#else
    static constexpr qfloat16 _limit_epsilon()    noexcept { return qfloat16(Wrap(0x1400)); }
    static constexpr qfloat16 _limit_min()        noexcept { return qfloat16(Wrap(0x400)); }
    static constexpr qfloat16 _limit_denorm_min() noexcept { return qfloat16(Wrap(1)); }
    static constexpr qfloat16 _limit_max()        noexcept { return qfloat16(Wrap(0x7bff)); }
    static constexpr qfloat16 _limit_lowest()     noexcept { return qfloat16(Wrap(0xfbff)); }
    static constexpr qfloat16 _limit_infinity()   noexcept { return qfloat16(Wrap(0x7c00)); }
    static constexpr qfloat16 _limit_quiet_NaN()  noexcept { return qfloat16(Wrap(0x7e00)); }
#if QT_CONFIG(signaling_nan)
    static constexpr qfloat16 _limit_signaling_NaN() noexcept { return qfloat16(Wrap(0x7d00)); }
#endif
#endif // __STDCPP_FLOAT16_T__
    inline constexpr bool isNormal() const noexcept
    { return (b16 & 0x7c00) && (b16 & 0x7c00) != 0x7c00; }
private:
    // ABI note: Qt 6's qfloat16 began with just a quint16 member so it ended
    // up passed in general purpose registers in any function call taking
    // qfloat16 by value (it has trivial copy constructors). This means the
    // integer member in the anonymous union below must remain until a
    // binary-incompatible version of Qt. If you remove it, on platforms using
    // the System V ABI for C, the native type is passed in FP registers.
    union {
        quint16 b16;
#if QFLOAT16_IS_NATIVE
        NativeType nf;
#endif
    };
    constexpr inline explicit qfloat16(Wrap nibble) noexcept :
#if QFLOAT16_IS_NATIVE && defined(__cpp_lib_bit_cast)
        nf(std::bit_cast<NativeType>(nibble.b16))
#else
        b16(nibble.b16)
#endif
    {}

    Q_CORE_EXPORT static const quint32 mantissatable[];
    Q_CORE_EXPORT static const quint32 exponenttable[];
    Q_CORE_EXPORT static const quint32 offsettable[];
    Q_CORE_EXPORT static const quint16 basetable[];
    Q_CORE_EXPORT static const quint16 shifttable[];
    Q_CORE_EXPORT static const quint32 roundtable[];

    friend bool qIsNull(qfloat16 f) noexcept;

    friend inline qfloat16 operator-(qfloat16 a) noexcept
    {
        qfloat16 f;
        f.b16 = a.b16 ^ quint16(0x8000);
        return f;
    }

    friend inline qfloat16 operator+(qfloat16 a, qfloat16 b) noexcept { return qfloat16(static_cast<NearestFloat>(a) + static_cast<NearestFloat>(b)); }
    friend inline qfloat16 operator-(qfloat16 a, qfloat16 b) noexcept { return qfloat16(static_cast<NearestFloat>(a) - static_cast<NearestFloat>(b)); }
    friend inline qfloat16 operator*(qfloat16 a, qfloat16 b) noexcept { return qfloat16(static_cast<NearestFloat>(a) * static_cast<NearestFloat>(b)); }
    friend inline qfloat16 operator/(qfloat16 a, qfloat16 b) noexcept { return qfloat16(static_cast<NearestFloat>(a) / static_cast<NearestFloat>(b)); }

    friend size_t qHash(qfloat16 key, size_t seed = 0) noexcept
    { return qHash(float(key), seed); } // 6.4 algorithm, so keep using it; ### Qt 7: fix QTBUG-116077

QT_WARNING_PUSH
QT_WARNING_DISABLE_GCC("-Wfloat-conversion")

#define QF16_MAKE_ARITH_OP_FP(FP, OP) \
    friend inline FP operator OP(qfloat16 lhs, FP rhs) noexcept { return static_cast<FP>(lhs) OP rhs; } \
    friend inline FP operator OP(FP lhs, qfloat16 rhs) noexcept { return lhs OP static_cast<FP>(rhs); }
#define QF16_MAKE_ARITH_OP_EQ_FP(FP, OP_EQ, OP) \
    friend inline qfloat16& operator OP_EQ(qfloat16& lhs, FP rhs) noexcept \
    { lhs = qfloat16(NearestFloat(static_cast<FP>(lhs) OP rhs)); return lhs; }
#define QF16_MAKE_ARITH_OP(FP) \
    QF16_MAKE_ARITH_OP_FP(FP, +) \
    QF16_MAKE_ARITH_OP_FP(FP, -) \
    QF16_MAKE_ARITH_OP_FP(FP, *) \
    QF16_MAKE_ARITH_OP_FP(FP, /) \
    QF16_MAKE_ARITH_OP_EQ_FP(FP, +=, +) \
    QF16_MAKE_ARITH_OP_EQ_FP(FP, -=, -) \
    QF16_MAKE_ARITH_OP_EQ_FP(FP, *=, *) \
    QF16_MAKE_ARITH_OP_EQ_FP(FP, /=, /)

    QF16_MAKE_ARITH_OP(long double)
    QF16_MAKE_ARITH_OP(double)
    QF16_MAKE_ARITH_OP(float)
#if QFLOAT16_IS_NATIVE
    QF16_MAKE_ARITH_OP(NativeType)
#endif
#undef QF16_MAKE_ARITH_OP
#undef QF16_MAKE_ARITH_OP_FP

#define QF16_MAKE_ARITH_OP_INT(OP) \
    friend inline double operator OP(qfloat16 lhs, int rhs) noexcept { return static_cast<double>(lhs) OP rhs; } \
    friend inline double operator OP(int lhs, qfloat16 rhs) noexcept { return lhs OP static_cast<double>(rhs); }

    QF16_MAKE_ARITH_OP_INT(+)
    QF16_MAKE_ARITH_OP_INT(-)
    QF16_MAKE_ARITH_OP_INT(*)
    QF16_MAKE_ARITH_OP_INT(/)
#undef QF16_MAKE_ARITH_OP_INT

QT_WARNING_DISABLE_FLOAT_COMPARE

#if QFLOAT16_IS_NATIVE
#  define QF16_CONSTEXPR constexpr
#  define QF16_PARTIALLY_ORDERED Q_DECLARE_PARTIALLY_ORDERED_LITERAL_TYPE
#else
#  define QF16_CONSTEXPR
#  define QF16_PARTIALLY_ORDERED Q_DECLARE_PARTIALLY_ORDERED
#endif

    friend QF16_CONSTEXPR bool comparesEqual(const qfloat16 &lhs, const qfloat16 &rhs) noexcept
    { return static_cast<NearestFloat>(lhs) == static_cast<NearestFloat>(rhs); }
    friend QF16_CONSTEXPR
    Qt::partial_ordering compareThreeWay(const qfloat16 &lhs, const qfloat16 &rhs) noexcept
    { return Qt::compareThreeWay(static_cast<NearestFloat>(lhs), static_cast<NearestFloat>(rhs)); }
    QF16_PARTIALLY_ORDERED(qfloat16)

#define QF16_MAKE_ORDER_OP_FP(FP) \
    friend QF16_CONSTEXPR bool comparesEqual(const qfloat16 &lhs, FP rhs) noexcept \
    { return static_cast<FP>(lhs) == rhs; } \
    friend QF16_CONSTEXPR \
    Qt::partial_ordering compareThreeWay(const qfloat16 &lhs, FP rhs) noexcept \
    { return Qt::compareThreeWay(static_cast<FP>(lhs), rhs); } \
    QF16_PARTIALLY_ORDERED(qfloat16, FP)

    QF16_MAKE_ORDER_OP_FP(long double)
    QF16_MAKE_ORDER_OP_FP(double)
    QF16_MAKE_ORDER_OP_FP(float)
#if QFLOAT16_IS_NATIVE
    QF16_MAKE_ORDER_OP_FP(qfloat16::NativeType)
#endif
#undef QF16_MAKE_ORDER_OP_FP

    template <typename T, if_type_is_integral<T> = true>
    friend QF16_CONSTEXPR bool comparesEqual(const qfloat16 &lhs, T rhs) noexcept
    { return static_cast<NearestFloat>(lhs) == static_cast<NearestFloat>(rhs); }
    template <typename T, if_type_is_integral<T> = true>
    friend QF16_CONSTEXPR Qt::partial_ordering compareThreeWay(const qfloat16 &lhs, T rhs) noexcept
    { return Qt::compareThreeWay(static_cast<NearestFloat>(lhs), static_cast<NearestFloat>(rhs)); }

    QF16_PARTIALLY_ORDERED(qfloat16, qint8)
    QF16_PARTIALLY_ORDERED(qfloat16, quint8)
    QF16_PARTIALLY_ORDERED(qfloat16, qint16)
    QF16_PARTIALLY_ORDERED(qfloat16, quint16)
    QF16_PARTIALLY_ORDERED(qfloat16, qint32)
    QF16_PARTIALLY_ORDERED(qfloat16, quint32)
    QF16_PARTIALLY_ORDERED(qfloat16, long)
    QF16_PARTIALLY_ORDERED(qfloat16, unsigned long)
    QF16_PARTIALLY_ORDERED(qfloat16, qint64)
    QF16_PARTIALLY_ORDERED(qfloat16, quint64)
#ifdef QT_SUPPORTS_INT128
    QF16_PARTIALLY_ORDERED(qfloat16, qint128)
    QF16_PARTIALLY_ORDERED(qfloat16, quint128)
#endif

#undef QF16_PARTIALLY_ORDERED
#undef QF16_CONSTEXPR

QT_WARNING_POP

#ifndef QT_NO_DATASTREAM
    friend Q_CORE_EXPORT QDataStream &operator<<(QDataStream &ds, qfloat16 f);
    friend Q_CORE_EXPORT QDataStream &operator>>(QDataStream &ds, qfloat16 &f);
#endif
    friend Q_CORE_EXPORT QTextStream &operator<<(QTextStream &ts, qfloat16 f);
    friend Q_CORE_EXPORT QTextStream &operator>>(QTextStream &ts, qfloat16 &f);
};

Q_DECLARE_TYPEINFO(qfloat16, Q_PRIMITIVE_TYPE);

Q_CORE_EXPORT void qFloatToFloat16(qfloat16 *, const float *, qsizetype length) noexcept;
Q_CORE_EXPORT void qFloatFromFloat16(float *, const qfloat16 *, qsizetype length) noexcept;

// Complement qnumeric.h:
[[nodiscard]] inline bool qIsInf(qfloat16 f) noexcept { return f.isInf(); }
[[nodiscard]] inline bool qIsNaN(qfloat16 f) noexcept { return f.isNaN(); }
[[nodiscard]] inline bool qIsFinite(qfloat16 f) noexcept { return f.isFinite(); }
[[nodiscard]] inline int qFpClassify(qfloat16 f) noexcept { return f.fpClassify(); }
// [[nodiscard]] quint32 qFloatDistance(qfloat16 a, qfloat16 b);

[[nodiscard]] inline qfloat16 qSqrt(qfloat16 f)
{
#if defined(__cpp_lib_extended_float) && defined(__STDCPP_FLOAT16_T__) && 0
    // https://wg21.link/p1467 - disabled until tested
    using namespace std;
    return sqrt(f);
#elif QFLOAT16_IS_NATIVE && defined(__HAVE_FLOAT16) && __HAVE_FLOAT16
    // This C library (glibc) has sqrtf16().
    return sqrtf16(f);
#else
    bool mathUpdatesErrno = true;
#  if defined(__NO_MATH_ERRNO__) || defined(_M_FP_FAST)
    mathUpdatesErrno = false;
#  elif defined(math_errhandling)
    mathUpdatesErrno = (math_errhandling & MATH_ERRNO);
#  endif

    // We don't need to set errno to EDOM if (f >= 0 && f != -0 && !isnan(f))
    // (or if we don't care about errno in the first place). We can merge the
    // NaN check with by negating and inverting: !(0 > f), and leaving zero to
    // sqrtf().
    if (!mathUpdatesErrno || !(0 > f)) {
#  if defined(__AVX512FP16__)
        __m128h v = _mm_set_sh(f);
        v = _mm_sqrt_sh(v, v);
        return _mm_cvtsh_h(v);
#  endif
    }

    // WG14's N2601 does not provide a way to tell which types an
    // implementation supports, so we assume it doesn't and fall back to FP32
    float f32 = float(f);
    f32 = sqrtf(f32);
    return qfloat16::NearestFloat(f32);
#endif
}

// The remainder of these utility functions complement qglobal.h
[[nodiscard]] inline int qRound(qfloat16 d)
{ return qRound(static_cast<float>(d)); }

[[nodiscard]] inline qint64 qRound64(qfloat16 d)
{ return qRound64(static_cast<float>(d)); }

[[nodiscard]] inline bool qFuzzyCompare(qfloat16 p1, qfloat16 p2) noexcept
{
    qfloat16::NearestFloat f1 = static_cast<qfloat16::NearestFloat>(p1);
    qfloat16::NearestFloat f2 = static_cast<qfloat16::NearestFloat>(p2);
    // The significand precision for IEEE754 half precision is
    // 11 bits (10 explicitly stored), or approximately 3 decimal
    // digits.  In selecting the fuzzy comparison factor of 102.5f
    // (that is, (2^10+1)/10) below, we effectively select a
    // window of about 1 (least significant) decimal digit about
    // which the two operands can vary and still return true.
    return (qAbs(f1 - f2) * 102.5f <= qMin(qAbs(f1), qAbs(f2)));
}

/*!
  \internal
*/
[[nodiscard]] inline bool qFuzzyIsNull(qfloat16 f) noexcept
{
    return qAbs(f) < 0.00976f; // 1/102.5 to 3 significant digits; see qFuzzyCompare()
}

[[nodiscard]] inline bool qIsNull(qfloat16 f) noexcept
{
    return (f.b16 & static_cast<quint16>(0x7fff)) == 0;
}

inline int qIntCast(qfloat16 f) noexcept
{ return int(static_cast<qfloat16::NearestFloat>(f)); }

#if !defined(Q_QDOC) && !QFLOAT16_IS_NATIVE
QT_WARNING_PUSH
QT_WARNING_DISABLE_CLANG("-Wc99-extensions")
QT_WARNING_DISABLE_GCC("-Wold-style-cast")
inline qfloat16::qfloat16(float f) noexcept
{
#if defined(QT_COMPILER_SUPPORTS_F16C) && defined(__F16C__)
    __m128 packsingle = _mm_set_ss(f);
    __m128i packhalf = _mm_cvtps_ph(packsingle, 0);
    b16 = quint16(_mm_extract_epi16(packhalf, 0));
#elif defined (__ARM_FP16_FORMAT_IEEE)
    __fp16 f16 = __fp16(f);
    memcpy(&b16, &f16, sizeof(quint16));
#else
    quint32 u;
    memcpy(&u, &f, sizeof(quint32));
    const quint32 signAndExp = u >> 23;
    const quint16 base = basetable[signAndExp];
    const quint16 shift = shifttable[signAndExp];
    const quint32 round = roundtable[signAndExp];
    quint32 mantissa = (u & 0x007fffff);
    if ((signAndExp & 0xff) == 0xff) {
        if (mantissa) // keep nan from truncating to inf
            mantissa = qMax(1U << shift, mantissa);
    } else {
        // Round half to even. First round up by adding one in the most
        // significant bit we'll be discarding:
        mantissa += round;
        // If the last bit we'll be keeping is now set, but all later bits are
        // clear, we were at half and shouldn't have rounded up; decrement will
        // clear this last kept bit. Any later set bit hides the decrement.
        if (mantissa & (1 << shift))
            --mantissa;
    }

    // We use add as the mantissa may overflow causing
    // the exp part to shift exactly one value.
    b16 = quint16(base + (mantissa >> shift));
#endif
}
QT_WARNING_POP

inline qfloat16::operator float() const noexcept
{
#if defined(QT_COMPILER_SUPPORTS_F16C) && defined(__F16C__)
    __m128i packhalf = _mm_cvtsi32_si128(b16);
    __m128 packsingle = _mm_cvtph_ps(packhalf);
    return _mm_cvtss_f32(packsingle);
#elif defined (__ARM_FP16_FORMAT_IEEE)
    __fp16 f16;
    memcpy(&f16, &b16, sizeof(quint16));
    return float(f16);
#else
    quint32 u = mantissatable[offsettable[b16 >> 10] + (b16 & 0x3ff)]
                + exponenttable[b16 >> 10];
    float f;
    memcpy(&f, &u, sizeof(quint32));
    return f;
#endif
}
#endif // Q_QDOC and non-native

/*
  qHypot compatibility; see ../kernel/qmath.h
*/
namespace QtPrivate {
template <> struct QHypotType<qfloat16, qfloat16>
{
    using type = qfloat16;
};
template <typename R> struct QHypotType<R, qfloat16>
{
    using type = std::conditional_t<std::is_floating_point_v<R>, R, double>;
};
template <typename R> struct QHypotType<qfloat16, R> : QHypotType<R, qfloat16>
{
};
}

// Avoid passing qfloat16 to std::hypot(), while ensuring return types
// consistent with the above:
inline auto qHypot(qfloat16 x, qfloat16 y)
{
#if defined(QT_COMPILER_SUPPORTS_F16C) && defined(__F16C__) || QFLOAT16_IS_NATIVE
    return QtPrivate::QHypotHelper<qfloat16>(x).add(y).result();
#else
    return qfloat16(qHypot(float(x), float(y)));
#endif
}

// in ../kernel/qmath.h
template<typename F, typename ...Fs> auto qHypot(F first, Fs... rest);

template <typename T> typename QtPrivate::QHypotType<T, qfloat16>::type
qHypot(T x, qfloat16 y)
{
    if constexpr (std::is_floating_point_v<T>)
        return qHypot(x, float(y));
    else
        return qHypot(qfloat16(x), y);
}
template <typename T> auto qHypot(qfloat16 x, T y)
{
    return qHypot(y, x);
}

#if defined(__cpp_lib_hypot) && __cpp_lib_hypot >= 201603L // Expected to be true
// If any are not qfloat16, convert each qfloat16 to float:
/* (The following splits the some-but-not-all-qfloat16 cases up, using
   (X|Y|Z)&~(X&Y&Z) = X ? ~(Y&Z) : Y|Z = X&~(Y&Z) | ~X&Y | ~X&~Y&Z,
   into non-overlapping cases, to avoid ambiguity.) */
template <typename Ty, typename Tz,
          typename std::enable_if<
              // Ty, Tz aren't both qfloat16:
              !(std::is_same_v<qfloat16, Ty> && std::is_same_v<qfloat16, Tz>), int>::type = 0>
auto qHypot(qfloat16 x, Ty y, Tz z) { return qHypot(qfloat16::NearestFloat(x), y, z); }
template <typename Tx, typename Tz,
          typename std::enable_if<
              // Tx isn't qfloat16:
              !std::is_same_v<qfloat16, Tx>, int>::type = 0>
auto qHypot(Tx x, qfloat16 y, Tz z) { return qHypot(x, qfloat16::NearestFloat(y), z); }
template <typename Tx, typename Ty,
          typename std::enable_if<
              // Neither Tx nor Ty is qfloat16:
              !std::is_same_v<qfloat16, Tx> && !std::is_same_v<qfloat16, Ty>, int>::type = 0>
auto qHypot(Tx x, Ty y, qfloat16 z) { return qHypot(x, y, qfloat16::NearestFloat(z)); }

// If all are qfloat16, stay with qfloat16 (albeit via float, if no native support):
inline auto qHypot(qfloat16 x, qfloat16 y, qfloat16 z)
{
#if (defined(QT_COMPILER_SUPPORTS_F16C) && defined(__F16C__)) || QFLOAT16_IS_NATIVE
    return QtPrivate::QHypotHelper<qfloat16>(x).add(y).add(z).result();
#else
    return qfloat16(qHypot(float(x), float(y), float(z)));
#endif
}
#endif // 3-arg std::hypot() is available

QT_END_NAMESPACE

namespace std {
template<>
class numeric_limits<QT_PREPEND_NAMESPACE(qfloat16)> : public numeric_limits<float>
{
public:
    /*
      Treat quint16 b16 as if it were:
      uint S: 1; // b16 >> 15 (sign); can be set for zero
      uint E: 5; // (b16 >> 10) & 0x1f (offset exponent)
      uint M: 10; // b16 & 0x3ff (adjusted mantissa)

      for E == 0: magnitude is M / 2.^{24}
      for 0 < E < 31: magnitude is (1. + M / 2.^{10}) * 2.^{E - 15)
      for E == 31: not finite
     */
    static constexpr int digits = 11;
    static constexpr int min_exponent = -13;
    static constexpr int max_exponent = 16;

    static constexpr int digits10 = 3;
    static constexpr int max_digits10 = 5;
    static constexpr int min_exponent10 = -4;
    static constexpr int max_exponent10 = 4;

    static constexpr QT_PREPEND_NAMESPACE(qfloat16) epsilon()
    { return QT_PREPEND_NAMESPACE(qfloat16)::_limit_epsilon(); }
    static constexpr QT_PREPEND_NAMESPACE(qfloat16) (min)()
    { return QT_PREPEND_NAMESPACE(qfloat16)::_limit_min(); }
    static constexpr QT_PREPEND_NAMESPACE(qfloat16) denorm_min()
    { return QT_PREPEND_NAMESPACE(qfloat16)::_limit_denorm_min(); }
    static constexpr QT_PREPEND_NAMESPACE(qfloat16) (max)()
    { return QT_PREPEND_NAMESPACE(qfloat16)::_limit_max(); }
    static constexpr QT_PREPEND_NAMESPACE(qfloat16) lowest()
    { return QT_PREPEND_NAMESPACE(qfloat16)::_limit_lowest(); }
    static constexpr QT_PREPEND_NAMESPACE(qfloat16) infinity()
    { return QT_PREPEND_NAMESPACE(qfloat16)::_limit_infinity(); }
    static constexpr QT_PREPEND_NAMESPACE(qfloat16) quiet_NaN()
    { return QT_PREPEND_NAMESPACE(qfloat16)::_limit_quiet_NaN(); }
#if QT_CONFIG(signaling_nan)
    static constexpr QT_PREPEND_NAMESPACE(qfloat16) signaling_NaN()
    { return QT_PREPEND_NAMESPACE(qfloat16)::_limit_signaling_NaN(); }
#else
    static constexpr bool has_signaling_NaN = false;
#endif
};

template<> class numeric_limits<const QT_PREPEND_NAMESPACE(qfloat16)>
    : public numeric_limits<QT_PREPEND_NAMESPACE(qfloat16)> {};
template<> class numeric_limits<volatile QT_PREPEND_NAMESPACE(qfloat16)>
    : public numeric_limits<QT_PREPEND_NAMESPACE(qfloat16)> {};
template<> class numeric_limits<const volatile QT_PREPEND_NAMESPACE(qfloat16)>
    : public numeric_limits<QT_PREPEND_NAMESPACE(qfloat16)> {};

// Adding overloads to std isn't allowed, so we can't extend this to support
// for fpclassify(), isnormal() &c. (which, furthermore, are macros on MinGW).
} // namespace std

// std::format support
#ifdef QT_SUPPORTS_STD_FORMAT

QT_BEGIN_NAMESPACE

namespace QtPrivate {

// [format.formatter.spec] / 5
template <typename T, typename CharT>
constexpr bool FormatterDoesNotExist =
        std::negation_v<
                std::disjunction<
                        std::is_default_constructible<std::formatter<T, CharT>>,
                        std::is_copy_constructible<std::formatter<T, CharT>>,
                        std::is_move_constructible<std::formatter<T, CharT>>,
                        std::is_copy_assignable<std::formatter<T, CharT>>,
                        std::is_move_assignable<std::formatter<T, CharT>>
                        >
                >;

template <typename CharT>
using QFloat16FormatterBaseType =
        std::conditional_t<FormatterDoesNotExist<qfloat16::NearestFloat, CharT>,
                           float,
                           qfloat16::NearestFloat>;

} // namespace QtPrivate

QT_END_NAMESPACE

namespace std {
template <typename CharT>
struct formatter<QT_PREPEND_NAMESPACE(qfloat16), CharT>
    : std::formatter<QT_PREPEND_NAMESPACE(QtPrivate::QFloat16FormatterBaseType<CharT>), CharT>
{
    template <typename FormatContext>
    auto format(QT_PREPEND_NAMESPACE(qfloat16) val, FormatContext &ctx) const
    {
        using FloatType = QT_PREPEND_NAMESPACE(QtPrivate::QFloat16FormatterBaseType<CharT>);
        return std::formatter<FloatType, CharT>::format(FloatType(val), ctx);
    }
};
} // namespace std

#endif // QT_SUPPORTS_STD_FORMAT

#endif // QFLOAT16_H
