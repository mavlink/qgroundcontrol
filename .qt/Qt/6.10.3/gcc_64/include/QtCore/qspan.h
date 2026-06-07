// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QSPAN_H
#define QSPAN_H

#include <QtCore/qcompilerdetection.h>
#include <QtCore/qtypes.h>
#include <QtCore/qcontainerfwd.h>

#include <array>
#include <cstddef>
#include <cassert>
#include <initializer_list>
#include <QtCore/q20iterator.h>
#include <QtCore/q20memory.h>
#ifdef __cpp_lib_span
#include <span>
#endif
#include <QtCore/q20type_traits.h>

QT_BEGIN_NAMESPACE

// like std::dynamic_extent
namespace q20 {
    inline constexpr auto dynamic_extent = std::size_t(-1);
} // namespace q20

QT_BEGIN_INCLUDE_NAMESPACE
#ifdef __cpp_lib_span
#ifdef __cpp_lib_concepts
namespace std::ranges {
// Officially, these are defined in <ranges>, but that is a heavy-hitter header.
// OTOH, <span> must specialize these variable templates, too, so we assume that
// <span> includes some meaningful subset of <ranges> and just go ahead and use them:
template <typename T, std::size_t E>
constexpr inline bool enable_borrowed_range<QT_PREPEND_NAMESPACE(QSpan)<T, E>> = true;
template <typename T, std::size_t E>
constexpr inline bool enable_view<QT_PREPEND_NAMESPACE(QSpan)<T, E>> = true;
} // namespace std::ranges
#endif // __cpp_lib_concepts
#endif // __cpp_lib_span
QT_END_INCLUDE_NAMESPACE

namespace QSpanPrivate {

template <typename From, typename To>
std::conditional_t<std::is_const_v<From>, const To &, To &> // like [forward]/6.1 COPY_CONST
const_propagated(To &in) { return in; }

template <typename T, std::size_t E> class QSpanBase;

template <typename T>
struct is_qspan_helper : std::false_type {};
template <typename T, std::size_t E>
struct is_qspan_helper<QSpan<T, E>> : std::true_type {};
template <typename T, std::size_t E>
struct is_qspan_helper<QSpanBase<T, E>> : std::true_type {};
template <typename T>
using is_qspan = is_qspan_helper<q20::remove_cvref_t<T>>;

template <typename T>
struct is_std_span_helper : std::false_type {};
#ifdef __cpp_lib_span
template <typename T, std::size_t E>
struct is_std_span_helper<std::span<T, E>> : std::true_type {};
#endif // __cpp_lib_span
template <typename T>
using is_std_span = is_std_span_helper<q20::remove_cvref_t<T>>;

template <typename T>
struct is_std_array_helper : std::false_type {};
template <typename T, std::size_t N>
struct is_std_array_helper<std::array<T, N>> : std::true_type {};
template <typename T>
using is_std_array = is_std_array_helper<q20::remove_cvref_t<T>>;

template <typename From, typename To>
using is_qualification_conversion =
    std::is_convertible<From(*)[], To(*)[]>; // https://eel.is/c++draft/span.cons#note-1
template <typename From, typename To>
constexpr inline bool is_qualification_conversion_v = is_qualification_conversion<From, To>::value;

namespace AdlTester {
#define MAKE_ADL_TEST(what) \
    using std:: what; /* bring into scope */ \
    template <typename T> using what ## _result = decltype( what (std::declval<T&&>())); \
    /* end */
MAKE_ADL_TEST(begin)
MAKE_ADL_TEST(data)
MAKE_ADL_TEST(size)
#undef MAKE_ADL_TEST
}

// Replacements for std::ranges::XXX(), but only bringing in ADL XXX()s,
// not doing the extra work C++20 requires
template <typename Range>
AdlTester::begin_result<Range> adl_begin(Range &&r) { using std::begin; return begin(r); }
template <typename Range>
AdlTester::data_result<Range>  adl_data(Range &&r)  { using std::data; return data(r); }
template <typename Range>
AdlTester::size_result<Range>  adl_size(Range &&r)  { using std::size; return size(r); }

// Replacement for std::ranges::iterator_t (which depends on C++20 std::ranges::begin)
// This one uses adl_begin() instead.
template <typename Range>
using iterator_t = decltype(QSpanPrivate::adl_begin(std::declval<Range&>()));
template <typename Range>
using range_reference_t = q20::iter_reference_t<QSpanPrivate::iterator_t<Range>>;

template <typename T>
class QSpanCommon {
protected:
    template <typename Iterator>
    using is_compatible_iterator = std::conjunction<
            // ### C++20: extend to contiguous_iteratorss
            std::is_base_of<
                std::random_access_iterator_tag,
                typename std::iterator_traits<Iterator>::iterator_category
            >,
            is_qualification_conversion<
                std::remove_reference_t<q20::iter_reference_t<Iterator>>,
                T
            >
        >;
    template <typename Iterator, typename End>
    using is_compatible_iterator_and_sentinel = std::conjunction<
            // ### C++20: extend to contiguous_iterators and real sentinels
            is_compatible_iterator<Iterator>,
            std::negation<std::is_convertible<End, std::size_t>>
        >;
    template <typename Range, typename = void> // wrap use of SFINAE-unfriendly iterator_t:
    struct is_compatible_range_helper : std::false_type {};
    template <typename Range>
    struct is_compatible_range_helper<Range, std::void_t<QSpanPrivate::iterator_t<Range>>>
        : is_compatible_iterator<QSpanPrivate::iterator_t<Range>> {};
    template <typename Range>
    using is_compatible_range = std::conjunction<
            // ### C++20: extend to contiguous_iterators
            std::negation<is_qspan<Range>>,
            std::negation<is_std_span<Range>>,
            std::negation<is_std_array<Range>>,
            std::negation<std::is_array<q20::remove_cvref_t<Range>>>,
            is_compatible_range_helper<Range>
        >;

    // constraints
    template <typename Iterator>
    using if_compatible_iterator = std::enable_if_t<
                is_compatible_iterator<Iterator>::value
            , bool>;
    template <typename Iterator, typename End>
    using if_compatible_iterator_and_sentinel = std::enable_if_t<
                is_compatible_iterator_and_sentinel<Iterator, End>::value
            , bool>;
    template <typename Range>
    using if_compatible_range = std::enable_if_t<is_compatible_range<Range>::value, bool>;
}; // class QSpanCommon

template <typename T, std::size_t E>
class QSpanBase : protected QSpanCommon<T>
{
    static_assert(E < size_t{(std::numeric_limits<qsizetype>::max)()},
                  "QSpan only supports extents that fit into the signed size type (qsizetype).");

    template <typename S, std::size_t N>
    using if_compatible_array = std::enable_if_t<
            N == E && is_qualification_conversion_v<S, T>
        , bool>;

    template <typename S>
    using if_qualification_conversion = std::enable_if_t<
            is_qualification_conversion_v<S, T>
        , bool>;
protected:
    using Base = QSpanCommon<T>;

    // data members:
    T *m_data;
    static constexpr qsizetype m_size = qsizetype(E);

    // types and constants:
    // (in QSpan only)

    // constructors (need to be public d/t the way ctor inheriting works):
public:
    template <std::size_t E2 = E, std::enable_if_t<E2 == 0, bool> = true>
    Q_IMPLICIT constexpr QSpanBase() noexcept : m_data{nullptr} {}

    template <typename It, typename Base::template if_compatible_iterator<It> = true>
    explicit constexpr QSpanBase(It first, qsizetype count)
        : m_data{q20::to_address(first)}
    {
        Q_ASSERT(count == m_size);
    }

    template <typename It, typename End, typename Base::template if_compatible_iterator_and_sentinel<It, End> = true>
    explicit constexpr QSpanBase(It first, End last)
        : QSpanBase(first, last - first) {}

    template <size_t N, std::enable_if_t<N == E, bool> = true>
    Q_IMPLICIT constexpr QSpanBase(q20::type_identity_t<T> (&arr)[N]) noexcept
        : QSpanBase(arr, N) {}

    template <typename S, size_t N, if_compatible_array<S, N> = true>
    Q_IMPLICIT constexpr QSpanBase(std::array<S, N> &arr) noexcept
        : QSpanBase(arr.data(), N) {}

    template <typename S, size_t N, if_compatible_array<S, N> = true>
    Q_IMPLICIT constexpr QSpanBase(const std::array<S, N> &arr) noexcept
        : QSpanBase(arr.data(), N) {}

    template <typename Range, typename Base::template if_compatible_range<Range> = true>
    Q_IMPLICIT constexpr QSpanBase(Range &&r)
        : QSpanBase(QSpanPrivate::adl_data(QSpanPrivate::const_propagated<T>(r)), // no forward<>() here (std doesn't have it, either)
                    qsizetype(QSpanPrivate::adl_size(r))) // ditto, no forward<>()
    {}

    template <typename S, if_qualification_conversion<S> = true>
    Q_IMPLICIT constexpr QSpanBase(QSpan<S, E> other) noexcept
        : QSpanBase(other.data(), other.size())
    {}

    template <typename S, if_qualification_conversion<S> = true>
    Q_IMPLICIT constexpr QSpanBase(QSpan<S> other)
        : QSpanBase(other.data(), other.size())
    {}

    template <typename U = T, std::enable_if_t<std::is_const_v<U>, bool> = true>
    Q_IMPLICIT constexpr QSpanBase(std::initializer_list<std::remove_cv_t<T>> il)
        : QSpanBase(il.begin(), il.size())
    {}

#ifdef __cpp_lib_span
    template <typename S, if_qualification_conversion<S> = true>
    Q_IMPLICIT constexpr QSpanBase(std::span<S, E> other) noexcept
        : QSpanBase(other.data(), other.size())
    {}

    template <typename S, if_qualification_conversion<S> = true>
    Q_IMPLICIT constexpr QSpanBase(std::span<S> other)
        : QSpanBase(other.data(), other.size())
    {}
#endif // __cpp_lib_span
}; // class QSpanBase (fixed extent)

template <typename T>
class QSpanBase<T, q20::dynamic_extent> : protected QSpanCommon<T>
{
    template <typename S>
    using if_qualification_conversion = std::enable_if_t<
            is_qualification_conversion_v<S, T>
        , bool>;
protected:
    using Base = QSpanCommon<T>;

    // data members:
    T *m_data;
    qsizetype m_size;

    // constructors (need to be public d/t the way ctor inheriting works):
public:
    Q_IMPLICIT constexpr QSpanBase() noexcept : m_data{nullptr}, m_size{0} {}

    template <typename It, typename Base::template if_compatible_iterator<It> = true>
    Q_IMPLICIT constexpr QSpanBase(It first, qsizetype count)
        : m_data{q20::to_address(first)}, m_size{count} {}

    template <typename It, typename End, typename Base::template if_compatible_iterator_and_sentinel<It, End> = true>
    Q_IMPLICIT constexpr QSpanBase(It first, End last)
        : QSpanBase(first, last - first) {}

    template <size_t N>
    Q_IMPLICIT constexpr QSpanBase(q20::type_identity_t<T> (&arr)[N]) noexcept
        : QSpanBase(arr, N) {}

    template <typename S, size_t N, if_qualification_conversion<S> = true>
    Q_IMPLICIT constexpr QSpanBase(std::array<S, N> &arr) noexcept
        : QSpanBase(arr.data(), N) {}

    template <typename S, size_t N, if_qualification_conversion<S> = true>
    Q_IMPLICIT constexpr QSpanBase(const std::array<S, N> &arr) noexcept
        : QSpanBase(arr.data(), N) {}

    template <typename Range, typename Base::template if_compatible_range<Range> = true>
    Q_IMPLICIT constexpr QSpanBase(Range &&r)
        : QSpanBase(QSpanPrivate::adl_data(QSpanPrivate::const_propagated<T>(r)), // no forward<>() here (std doesn't have it, either)
                    qsizetype(QSpanPrivate::adl_size(r))) // ditto, no forward<>()
    {}

    template <typename S, size_t N, if_qualification_conversion<S> = true>
    Q_IMPLICIT constexpr QSpanBase(QSpan<S, N> other) noexcept
        : QSpanBase(other.data(), other.size())
    {}

    template <typename U = T, std::enable_if_t<std::is_const_v<U>, bool> = true>
    Q_IMPLICIT constexpr QSpanBase(std::initializer_list<std::remove_cv_t<T>> il) noexcept
        : QSpanBase(il.begin(), il.size())
    {}

#ifdef __cpp_lib_span
    template <typename S, size_t N, if_qualification_conversion<S> = true>
    Q_IMPLICIT constexpr QSpanBase(std::span<S, N> other) noexcept
        : QSpanBase(other.data(), other.size())
    {}
#endif // __cpp_lib_span
}; // class QSpanBase (dynamic extent)

} // namespace QSpanPrivate

template <typename T, std::size_t E>
class QSpan
#ifndef Q_QDOC
    : private QSpanPrivate::QSpanBase<T, E>
#endif
{
    using Base = QSpanPrivate::QSpanBase<T, E>;
    Q_ALWAYS_INLINE constexpr void verify([[maybe_unused]] qsizetype pos = 0,
                                          [[maybe_unused]] qsizetype n = 1) const
    {
        Q_ASSERT(pos >= 0);
        Q_ASSERT(pos <= size());
        Q_ASSERT(n >= 0);
        Q_ASSERT(n <= size() - pos);
    }

    template <std::size_t N>
    static constexpr bool subspan_always_succeeds_v = N <= E && E != q20::dynamic_extent;
public:
    // constants and types
    using value_type = std::remove_cv_t<T>;
#ifdef QT_COMPILER_HAS_LWG3346
    using iterator_concept = std::contiguous_iterator_tag;
    using element_type = T;
#endif
    using size_type = qsizetype;               // difference to std::span
    using difference_type = qptrdiff;          // difference to std::span
    using pointer = T*;
    using const_pointer = const T*;
    using reference = T&;
    using const_reference = const T&;
    using iterator = pointer;                  // implementation-defined choice
    using const_iterator = const_pointer;      // implementation-defined choice
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;
    static constexpr std::size_t extent = E;

    // [span.cons], constructors, copy, and assignment
    using Base::Base;
#ifdef Q_QDOC
    template <typename It> using if_compatible_iterator = bool;
    template <typename S> using if_qualification_conversion = bool;
    template <typename Range> using if_compatible_range = bool;
    template <typename It, if_compatible_iterator<It> = true> constexpr QSpan(It first, qsizetype count);
    template <typename It, if_compatible_iterator<It> = true> constexpr QSpan(It first, It last);
    template <size_t N> constexpr QSpan(q20::type_identity_t<T> (&arr)[N]) noexcept;
    template <typename S, size_t N, if_qualification_conversion<S> = true> constexpr QSpan(std::array<S, N> &arr) noexcept;
    template <typename S, size_t N, if_qualification_conversion<S> = true> constexpr QSpan(const std::array<S, N> &arr) noexcept;
    template <typename Range, if_compatible_range<Range> = true> constexpr QSpan(Range &&r);
    template <typename S, size_t N, if_qualification_conversion<S> = true> constexpr QSpan(QSpan<S, N> other) noexcept;
    template <typename S, size_t N, if_qualification_conversion<S> = true> constexpr QSpan(std::span<S, N> other) noexcept;
    constexpr QSpan(std::initializer_list<value_type> il);
#endif // Q_QDOC

    // [span.obs]
    [[nodiscard]] constexpr size_type size() const noexcept { return this->m_size; }
    [[nodiscard]] constexpr size_type size_bytes() const noexcept { return size() * sizeof(T); }
    [[nodiscard]] constexpr bool empty() const noexcept { return size() == 0; }

    // [span.elem]
    [[nodiscard]] constexpr reference operator[](size_type idx) const
    { verify(idx); return data()[idx]; }
    [[nodiscard]] constexpr reference front() const { verify(); return *data(); }
    [[nodiscard]] constexpr reference back() const  { verify(); return data()[size() - 1]; }
    [[nodiscard]] constexpr pointer data() const noexcept { return this->m_data; }

    // [span.iterators]
    [[nodiscard]] constexpr iterator begin() const noexcept { return data(); }
    [[nodiscard]] constexpr iterator end() const noexcept { return data() + size(); }
    [[nodiscard]] constexpr const_iterator cbegin() const noexcept { return begin(); }
    [[nodiscard]] constexpr const_iterator cend() const noexcept { return end(); }
    [[nodiscard]] constexpr reverse_iterator rbegin() const noexcept { return reverse_iterator{end()}; }
    [[nodiscard]] constexpr reverse_iterator rend() const noexcept { return reverse_iterator{begin()}; }
    [[nodiscard]] constexpr const_reverse_iterator crbegin() const noexcept { return rbegin(); }
    [[nodiscard]] constexpr const_reverse_iterator crend() const noexcept { return rend(); }

    // [span.sub]
    template <std::size_t Count>
    [[nodiscard]] constexpr QSpan<T, Count> first() const
        noexcept(subspan_always_succeeds_v<Count>)
    {
        static_assert(Count <= E,
                      "Count cannot be larger than the span's extent.");
        verify(0, Count);
        return QSpan<T, Count>{data(), Count};
    }

    template <std::size_t Count>
    [[nodiscard]] constexpr QSpan<T, Count> last() const
        noexcept(subspan_always_succeeds_v<Count>)
    {
        static_assert(Count <= E,
                      "Count cannot be larger than the span's extent.");
        verify(0, Count);
        return QSpan<T, Count>{data() + (size() - Count), Count};
    }

    template <std::size_t Offset>
    [[nodiscard]] constexpr auto subspan() const
        noexcept(subspan_always_succeeds_v<Offset>)
    {
        static_assert(Offset <= E,
                      "Offset cannot be larger than the span's extent.");
        verify(Offset, 0);
        if constexpr (E == q20::dynamic_extent)
            return QSpan<T>{data() + Offset, qsizetype(size() - Offset)};
        else
            return QSpan<T, E - Offset>{data() + Offset, qsizetype(E - Offset)};
    }

    template <std::size_t Offset, std::size_t Count>
    [[nodiscard]] constexpr auto subspan() const
        noexcept(subspan_always_succeeds_v<Offset + Count>)
    { return subspan<Offset>().template first<Count>(); }

    [[nodiscard]] constexpr QSpan<T> first(size_type n) const { verify(0, n); return {data(), n}; }
    [[nodiscard]] constexpr QSpan<T> last(size_type n)  const { verify(0, n); return {data() + (size() - n), n}; }
    [[nodiscard]] constexpr QSpan<T> subspan(size_type pos) const { verify(pos, 0); return {data() + pos, size() - pos}; }
    [[nodiscard]] constexpr QSpan<T> subspan(size_type pos, size_type n) const { return subspan(pos).first(n); }

    // Qt-compatibility API:
    [[nodiscard]] constexpr bool isEmpty() const noexcept { return empty(); }
    // nullary first()/last() clash with first<>() and last<>(), so they're not provided for QSpan
    [[nodiscard]] constexpr QSpan<T> sliced(size_type pos) const { return subspan(pos); }
    [[nodiscard]] constexpr QSpan<T> sliced(size_type pos, size_type n) const { return subspan(pos, n); }
    [[nodiscard]] constexpr QSpan<T> chopped(size_type n) const { verify(0, n); return first(size() - n); }

#ifdef __cpp_concepts
#  define QT_ONLY_IF_DYNAMIC_SPAN(DECL) \
    DECL requires(E == q20::dynamic_extent)
#else
#  define QT_ONLY_IF_DYNAMIC_SPAN(DECL) \
    template <size_t M = E, typename = std::enable_if_t<M == q20::dynamic_extent>> DECL
#endif
    QT_ONLY_IF_DYNAMIC_SPAN(
    constexpr void slice(size_type pos)
    )
    { *this = sliced(pos); }
    QT_ONLY_IF_DYNAMIC_SPAN(
    constexpr void slice(size_type pos, size_type n)
    )
    { *this = sliced(pos, n); }
    QT_ONLY_IF_DYNAMIC_SPAN(
    constexpr void chop(size_type n)
    )
    { *this = chopped(n); }
#undef QT_ONLY_IF_DYNAMIC_SPAN

private:
    // [span.objectrep]
    [[nodiscard]] friend
    QSpan<const std::byte, E == q20::dynamic_extent ? q20::dynamic_extent : E * sizeof(T)>
    as_bytes(QSpan s) noexcept
    {
        using R = QSpan<const std::byte, E == q20::dynamic_extent ? q20::dynamic_extent : E * sizeof(T)>;
        return R{reinterpret_cast<const std::byte *>(s.data()), s.size_bytes()};
    }

    template <typename U>
    using if_mutable = std::enable_if_t<!std::is_const_v<U>, bool>;

#ifndef Q_QDOC
    template <typename T2 = T, if_mutable<T2> = true>
#endif
    [[nodiscard]] friend
    QSpan<std::byte, E == q20::dynamic_extent ? q20::dynamic_extent : E * sizeof(T)>
    as_writable_bytes(QSpan s) noexcept
    {
        using R = QSpan<std::byte, E == q20::dynamic_extent ? q20::dynamic_extent : E * sizeof(T)>;
        return R{reinterpret_cast<std::byte *>(s.data()), s.size_bytes()};
    }
}; // class QSpan

// [span.deduct]
template <class It, class EndOrSize>
QSpan(It, EndOrSize) -> QSpan<std::remove_reference_t<q20::iter_reference_t<It>>>;
template <class T, std::size_t N>
QSpan(T (&)[N]) -> QSpan<T, N>;
template <class T, std::size_t N>
QSpan(std::array<T, N> &) -> QSpan<T, N>;
template <class T, std::size_t N>
QSpan(const std::array<T, N> &) -> QSpan<const T, N>;
template <class R>
QSpan(R&&) -> QSpan<std::remove_reference_t<QSpanPrivate::range_reference_t<R>>>;

QT_END_NAMESPACE

#endif // QSPAN_H
