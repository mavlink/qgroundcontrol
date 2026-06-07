// Copyright (C) 2020 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Marc Mutz <marc.mutz@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:data-parser
#ifndef QUTF8STRINGVIEW_H
#define QUTF8STRINGVIEW_H

#if 0
#pragma qt_class(QUtf8StringView)
#endif

#include <QtCore/qstringalgorithms.h>
#include <QtCore/qstringfwd.h>
#include <QtCore/qarraydata.h> // for QContainerImplHelper
#include <QtCore/qbytearrayview.h>
#include <QtCore/qcompare.h>
#include <QtCore/qcontainerfwd.h>

#include <string>
#include <string_view>
#include <QtCore/q20type_traits.h>

QT_BEGIN_NAMESPACE

namespace QtPrivate {
template <typename Char>
using IsCompatibleChar8TypeHelper = std::disjunction<
#ifdef __cpp_char8_t
        std::is_same<Char, char8_t>,
#endif
        std::is_same<Char, char>,
        std::is_same<Char, uchar>,
        std::is_same<Char, signed char>
    >;
template <typename Char>
using IsCompatibleChar8Type
    = IsCompatibleChar8TypeHelper<q20::remove_cvref_t<Char>>;

template <typename Pointer>
struct IsCompatiblePointer8Helper : std::false_type {};
template <typename Char>
struct IsCompatiblePointer8Helper<Char*>
    : IsCompatibleChar8Type<Char> {};
template <typename Pointer>
using IsCompatiblePointer8
    = IsCompatiblePointer8Helper<q20::remove_cvref_t<Pointer>>;

template <typename T, typename Enable = void>
struct IsContainerCompatibleWithQUtf8StringView : std::false_type {};

template <typename T>
struct IsContainerCompatibleWithQUtf8StringView<T, std::enable_if_t<std::conjunction_v<
        // lacking concepts and ranges, we accept any T whose std::data yields a suitable pointer ...
        IsCompatiblePointer8<decltype(std::data(std::declval<const T &>()))>,
        // ... and that has a suitable size ...
        std::is_convertible<
            decltype(std::size(std::declval<const T &>())),
            qsizetype
        >,
        // ... and it's a range as it defines an iterator-like API
        IsCompatibleChar8Type<typename std::iterator_traits<
            decltype(std::begin(std::declval<const T &>()))>::value_type
        >,
        std::is_convertible<
            decltype( std::begin(std::declval<const T &>()) != std::end(std::declval<const T &>()) ),
            bool
        >,

        // This needs to be treated specially due to the empty vs null distinction
        std::negation<std::is_same<std::decay_t<T>, QByteArray>>,

        // This has a compatible value_type, but explicitly a different encoding
        std::negation<std::is_same<std::decay_t<T>, QLatin1StringView>>,

        // Don't make an accidental copy constructor
        std::negation<std::disjunction<
            std::is_same<std::decay_t<T>, QBasicUtf8StringView<true>>,
            std::is_same<std::decay_t<T>, QBasicUtf8StringView<false>>
        >>
    >>> : std::true_type {};

struct hide_char8_t {
#ifdef __cpp_char8_t
    using type = char8_t;
#endif
};

struct wrap_char { using type = char; };

} // namespace QtPrivate

#ifdef Q_QDOC
#define QBasicUtf8StringView QUtf8StringView
#else
template <bool UseChar8T>
#endif
class QBasicUtf8StringView
{
public:
#ifndef Q_QDOC
    using storage_type = typename std::conditional<UseChar8T,
            QtPrivate::hide_char8_t,
            QtPrivate::wrap_char
        >::type::type;
#else
    using storage_type = typename QtPrivate::hide_char8_t;
#endif
    typedef const storage_type value_type;
    typedef qptrdiff difference_type;
    typedef qsizetype size_type;
    typedef value_type &reference;
    typedef value_type &const_reference;
    typedef value_type *pointer;
    typedef value_type *const_pointer;

    typedef pointer iterator;
    typedef const_pointer const_iterator;
    typedef std::reverse_iterator<iterator> reverse_iterator;
    typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

private:
    template <typename Char>
    using if_compatible_char = std::enable_if_t<QtPrivate::IsCompatibleChar8Type<Char>::value, bool>;

    template <typename Pointer>
    using if_compatible_pointer = std::enable_if_t<QtPrivate::IsCompatiblePointer8<Pointer>::value, bool>;

    template <typename T>
    using if_compatible_qstring_like = std::enable_if_t<std::is_same_v<T, QByteArray>, bool>;

    template <typename T>
    using if_compatible_container = std::enable_if_t<QtPrivate::IsContainerCompatibleWithQUtf8StringView<T>::value, bool>;

    template <typename Container>
    static constexpr qsizetype lengthHelperContainer(const Container &c) noexcept
    {
        return qsizetype(std::size(c));
    }

    // Note: Do not replace with std::size(const Char (&)[N]), because the result
    // will be of by one.
    template <typename Char, size_t N>
    static constexpr qsizetype lengthHelperContainer(const Char (&str)[N]) noexcept
    {
        return QtPrivate::lengthHelperContainer(str);
    }

    template <typename Char>
    static const storage_type *castHelper(const Char *str) noexcept
    { return reinterpret_cast<const storage_type*>(str); }
    static constexpr const storage_type *castHelper(const storage_type *str) noexcept
    { return str; }

public:
    constexpr QBasicUtf8StringView() noexcept
        : m_data(nullptr), m_size(0) {}
    constexpr QBasicUtf8StringView(std::nullptr_t) noexcept
        : QBasicUtf8StringView() {}

    template <typename Char, if_compatible_char<Char> = true>
    constexpr QBasicUtf8StringView(const Char *str, qsizetype len)
        : m_data(castHelper(str)),
          m_size((Q_ASSERT(len >= 0), Q_ASSERT(str || !len), len)) {}

    template <typename Char, if_compatible_char<Char> = true>
    constexpr QBasicUtf8StringView(const Char *f, const Char *l)
        : QBasicUtf8StringView(f, l - f) {}

#ifdef Q_QDOC
    template <typename Char, size_t N>
    constexpr QBasicUtf8StringView(const Char (&array)[N]) noexcept;

    template <typename Char>
    constexpr QBasicUtf8StringView(const Char *str) noexcept;
#else
    template <typename Pointer, if_compatible_pointer<Pointer> = true>
    constexpr QBasicUtf8StringView(const Pointer &str) noexcept
        : QBasicUtf8StringView(str, QtPrivate::lengthHelperPointer(str)) {}

    template <typename Char, if_compatible_char<Char> = true>
    constexpr QBasicUtf8StringView(const Char (&str)[]) noexcept
        : QBasicUtf8StringView(&*str) {} // decay to pointer
#endif

#ifdef Q_QDOC
    QBasicUtf8StringView(const QByteArray &str) noexcept;
    constexpr QBasicUtf8StringView(const storage_type *d, qsizetype n) noexcept {};
#else
    template <typename String, if_compatible_qstring_like<String> = true>
    QBasicUtf8StringView(const String &str) noexcept
        : QBasicUtf8StringView{str.begin(), str.size()} {}
#endif

    template <typename Container, if_compatible_container<Container> = true>
    constexpr QBasicUtf8StringView(const Container &c) noexcept
        : QBasicUtf8StringView(std::data(c), lengthHelperContainer(c)) {}

#if defined(__cpp_char8_t) && !defined(Q_QDOC)
    constexpr QBasicUtf8StringView(QBasicUtf8StringView<!UseChar8T> other)
        : QBasicUtf8StringView(other.data(), other.size()) {}
#endif

    template <typename Char, size_t Size, if_compatible_char<Char> = true>
    [[nodiscard]] constexpr static QBasicUtf8StringView fromArray(const Char (&string)[Size]) noexcept
    { return QBasicUtf8StringView(string, Size); }

    [[nodiscard]] inline QString toString() const; // defined in qstring.h

    [[nodiscard]] constexpr qsizetype size() const noexcept { return m_size; }
    [[nodiscard]] constexpr const_pointer data() const noexcept { return m_data; }
#ifdef __cpp_char8_t
    [[nodiscard]] const char8_t *utf8() const noexcept { return reinterpret_cast<const char8_t*>(m_data); }
#endif

    [[nodiscard]] constexpr storage_type operator[](qsizetype n) const
    { verify(n, 1); return m_data[n]; }

    //
    // QString API
    //

    [[nodiscard]] constexpr storage_type at(qsizetype n) const { return (*this)[n]; }

    template <typename...Args>
    [[nodiscard]] inline QString arg(Args &&...args) const;

    [[nodiscard]]
    constexpr QBasicUtf8StringView mid(qsizetype pos, qsizetype n = -1) const
    {
        using namespace QtPrivate;
        auto result = QContainerImplHelper::mid(size(), &pos, &n);
        return result == QContainerImplHelper::Null ? QBasicUtf8StringView() : QBasicUtf8StringView(m_data + pos, n);
    }
    [[nodiscard]]
    constexpr QBasicUtf8StringView left(qsizetype n) const
    {
        if (size_t(n) >= size_t(size()))
            n = size();
        return QBasicUtf8StringView(m_data, n);
    }
    [[nodiscard]]
    constexpr QBasicUtf8StringView right(qsizetype n) const
    {
        if (size_t(n) >= size_t(size()))
            n = size();
        return QBasicUtf8StringView(m_data + m_size - n, n);
    }

    [[nodiscard]] constexpr QBasicUtf8StringView sliced(qsizetype pos) const
    { verify(pos, 0); return QBasicUtf8StringView{m_data + pos, m_size - pos}; }
    [[nodiscard]] constexpr QBasicUtf8StringView sliced(qsizetype pos, qsizetype n) const
    { verify(pos, n); return QBasicUtf8StringView(m_data + pos, n); }
    [[nodiscard]] constexpr QBasicUtf8StringView first(qsizetype n) const
    { verify(0, n); return sliced(0, n); }
    [[nodiscard]] constexpr QBasicUtf8StringView last(qsizetype n) const
    { verify(0, n); return sliced(m_size - n, n); }
    [[nodiscard]] constexpr QBasicUtf8StringView chopped(qsizetype n) const
    { verify(0, n); return sliced(0, m_size - n); }

    constexpr QBasicUtf8StringView &slice(qsizetype pos)
    { *this = sliced(pos); return *this; }
    constexpr QBasicUtf8StringView &slice(qsizetype pos, qsizetype n)
    { *this = sliced(pos, n); return *this; }

    constexpr void truncate(qsizetype n)
    { verify(0, n); m_size = n; }
    constexpr void chop(qsizetype n)
    { verify(0, n); m_size -= n; }

    [[nodiscard]] inline bool isValidUtf8() const noexcept
    {
        return QByteArrayView(reinterpret_cast<const char *>(data()), size()).isValidUtf8();
    }

    //
    // STL compatibility API:
    //
    [[nodiscard]] const_iterator begin()   const noexcept { return data(); }
    [[nodiscard]] const_iterator end()     const noexcept { return data() + size(); }
    [[nodiscard]] const_iterator cbegin()  const noexcept { return begin(); }
    [[nodiscard]] const_iterator cend()    const noexcept { return end(); }
    [[nodiscard]] const_reverse_iterator rbegin()  const noexcept { return const_reverse_iterator(end()); }
    [[nodiscard]] const_reverse_iterator rend()    const noexcept { return const_reverse_iterator(begin()); }
    [[nodiscard]] const_reverse_iterator crbegin() const noexcept { return rbegin(); }
    [[nodiscard]] const_reverse_iterator crend()   const noexcept { return rend(); }

    [[nodiscard]] constexpr bool empty() const noexcept { return size() == 0; }
    [[nodiscard]] constexpr storage_type front() const { return Q_ASSERT(!empty()), m_data[0]; }
    [[nodiscard]] constexpr storage_type back()  const { return Q_ASSERT(!empty()), m_data[m_size - 1]; }

    [[nodiscard]] Q_IMPLICIT operator std::string_view() const noexcept
    { return std::string_view{reinterpret_cast<const char*>(data()), size_t(size())}; }

#ifdef __cpp_lib_char8_t
    [[nodiscard]] Q_IMPLICIT operator std::u8string_view() const noexcept
    { return std::u8string_view{utf8(), size_t(size())}; }
#endif

    [[nodiscard]] constexpr qsizetype max_size() const noexcept { return maxSize(); }

    //
    // Qt compatibility API:
    //
    [[nodiscard]] constexpr bool isNull() const noexcept { return !m_data; }
    [[nodiscard]] constexpr bool isEmpty() const noexcept { return empty(); }
    [[nodiscard]] constexpr qsizetype length() const noexcept
    { return size(); }

    [[nodiscard]] int compare(QBasicUtf8StringView other,
                              Qt::CaseSensitivity cs = Qt::CaseSensitive) const noexcept
    {
        return QtPrivate::compareStrings(*this, other, cs);
    }

    // all defined in qstring.h
    [[nodiscard]] inline int compare(QChar other,
                                     Qt::CaseSensitivity cs = Qt::CaseSensitive) const noexcept;
    [[nodiscard]] inline int compare(QStringView other,
                                     Qt::CaseSensitivity cs = Qt::CaseSensitive) const noexcept;
    [[nodiscard]] inline int compare(QLatin1StringView other,
                                     Qt::CaseSensitivity cs = Qt::CaseSensitive) const noexcept;
    [[nodiscard]] inline int compare(const QByteArray &other,
                                     Qt::CaseSensitivity cs = Qt::CaseSensitive) const noexcept;

    [[nodiscard]] inline bool equal(QChar other) const noexcept;
    [[nodiscard]] inline bool equal(QStringView other) const noexcept;
    [[nodiscard]] inline bool equal(QLatin1StringView other) const noexcept;
    [[nodiscard]] inline bool equal(const QByteArray &other) const noexcept;
    // end defined in qstring.h

    [[nodiscard]] static constexpr qsizetype maxSize() noexcept
    {
        // -1 to deal with the pointer one-past-the-end;
        return QtPrivate::MaxAllocSize - 1;
    }

private:
    [[nodiscard]] static inline int compare(QBasicUtf8StringView lhs, QBasicUtf8StringView rhs) noexcept
    {
        return QtPrivate::compareStrings(QBasicUtf8StringView<false>(lhs.data(), lhs.size()),
                                         QBasicUtf8StringView<false>(rhs.data(), rhs.size()));
    }

    friend bool
    comparesEqual(const QBasicUtf8StringView &lhs, const QBasicUtf8StringView &rhs) noexcept
    {
        return lhs.size() == rhs.size()
                && QtPrivate::equalStrings(QBasicUtf8StringView<false>(lhs.data(), lhs.size()),
                                           QBasicUtf8StringView<false>(rhs.data(), rhs.size()));
    }
    friend Qt::strong_ordering
    compareThreeWay(const QBasicUtf8StringView &lhs, const QBasicUtf8StringView &rhs) noexcept
    {
        const int res = QBasicUtf8StringView::compare(lhs, rhs);
        return Qt::compareThreeWay(res, 0);
    }
    Q_DECLARE_STRONGLY_ORDERED(QBasicUtf8StringView)

    friend bool
    comparesEqual(const QBasicUtf8StringView &lhs, const QLatin1StringView &rhs) noexcept
    {
        return lhs.equal(rhs);
    }
    friend Qt::strong_ordering
    compareThreeWay(const QBasicUtf8StringView &lhs, const QLatin1StringView &rhs) noexcept
    {
        const int res = lhs.compare(rhs);
        return Qt::compareThreeWay(res, 0);
    }
    Q_DECLARE_STRONGLY_ORDERED(QBasicUtf8StringView, QLatin1StringView)

    friend bool
    comparesEqual(const QBasicUtf8StringView &lhs, const QStringView &rhs) noexcept
    { return lhs.equal(rhs); }
    friend Qt::strong_ordering
    compareThreeWay(const QBasicUtf8StringView &lhs, const QStringView &rhs) noexcept
    {
        const int res = lhs.compare(rhs);
        return Qt::compareThreeWay(res, 0);
    }
    Q_DECLARE_STRONGLY_ORDERED(QBasicUtf8StringView, QStringView)

    friend bool comparesEqual(const QBasicUtf8StringView &lhs, const QChar &rhs) noexcept
    { return lhs.equal(rhs); }
    friend Qt::strong_ordering
    compareThreeWay(const QBasicUtf8StringView &lhs, const QChar &rhs) noexcept
    {
        const int res = lhs.compare(rhs);
        return Qt::compareThreeWay(res, 0);
    }
    Q_DECLARE_STRONGLY_ORDERED(QBasicUtf8StringView, QChar)
    Q_DECLARE_STRONGLY_ORDERED(QBasicUtf8StringView, char16_t)

#if !defined(QT_NO_CAST_FROM_ASCII) && !defined(QT_RESTRICTED_CAST_FROM_ASCII)
    friend bool
    comparesEqual(const QBasicUtf8StringView &lhs, const QByteArrayView &rhs) noexcept
    {
        return lhs.size() == rhs.size()
                && QtPrivate::equalStrings(QBasicUtf8StringView<false>(lhs.data(), lhs.size()),
                                           QBasicUtf8StringView<false>(rhs.data(), rhs.size()));
    }
    friend Qt::strong_ordering
    compareThreeWay(const QBasicUtf8StringView &lhs, const QByteArrayView &rhs) noexcept
    {
        const int res = QtPrivate::compareStrings(QBasicUtf8StringView<false>(lhs.data(), lhs.size()),
                                                  QBasicUtf8StringView<false>(rhs.data(), rhs.size()));
        return Qt::compareThreeWay(res, 0);
    }
    Q_DECLARE_STRONGLY_ORDERED(QBasicUtf8StringView, QByteArrayView, QT_ASCII_CAST_WARN)

    friend bool
    comparesEqual(const QBasicUtf8StringView &lhs, const QByteArray &rhs) noexcept
    {
        return lhs.equal(rhs);
    }
    friend Qt::strong_ordering
    compareThreeWay(const QBasicUtf8StringView &lhs, const QByteArray &rhs) noexcept
    {
        const int res = lhs.compare(rhs);
        return Qt::compareThreeWay(res, 0);
    }
    Q_DECLARE_STRONGLY_ORDERED(QBasicUtf8StringView, QByteArray, QT_ASCII_CAST_WARN)

    friend bool comparesEqual(const QBasicUtf8StringView &lhs, const char *rhs) noexcept
    { return comparesEqual(lhs, QByteArrayView(rhs)); }
    friend Qt::strong_ordering
    compareThreeWay(const QBasicUtf8StringView &lhs, const char *rhs) noexcept
    { return compareThreeWay(lhs, QByteArrayView(rhs)); }
    Q_DECLARE_STRONGLY_ORDERED(QBasicUtf8StringView, const char *, QT_ASCII_CAST_WARN)
#endif // !defined(QT_NO_CAST_FROM_ASCII) && !defined(QT_RESTRICTED_CAST_FROM_ASCII)

    Q_ALWAYS_INLINE constexpr void verify([[maybe_unused]] qsizetype pos = 0,
                                          [[maybe_unused]] qsizetype n = 1) const
    {
        Q_ASSERT(pos >= 0);
        Q_ASSERT(pos <= size());
        Q_ASSERT(n >= 0);
        Q_ASSERT(n <= size() - pos);
    }
    const storage_type *m_data;
    qsizetype m_size;
};

#ifdef Q_QDOC
#undef QBasicUtf8StringView
#else
template <bool UseChar8T>
Q_DECLARE_TYPEINFO_BODY(QBasicUtf8StringView<UseChar8T>, Q_PRIMITIVE_TYPE);

template <typename QStringLike, std::enable_if_t<std::is_same_v<QStringLike, QByteArray>, bool> = true>
[[nodiscard]] inline q_no_char8_t::QUtf8StringView qToUtf8StringViewIgnoringNull(const QStringLike &s) noexcept
{ return q_no_char8_t::QUtf8StringView(s.begin(), s.size()); }
#endif // Q_QDOC

QT_END_NAMESPACE

#endif /* QUTF8STRINGVIEW_H */
