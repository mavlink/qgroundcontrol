// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2015 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Marc Mutz <marc.mutz@kdab.com>
// Copyright (C) 2024 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QHASHFUNCTIONS_H
#define QHASHFUNCTIONS_H

#include <QtCore/qstring.h>
#include <QtCore/qstringfwd.h>

#include <numeric> // for std::accumulate
#include <functional> // for std::hash
#include <utility> // For std::pair

#if 0
#pragma qt_class(QHashFunctions)
#endif

#if defined(Q_CC_MSVC)
#pragma warning( push )
#pragma warning( disable : 4311 ) // disable pointer truncation warning
#pragma warning( disable : 4127 ) // conditional expression is constant
#endif

QT_BEGIN_NAMESPACE

class QBitArray;

#if QT_DEPRECATED_SINCE(6,6)
QT_DEPRECATED_VERSION_X_6_6("Use QHashSeed instead")
Q_CORE_EXPORT int qGlobalQHashSeed();
QT_DEPRECATED_VERSION_X_6_6("Use QHashSeed instead")
Q_CORE_EXPORT void qSetGlobalQHashSeed(int newSeed);
#endif

struct QHashSeed
{
    constexpr QHashSeed(size_t d = 0) : data(d) {}
    constexpr operator size_t() const noexcept { return data; }

    static Q_CORE_EXPORT QHashSeed globalSeed() noexcept;
    static Q_CORE_EXPORT void setDeterministicGlobalSeed();
    static Q_CORE_EXPORT void resetRandomGlobalSeed();
private:
    size_t data;
};

// Whether, ∀ t of type T && ∀ seed, qHash(Key(t), seed) == qHash(t, seed)
template <typename Key, typename T> struct QHashHeterogeneousSearch : std::false_type {};

// Specializations
template <> struct QHashHeterogeneousSearch<QString, QStringView> : std::true_type {};
template <> struct QHashHeterogeneousSearch<QStringView, QString> : std::true_type {};
template <> struct QHashHeterogeneousSearch<QByteArray, QByteArrayView> : std::true_type {};
template <> struct QHashHeterogeneousSearch<QByteArrayView, QByteArray> : std::true_type {};
#ifndef Q_PROCESSOR_ARM
template <> struct QHashHeterogeneousSearch<QString, QLatin1StringView> : std::true_type {};
template <> struct QHashHeterogeneousSearch<QStringView, QLatin1StringView> : std::true_type {};
template <> struct QHashHeterogeneousSearch<QLatin1StringView, QString> : std::true_type {};
template <> struct QHashHeterogeneousSearch<QLatin1StringView, QStringView> : std::true_type {};
#endif

namespace QHashPrivate {

Q_DECL_CONST_FUNCTION constexpr size_t hash(size_t key, size_t seed) noexcept
{
    key ^= seed;
    if constexpr (sizeof(size_t) == 4) {
        key ^= key >> 16;
        key *= UINT32_C(0x45d9f3b);
        key ^= key >> 16;
        key *= UINT32_C(0x45d9f3b);
        key ^= key >> 16;
        return key;
    } else {
        quint64 key64 = key;
        key64 ^= key64 >> 32;
        key64 *= UINT64_C(0xd6e8feb86659fd93);
        key64 ^= key64 >> 32;
        key64 *= UINT64_C(0xd6e8feb86659fd93);
        key64 ^= key64 >> 32;
        return size_t(key64);
    }
}

template <typename T1, typename T2> static constexpr bool noexceptPairHash();
}

Q_CORE_EXPORT Q_DECL_PURE_FUNCTION size_t qHashBits(const void *p, size_t size, size_t seed = 0) noexcept;

// implementation below qHashMulti
template <typename T1, typename T2> inline size_t qHash(const std::pair<T1, T2> &key, size_t seed = 0)
    noexcept(QHashPrivate::noexceptPairHash<T1, T2>());

// C++ builtin types
#define QT_MK_QHASH_COMPAT(X) \
    template <typename T, std::enable_if_t<std::is_same_v<T, X>, bool> = true> \
    constexpr size_t qHash(T key, size_t seed = 0) noexcept \
    /* QHashPrivate::hash() xors before mixing, while 1-to-2-arg adapter xors after */ \
    { return QHashPrivate::hash(size_t(key), 0 QT7_ONLY(+ seed)) QT6_ONLY(^ seed); } \
    /* end */
QT_MK_QHASH_COMPAT(bool) // QTBUG-126674
Q_DECL_CONST_FUNCTION constexpr inline size_t qHash(char key, size_t seed = 0) noexcept
{ return QHashPrivate::hash(size_t(key), seed); }
Q_DECL_CONST_FUNCTION constexpr inline size_t qHash(uchar key, size_t seed = 0) noexcept
{ return QHashPrivate::hash(size_t(key), seed); }
Q_DECL_CONST_FUNCTION constexpr inline size_t qHash(signed char key, size_t seed = 0) noexcept
{ return QHashPrivate::hash(size_t(key), seed); }
Q_DECL_CONST_FUNCTION constexpr inline size_t qHash(ushort key, size_t seed = 0) noexcept
{ return QHashPrivate::hash(size_t(key), seed); }
Q_DECL_CONST_FUNCTION constexpr inline size_t qHash(short key, size_t seed = 0) noexcept
{ return QHashPrivate::hash(size_t(key), seed); }
Q_DECL_CONST_FUNCTION constexpr inline size_t qHash(uint key, size_t seed = 0) noexcept
{ return QHashPrivate::hash(size_t(key), seed); }
Q_DECL_CONST_FUNCTION constexpr inline size_t qHash(int key, size_t seed = 0) noexcept
{ return QHashPrivate::hash(size_t(key), seed); }
Q_DECL_CONST_FUNCTION constexpr inline size_t qHash(ulong key, size_t seed = 0) noexcept
{ return QHashPrivate::hash(size_t(key), seed); }
Q_DECL_CONST_FUNCTION constexpr inline size_t qHash(long key, size_t seed = 0) noexcept
{ return QHashPrivate::hash(size_t(key), seed); }
#undef QT_MK_QHASH_COMPAT
Q_DECL_CONST_FUNCTION constexpr inline size_t qHash(quint64 key, size_t seed = 0) noexcept
{
    if constexpr (sizeof(quint64) > sizeof(size_t))
        key ^= (key >> 32);
    return QHashPrivate::hash(size_t(key), seed);
}
Q_DECL_CONST_FUNCTION constexpr inline size_t qHash(qint64 key, size_t seed = 0) noexcept
{
    if constexpr (sizeof(qint64) > sizeof(size_t)) {
        // Avoid QTBUG-116080: we XOR the top half with its own sign bit:
        // - if the qint64 is in range of qint32, then signmask ^ high == 0
        //   (for Qt 7 only)
        // - if the qint64 is in range of quint32, then signmask == 0 and we
        //   do the same as the quint64 overload above
        quint32 high = quint32(quint64(key) >> 32);
        quint32 low = quint32(quint64(key));
        quint32 signmask = qint32(high) >> 31;  // all zeroes or all ones
        signmask = QT_VERSION_MAJOR > 6 ? signmask : 0;
        low ^= signmask ^ high;
        return qHash(low, seed);
    }
    return qHash(quint64(key), seed);
}
#ifdef QT_SUPPORTS_INT128
constexpr size_t qHash(quint128 key, size_t seed = 0) noexcept
{
    return qHash(quint64(key + (key >> 64)), seed);
}
constexpr size_t qHash(qint128 key, size_t seed = 0) noexcept
{
    // Avoid QTBUG-116080: same as above, but with double the sizes and without
    // the need for compatibility
    quint64 high = quint64(quint128(key) >> 64);
    quint64 low = quint64(quint128(key));
    quint64 signmask = qint64(high) >> 63; // all zeroes or all ones
    low += signmask ^ high;
    return qHash(low, seed);
}
#endif // QT_SUPPORTS_INT128
Q_DECL_CONST_FUNCTION inline size_t qHash(float key, size_t seed = 0) noexcept
{
    // ensure -0 gets mapped to 0
    key += 0.0f;
    uint k;
    memcpy(&k, &key, sizeof(float));
    return QHashPrivate::hash(k, seed);
}
Q_CORE_EXPORT Q_DECL_CONST_FUNCTION size_t qHash(double key, size_t seed = 0) noexcept;
Q_CORE_EXPORT Q_DECL_CONST_FUNCTION size_t qHash(long double key, size_t seed = 0) noexcept;
Q_DECL_CONST_FUNCTION constexpr inline size_t qHash(wchar_t key, size_t seed = 0) noexcept
{ return QHashPrivate::hash(size_t(key), seed); }
Q_DECL_CONST_FUNCTION constexpr inline size_t qHash(char16_t key, size_t seed = 0) noexcept
{ return QHashPrivate::hash(size_t(key), seed); }
Q_DECL_CONST_FUNCTION constexpr inline size_t qHash(char32_t key, size_t seed = 0) noexcept
{ return QHashPrivate::hash(size_t(key), seed); }
#ifdef __cpp_char8_t
Q_DECL_CONST_FUNCTION constexpr inline size_t qHash(char8_t key, size_t seed = 0) noexcept
{ return QHashPrivate::hash(size_t(key), seed); }
#endif
template <class T> inline size_t qHash(const T *key, size_t seed = 0) noexcept
{
    return qHash(reinterpret_cast<quintptr>(key), seed);
}
Q_DECL_CONST_FUNCTION constexpr inline size_t qHash(std::nullptr_t, size_t seed = 0) noexcept
{
    return seed;
}
template <class Enum, std::enable_if_t<std::is_enum_v<Enum>, bool> = true>
Q_DECL_CONST_FUNCTION constexpr inline size_t qHash(Enum e, size_t seed = 0) noexcept
{ return QHashPrivate::hash(qToUnderlying(e), seed); }

// (some) Qt types
Q_DECL_CONST_FUNCTION constexpr inline size_t qHash(const QChar key, size_t seed = 0) noexcept { return qHash(key.unicode(), seed); }

#if QT_CORE_REMOVED_SINCE(6, 4)
Q_CORE_EXPORT Q_DECL_PURE_FUNCTION size_t qHash(const QByteArray &key, size_t seed = 0) noexcept;
Q_CORE_EXPORT Q_DECL_PURE_FUNCTION size_t qHash(const QByteArrayView &key, size_t seed = 0) noexcept;
#else
Q_CORE_EXPORT Q_DECL_PURE_FUNCTION size_t qHash(QByteArrayView key, size_t seed = 0) noexcept;
inline Q_DECL_PURE_FUNCTION size_t qHash(const QByteArray &key, size_t seed = 0
        QT6_DECL_NEW_OVERLOAD_TAIL) noexcept
{ return qHash(qToByteArrayViewIgnoringNull(key), seed); }
#endif

Q_CORE_EXPORT Q_DECL_PURE_FUNCTION size_t qHash(QStringView key, size_t seed = 0) noexcept;
inline Q_DECL_PURE_FUNCTION size_t qHash(const QString &key, size_t seed = 0) noexcept
{ return qHash(QStringView{key}, seed); }
#ifndef QT_BOOTSTRAPPED
Q_CORE_EXPORT Q_DECL_PURE_FUNCTION size_t qHash(const QBitArray &key, size_t seed = 0) noexcept;
#endif
Q_CORE_EXPORT Q_DECL_PURE_FUNCTION size_t qHash(QLatin1StringView key, size_t seed = 0) noexcept;
Q_DECL_CONST_FUNCTION constexpr inline size_t qHash(QKeyCombination key, size_t seed = 0) noexcept
{ return qHash(key.toCombined(), seed); }
Q_CORE_EXPORT Q_DECL_PURE_FUNCTION uint qt_hash(QStringView key, uint chained = 0) noexcept;

template <typename Enum>
Q_DECL_CONST_FUNCTION constexpr inline size_t qHash(QFlags<Enum> flags, size_t seed = 0) noexcept
{ return qHash(flags.toInt(), seed); }

// ### Qt 7: remove this "catch-all" overload logic, and require users
// to provide the two-argument version of qHash.
#if (QT_VERSION < QT_VERSION_CHECK(7, 0, 0))
// Beware of moving this code from here. It needs to see all the
// declarations of qHash overloads for C++ fundamental types *before*
// its own declaration.
namespace QHashPrivate {
template <typename T, typename = void>
constexpr inline bool HasQHashSingleArgOverload = false;

template <typename T>
constexpr inline bool HasQHashSingleArgOverload<T, std::enable_if_t<
    std::is_convertible_v<decltype(qHash(std::declval<const T &>())), size_t>
>> = true;
}

// Add Args... to make this overload consistently a worse match than
// original 2-arg qHash overloads (QTBUG-126659)
template <typename T, typename...Args,
          std::enable_if_t<QHashPrivate::HasQHashSingleArgOverload<T>
                           && sizeof...(Args) == 0 && !std::is_enum_v<T>, bool> = true>
constexpr size_t qHash(const T &t, size_t seed, Args&&...) noexcept(noexcept(qHash(t)))
{ return qHash(t) ^ seed; }
#endif // < Qt 7

namespace QHashPrivate {

namespace detail {
// approximates std::equality_comparable_with
template <typename T, typename U, typename = void>
struct is_equality_comparable_with : std::false_type {};

template <typename T, typename U>
struct is_equality_comparable_with<T, U,
    std::void_t<
        decltype(bool(std::declval<T>() == std::declval<U>())),
        decltype(bool(std::declval<U>() == std::declval<T>())),
        decltype(bool(std::declval<T>() != std::declval<U>())),
        decltype(bool(std::declval<U>() != std::declval<T>()))
    >>
    : std::true_type {};
}

template <typename Key, typename T> struct HeterogeneouslySearchableWithHelper
    : std::conjunction<
        // if Key and T are not the same (member already exists)
        std::negation<std::is_same<Key, T>>,
        // but are comparable amongst each other
        detail::is_equality_comparable_with<Key, T>,
        // and supports heteregenous hashing
        QHashHeterogeneousSearch<Key, T>
    > {};

template <typename Key, typename T>
using HeterogeneouslySearchableWith = HeterogeneouslySearchableWithHelper<
        q20::remove_cvref_t<Key>,
        q20::remove_cvref_t<T>
>;

template <typename Key, typename K>
using if_heterogeneously_searchable_with = std::enable_if_t<
        QHashPrivate::HeterogeneouslySearchableWith<Key, K>::value,
    bool>;

}

template<typename T>
bool qHashEquals(const T &a, const T &b)
{
    return a == b;
}

template <typename T1, typename T2, QHashPrivate::if_heterogeneously_searchable_with<T1, T2> = true>
bool qHashEquals(const T1 &a, const T2 &b)
{
    return a == b;
}

namespace QtPrivate {
template <typename Mixer> struct QHashCombinerWithSeed : private Mixer
{
    using result_type = typename Mixer::result_type;
    size_t seed;
    constexpr QHashCombinerWithSeed(result_type s) noexcept : seed(s) {}

    template <typename T>
    constexpr result_type operator()(result_type result, const T &t) const
        noexcept(noexcept(qHash(t, seed)))
    {
        return Mixer::operator()(result, qHash(t, seed));
    }
};

#if QT_VERSION < QT_VERSION_CHECK(7, 0, 0) || defined(QT_BOOTSTRAPPED)
// Earlier Qt 6.x versions of qHashMulti() failed to pass the seed as the seed
// argument of qHash(), so this class exists for compatibility with user and
// inline code that relies on the old behavior. For Qt 7, we'll replace with
// the above version, except for the bootstrapped tools (which have no seed).
template <typename Mixer> struct QHashCombiner : private Mixer
{
    using result_type = typename Mixer::result_type;

    static constexpr size_t seed = 0;
    constexpr QHashCombiner(result_type) noexcept {}
    Q_DECL_DEPRECATED_X("pass the seed argument") constexpr QHashCombiner() noexcept {}

    template <typename T>
    constexpr result_type operator()(result_type result, const T &t) const
        noexcept(noexcept(qHash(t, seed)))
    {
        return Mixer::operator()(result, qHash(t, seed));
    }
};
#else
template <typename Mixer> using QHashCombiner = QHashCombinerWithSeed<Mixer>;
#endif

struct QHashCombineMixer
{
    typedef size_t result_type;
    constexpr result_type operator()(result_type result, result_type hash) const noexcept
    {
        // combiner taken from N3876 / boost::hash_combine
        return result ^ (hash + 0x9e3779b9 + (result << 6) + (result >> 2));
    }
};
using QHashCombine = QHashCombiner<QHashCombineMixer>;
using QHashCombineWithSeed = QHashCombinerWithSeed<QHashCombineMixer>;

struct QHashCombineCommutativeMixer : std::plus<size_t>
{
    // QHashCombine is a good hash combiner, but is not commutative,
    // ie. it depends on the order of the input elements. That is
    // usually what we want: {0,1,3} should hash differently than
    // {1,3,0}. Except when it isn't (e.g. for QSet and
    // QHash). Therefore, provide a commutative combiner, too.
    typedef size_t result_type;
};
using QHashCombineCommutative = QHashCombiner<QHashCombineCommutativeMixer>;
using QHashCombineCommutativeWithSeed = QHashCombinerWithSeed<QHashCombineCommutativeMixer>;

template <typename... T>
using QHashMultiReturnType = decltype(
    std::declval< std::enable_if_t<(sizeof...(T) > 0)> >(),
    (qHash(std::declval<const T &>(), size_t(0)), ...),
    size_t{}
);

// workaround for a MSVC ICE,
// https://developercommunity.visualstudio.com/content/problem/996540/internal-compiler-error-on-msvc-1924-when-doing-sf.html
template <typename T>
inline constexpr bool QNothrowHashableHelper_v = noexcept(qHash(std::declval<const T &>(), size_t(0)));

template <typename T, typename Enable = void>
struct QNothrowHashable : std::false_type {};

template <typename T>
struct QNothrowHashable<T, std::enable_if_t<QNothrowHashableHelper_v<T>>> : std::true_type {};

template <typename T>
constexpr inline bool QNothrowHashable_v = QNothrowHashable<T>::value;

} // namespace QtPrivate

template <typename... T>
constexpr
#ifdef Q_QDOC
size_t
#else
QtPrivate::QHashMultiReturnType<T...>
#endif
qHashMulti(size_t seed, const T &... args)
    noexcept(std::conjunction_v<QtPrivate::QNothrowHashable<T>...>)
{
    QtPrivate::QHashCombine hash(seed);
    return ((seed = hash(seed, args)), ...), seed;
}

template <typename... T>
constexpr
#ifdef Q_QDOC
size_t
#else
QtPrivate::QHashMultiReturnType<T...>
#endif
qHashMultiCommutative(size_t seed, const T &... args)
    noexcept(std::conjunction_v<QtPrivate::QNothrowHashable<T>...>)
{
    QtPrivate::QHashCombineCommutative hash(seed);
    return ((seed = hash(seed, args)), ...), seed;
}

template <typename InputIterator>
inline size_t qHashRange(InputIterator first, InputIterator last, size_t seed = 0)
    noexcept(noexcept(qHash(*first, 0))) // assume iterator operations don't throw
{
    return std::accumulate(first, last, seed, QtPrivate::QHashCombine(seed));
}

template <typename InputIterator>
inline size_t qHashRangeCommutative(InputIterator first, InputIterator last, size_t seed = 0)
    noexcept(noexcept(qHash(*first, 0))) // assume iterator operations don't throw
{
    return std::accumulate(first, last, seed, QtPrivate::QHashCombineCommutative(seed));
}

namespace QHashPrivate {
template <typename T1, typename T2> static constexpr bool noexceptPairHash()
{
    size_t seed = 0;
    return noexcept(qHash(std::declval<T1>(), seed)) && noexcept(qHash(std::declval<T2>(), seed));
}
} // QHashPrivate

template <typename T1, typename T2> inline size_t qHash(const std::pair<T1, T2> &key, size_t seed)
    noexcept(QHashPrivate::noexceptPairHash<T1, T2>())
{
    return qHashMulti(seed, key.first, key.second);
}

#define QT_SPECIALIZE_STD_HASH_TO_CALL_QHASH(Class, Arguments)      \
    QT_BEGIN_INCLUDE_NAMESPACE                                      \
    namespace std {                                                 \
        template <>                                                 \
        struct hash< QT_PREPEND_NAMESPACE(Class) > {                \
            using argument_type = QT_PREPEND_NAMESPACE(Class);      \
            using result_type = size_t;                             \
            size_t operator()(Arguments s) const                    \
              noexcept(QT_PREPEND_NAMESPACE(                        \
                     QtPrivate::QNothrowHashable_v)<argument_type>) \
            {                                                       \
                /* this seeds qHash with the result of */           \
                /* std::hash applied to an int, to reap */          \
                /* any protection against predictable hash */       \
                /* values the std implementation may provide */     \
                using QT_PREPEND_NAMESPACE(qHash);                  \
                return qHash(s, qHash(std::hash<int>{}(0)));        \
            }                                                       \
        };                                                          \
    }                                                               \
    QT_END_INCLUDE_NAMESPACE                                        \
    /*end*/

#define QT_SPECIALIZE_STD_HASH_TO_CALL_QHASH_BY_CREF(Class) \
    QT_SPECIALIZE_STD_HASH_TO_CALL_QHASH(Class, const argument_type &)
#define QT_SPECIALIZE_STD_HASH_TO_CALL_QHASH_BY_VALUE(Class) \
    QT_SPECIALIZE_STD_HASH_TO_CALL_QHASH(Class, argument_type)

QT_SPECIALIZE_STD_HASH_TO_CALL_QHASH_BY_CREF(QString)
QT_SPECIALIZE_STD_HASH_TO_CALL_QHASH_BY_VALUE(QStringView)
QT_SPECIALIZE_STD_HASH_TO_CALL_QHASH_BY_VALUE(QLatin1StringView)
QT_SPECIALIZE_STD_HASH_TO_CALL_QHASH_BY_VALUE(QByteArrayView)
QT_SPECIALIZE_STD_HASH_TO_CALL_QHASH_BY_CREF(QByteArray)
#ifndef QT_BOOTSTRAPPED
QT_SPECIALIZE_STD_HASH_TO_CALL_QHASH_BY_CREF(QBitArray)
#endif

QT_END_NAMESPACE

#if defined(Q_CC_MSVC)
#pragma warning( pop )
#endif

#endif // QHASHFUNCTIONS_H
