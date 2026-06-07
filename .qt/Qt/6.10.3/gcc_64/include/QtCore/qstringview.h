// Copyright (C) 2020 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Marc Mutz <marc.mutz@kdab.com>
// Copyright (C) 2019 Mail.ru Group.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:data-parser
#ifndef QSTRINGVIEW_H
#define QSTRINGVIEW_H

#include <QtCore/qchar.h>
#include <QtCore/qcompare.h>
#include <QtCore/qcontainerfwd.h>
#include <QtCore/qbytearray.h>
#include <QtCore/qstringfwd.h>
#include <QtCore/qstringalgorithms.h>

#include <string>
#include <string_view>
#include <QtCore/q20type_traits.h>

#if defined(Q_OS_DARWIN) || defined(Q_QDOC)
Q_FORWARD_DECLARE_CF_TYPE(CFString);
Q_FORWARD_DECLARE_OBJC_CLASS(NSString);
#endif

QT_BEGIN_NAMESPACE

class QRegularExpression;
class QRegularExpressionMatch;

namespace QtPrivate {
template <typename Char>
struct IsCompatibleCharTypeHelper
    : std::integral_constant<bool,
                             std::is_same<Char, QChar>::value ||
                             std::is_same<Char, ushort>::value ||
                             std::is_same<Char, char16_t>::value ||
                             (std::is_same<Char, wchar_t>::value && sizeof(wchar_t) == sizeof(QChar))> {};
template <typename Char>
struct IsCompatibleCharType
    : IsCompatibleCharTypeHelper<q20::remove_cvref_t<Char>> {};

template <typename Pointer>
struct IsCompatiblePointerHelper : std::false_type {};
template <typename Char>
struct IsCompatiblePointerHelper<Char*>
    : IsCompatibleCharType<Char> {};
template <typename Pointer>
struct IsCompatiblePointer
    : IsCompatiblePointerHelper<q20::remove_cvref_t<Pointer>> {};

template <typename T, typename Enable = void>
struct IsContainerCompatibleWithQStringView : std::false_type {};

template <typename T>
struct IsContainerCompatibleWithQStringView<T, std::enable_if_t<std::conjunction_v<
            // lacking concepts and ranges, we accept any T whose std::data yields a suitable pointer ...
            IsCompatiblePointer<decltype( std::data(std::declval<const T &>()) )>,
            // ... and that has a suitable size ...
            std::is_convertible<decltype( std::size(std::declval<const T &>()) ), qsizetype>,
            // ... and it's a range as it defines an iterator-like API
            IsCompatibleCharType<typename std::iterator_traits<decltype( std::begin(std::declval<const T &>()) )>::value_type>,
            std::is_convertible<
                decltype( std::begin(std::declval<const T &>()) != std::end(std::declval<const T &>()) ),
                bool>,

            // These need to be treated specially due to the empty vs null distinction
            std::negation<std::is_same<std::decay_t<T>, QString>>,
#define QSTRINGVIEW_REFUSES_QSTRINGREF 1
            std::negation<std::is_same<q20::remove_cvref_t<T>, QStringRef>>, // QStringRef::op QStringView()

            // Don't make an accidental copy constructor
            std::negation<std::is_same<std::decay_t<T>, QStringView>>
        >>> : std::true_type {};

} // namespace QtPrivate

class QStringView
{
public:
    typedef char16_t storage_type;
    typedef const QChar value_type;
    typedef std::ptrdiff_t difference_type;
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
    using if_compatible_char = typename std::enable_if<QtPrivate::IsCompatibleCharType<Char>::value, bool>::type;

    template <typename Pointer>
    using if_compatible_pointer = typename std::enable_if<QtPrivate::IsCompatiblePointer<Pointer>::value, bool>::type;

    template <typename T>
    using if_compatible_qstring_like = typename std::enable_if<std::is_same<T, QString>::value, bool>::type;

    template <typename T>
    using if_compatible_container = typename std::enable_if<QtPrivate::IsContainerCompatibleWithQStringView<T>::value, bool>::type;

    template <typename Char>
    static constexpr qsizetype lengthHelperPointer(const Char *str) noexcept
    {
        if (q20::is_constant_evaluated())
            return QtPrivate::lengthHelperPointer(str);
        return QtPrivate::qustrlen(reinterpret_cast<const char16_t *>(str));
    }
    static qsizetype lengthHelperPointer(const QChar *str) noexcept
    {
        return QtPrivate::qustrlen(reinterpret_cast<const char16_t *>(str));
    }

    template <typename Char>
    static const storage_type *castHelper(const Char *str) noexcept
    { return reinterpret_cast<const storage_type*>(str); }
    static constexpr const storage_type *castHelper(const storage_type *str) noexcept
    { return str; }

public:
    constexpr QStringView() noexcept {}
    constexpr QStringView(std::nullptr_t) noexcept
        : QStringView() {}

    template <typename Char, if_compatible_char<Char> = true>
    constexpr QStringView(const Char *str, qsizetype len)
#if QT_VERSION >= QT_VERSION_CHECK(7, 0, 0) || defined(QT_BOOTSTRAPPED)
        : m_data(castHelper(str)),
          m_size((Q_PRE(len >= 0), Q_PRE(str || !len), len))
#else
        : m_size((Q_PRE(len >= 0), Q_PRE(str || !len), len)),
          m_data(castHelper(str))
#endif
    {}

    template <typename Char, if_compatible_char<Char> = true>
    constexpr QStringView(const Char *f, const Char *l)
        : QStringView(f, l - f) {}

#ifdef Q_QDOC
    template <typename Char, size_t N>
    constexpr QStringView(const Char (&array)[N]) noexcept;

    template <typename Char>
    constexpr QStringView(const Char *str) noexcept;
#else
    template <typename Pointer, if_compatible_pointer<Pointer> = true>
    constexpr QStringView(const Pointer &str) noexcept
        : QStringView(str, str ? lengthHelperPointer(str) : 0) {}

    template <typename Char, if_compatible_char<Char> = true>
    constexpr QStringView(const Char (&str)[]) noexcept // array of unknown bounds
        : QStringView{&*str} {} // decay to pointer
#endif

#ifdef Q_QDOC
    QStringView(const QString &str) noexcept;
#else
    template <typename String, if_compatible_qstring_like<String> = true>
    QStringView(const String &str) noexcept
        : QStringView{str.begin(), str.size()} {}
#endif

    template <typename Container, if_compatible_container<Container> = true>
    Q_ALWAYS_INLINE constexpr QStringView(const Container &c) noexcept
        : QStringView(std::data(c), QtPrivate::lengthHelperContainer(c)) {}

    template <typename Char, size_t Size, if_compatible_char<Char> = true>
    [[nodiscard]] constexpr static QStringView fromArray(const Char (&string)[Size]) noexcept
    { return QStringView(string, Size); }

    [[nodiscard]] inline QString toString() const; // defined in qstring.h
#if defined(Q_OS_DARWIN) || defined(Q_QDOC)
    // defined in qcore_foundation.mm
    [[nodiscard]] Q_CORE_EXPORT CFStringRef toCFString() const Q_DECL_CF_RETURNS_RETAINED;
    [[nodiscard]] Q_CORE_EXPORT NSString *toNSString() const Q_DECL_NS_RETURNS_AUTORELEASED;
#endif

    [[nodiscard]] constexpr qsizetype size() const noexcept { return m_size; }
    [[nodiscard]] const_pointer data() const noexcept { return reinterpret_cast<const_pointer>(m_data); }
    [[nodiscard]] const_pointer constData() const noexcept { return data(); }
    [[nodiscard]] constexpr const storage_type *utf16() const noexcept { return m_data; }

    [[nodiscard]] constexpr QChar operator[](qsizetype n) const
    { verify(n, 1); return QChar(m_data[n]); }

    //
    // QString API
    //

    template <typename...Args>
    [[nodiscard]] inline QString arg(Args &&...args) const; // defined in qstring.h

    [[nodiscard]] QByteArray toLatin1() const { return QtPrivate::convertToLatin1(*this); }
    [[nodiscard]] QByteArray toUtf8() const { return QtPrivate::convertToUtf8(*this); }
    [[nodiscard]] QByteArray toLocal8Bit() const { return QtPrivate::convertToLocal8Bit(*this); }
    [[nodiscard]] inline QList<uint> toUcs4() const; // defined in qlist.h ### Qt 7 char32_t

    [[nodiscard]] constexpr QChar at(qsizetype n) const noexcept { return (*this)[n]; }

    [[nodiscard]] constexpr QStringView mid(qsizetype pos, qsizetype n = -1) const noexcept
    {
        using namespace QtPrivate;
        auto result = QContainerImplHelper::mid(size(), &pos, &n);
        return result == QContainerImplHelper::Null ? QStringView() : QStringView(m_data + pos, n);
    }
    [[nodiscard]] constexpr QStringView left(qsizetype n) const noexcept
    {
        if (size_t(n) >= size_t(size()))
            n = size();
        return QStringView(m_data, n);
    }
    [[nodiscard]] constexpr QStringView right(qsizetype n) const noexcept
    {
        if (size_t(n) >= size_t(size()))
            n = size();
        return QStringView(m_data + m_size - n, n);
    }

    [[nodiscard]] constexpr QStringView first(qsizetype n) const noexcept
    { verify(0, n); return sliced(0, n); }
    [[nodiscard]] constexpr QStringView last(qsizetype n) const noexcept
    { verify(0, n); return sliced(size() - n, n); }
    [[nodiscard]] constexpr QStringView sliced(qsizetype pos) const noexcept
    { verify(pos, 0); return QStringView(m_data + pos, size() - pos); }
    [[nodiscard]] constexpr QStringView sliced(qsizetype pos, qsizetype n) const noexcept
    { verify(pos, n); return QStringView(m_data + pos, n); }
    [[nodiscard]] constexpr QStringView chopped(qsizetype n) const noexcept
    { verify(0, n); return sliced(0, m_size - n); }

    constexpr void truncate(qsizetype n) noexcept
    { verify(0, n); ; m_size = n; }
    constexpr void chop(qsizetype n) noexcept
    { verify(0, n); m_size -= n; }

    [[nodiscard]] QStringView trimmed() const noexcept { return QtPrivate::trimmed(*this); }

    constexpr QStringView &slice(qsizetype pos)
    { *this = sliced(pos); return *this; }
    constexpr QStringView &slice(qsizetype pos, qsizetype n)
    { *this = sliced(pos, n); return *this; }

    template <typename Needle, typename...Flags>
    [[nodiscard]] constexpr inline auto tokenize(Needle &&needle, Flags...flags) const
        noexcept(noexcept(qTokenize(std::declval<const QStringView&>(), std::forward<Needle>(needle), flags...)))
            -> decltype(qTokenize(*this, std::forward<Needle>(needle), flags...))
    { return qTokenize(*this, std::forward<Needle>(needle), flags...); }

    [[nodiscard]] int compare(QStringView other, Qt::CaseSensitivity cs = Qt::CaseSensitive) const noexcept
    { return QtPrivate::compareStrings(*this, other, cs); }
    [[nodiscard]] inline int compare(QLatin1StringView other, Qt::CaseSensitivity cs = Qt::CaseSensitive) const noexcept;
    [[nodiscard]] inline int compare(QUtf8StringView other, Qt::CaseSensitivity cs = Qt::CaseSensitive) const noexcept;
    [[nodiscard]] constexpr int compare(QChar c) const noexcept
    { return size() >= 1 ? compare_single_char_helper(*utf16() - c.unicode()) : -1; }
    [[nodiscard]] int compare(QChar c, Qt::CaseSensitivity cs) const noexcept
    { return QtPrivate::compareStrings(*this, QStringView(&c, 1), cs); }

    [[nodiscard]] inline int localeAwareCompare(QStringView other) const;

    [[nodiscard]] bool startsWith(QStringView s, Qt::CaseSensitivity cs = Qt::CaseSensitive) const noexcept
    { return QtPrivate::startsWith(*this, s, cs); }
    [[nodiscard]] inline bool startsWith(QLatin1StringView s, Qt::CaseSensitivity cs = Qt::CaseSensitive) const noexcept;
    [[nodiscard]] bool startsWith(QChar c) const noexcept
    { return !empty() && front() == c; }
    [[nodiscard]] bool startsWith(QChar c, Qt::CaseSensitivity cs) const noexcept
    { return QtPrivate::startsWith(*this, QStringView(&c, 1), cs); }

    [[nodiscard]] bool endsWith(QStringView s, Qt::CaseSensitivity cs = Qt::CaseSensitive) const noexcept
    { return QtPrivate::endsWith(*this, s, cs); }
    [[nodiscard]] inline bool endsWith(QLatin1StringView s, Qt::CaseSensitivity cs = Qt::CaseSensitive) const noexcept;
    [[nodiscard]] bool endsWith(QChar c) const noexcept
    { return !empty() && back() == c; }
    [[nodiscard]] bool endsWith(QChar c, Qt::CaseSensitivity cs) const noexcept
    { return QtPrivate::endsWith(*this, QStringView(&c, 1), cs); }

    [[nodiscard]] qsizetype indexOf(QChar c, qsizetype from = 0, Qt::CaseSensitivity cs = Qt::CaseSensitive) const noexcept
    { return QtPrivate::findString(*this, from, c.unicode(), cs); }
    [[nodiscard]] qsizetype indexOf(QStringView s, qsizetype from = 0, Qt::CaseSensitivity cs = Qt::CaseSensitive) const noexcept
    { return QtPrivate::findString(*this, from, s, cs); }
    [[nodiscard]] inline qsizetype indexOf(QLatin1StringView s, qsizetype from = 0, Qt::CaseSensitivity cs = Qt::CaseSensitive) const noexcept;

    [[nodiscard]] bool contains(QChar c, Qt::CaseSensitivity cs = Qt::CaseSensitive) const noexcept
    { return indexOf(c, 0, cs) != qsizetype(-1); }
    [[nodiscard]] bool contains(QStringView s, Qt::CaseSensitivity cs = Qt::CaseSensitive) const noexcept
    { return indexOf(s, 0, cs) != qsizetype(-1); }
    [[nodiscard]] inline bool contains(QLatin1StringView s, Qt::CaseSensitivity cs = Qt::CaseSensitive) const noexcept;

    [[nodiscard]] qsizetype count(QChar c, Qt::CaseSensitivity cs = Qt::CaseSensitive) const noexcept
    { return QtPrivate::count(*this, c, cs); }
    [[nodiscard]] qsizetype count(QStringView s, Qt::CaseSensitivity cs = Qt::CaseSensitive) const noexcept
    { return QtPrivate::count(*this, s, cs); }
    [[nodiscard]] inline qsizetype count(QLatin1StringView s, Qt::CaseSensitivity cs = Qt::CaseSensitive) const;

    [[nodiscard]] qsizetype lastIndexOf(QChar c, Qt::CaseSensitivity cs = Qt::CaseSensitive) const noexcept
    { return lastIndexOf(c, -1, cs); }
    [[nodiscard]] qsizetype lastIndexOf(QChar c, qsizetype from, Qt::CaseSensitivity cs = Qt::CaseSensitive) const noexcept
    { return QtPrivate::lastIndexOf(*this, from, c.unicode(), cs); }
    [[nodiscard]] qsizetype lastIndexOf(QStringView s, Qt::CaseSensitivity cs = Qt::CaseSensitive) const noexcept
    { return lastIndexOf(s, size(), cs); }
    [[nodiscard]] qsizetype lastIndexOf(QStringView s, qsizetype from, Qt::CaseSensitivity cs = Qt::CaseSensitive) const noexcept
    { return QtPrivate::lastIndexOf(*this, from, s, cs); }
    [[nodiscard]] inline qsizetype lastIndexOf(QLatin1StringView s, Qt::CaseSensitivity cs = Qt::CaseSensitive) const noexcept;
    [[nodiscard]] inline qsizetype lastIndexOf(QLatin1StringView s, qsizetype from, Qt::CaseSensitivity cs = Qt::CaseSensitive) const noexcept;

#if QT_CONFIG(regularexpression)
    [[nodiscard]] qsizetype indexOf(const QRegularExpression &re, qsizetype from = 0, QRegularExpressionMatch *rmatch = nullptr) const
    {
        return QtPrivate::indexOf(*this, re, from, rmatch);
    }
#ifdef Q_QDOC
    [[nodiscard]] qsizetype lastIndexOf(const QRegularExpression &re, QRegularExpressionMatch *rmatch = nullptr) const;
#else
    // prevent an ambiguity when called like this: lastIndexOf(re, 0)
    template <typename T = QRegularExpressionMatch, std::enable_if_t<std::is_same_v<T, QRegularExpressionMatch>, bool> = false>
    [[nodiscard]] qsizetype lastIndexOf(const QRegularExpression &re, T *rmatch = nullptr) const
    {
        return QtPrivate::lastIndexOf(*this, re, size(), rmatch);
    }
#endif
    [[nodiscard]] qsizetype lastIndexOf(const QRegularExpression &re, qsizetype from, QRegularExpressionMatch *rmatch = nullptr) const
    {
        return QtPrivate::lastIndexOf(*this, re, from, rmatch);
    }
    [[nodiscard]] bool contains(const QRegularExpression &re, QRegularExpressionMatch *rmatch = nullptr) const
    {
        return QtPrivate::contains(*this, re, rmatch);
    }
    [[nodiscard]] qsizetype count(const QRegularExpression &re) const
    {
        return QtPrivate::count(*this, re);
    }
#endif

    [[nodiscard]] bool isRightToLeft() const noexcept
    { return QtPrivate::isRightToLeft(*this); }
    [[nodiscard]] bool isValidUtf16() const noexcept
    { return QtPrivate::isValidUtf16(*this); }

    [[nodiscard]] bool isUpper() const noexcept
    { return QtPrivate::isUpper(*this); }
    [[nodiscard]] bool isLower() const noexcept
    { return QtPrivate::isLower(*this); }

    [[nodiscard]] inline short toShort(bool *ok = nullptr, int base = 10) const;
    [[nodiscard]] inline ushort toUShort(bool *ok = nullptr, int base = 10) const;
    [[nodiscard]] inline int toInt(bool *ok = nullptr, int base = 10) const;
    [[nodiscard]] inline uint toUInt(bool *ok = nullptr, int base = 10) const;
    [[nodiscard]] inline long toLong(bool *ok = nullptr, int base = 10) const;
    [[nodiscard]] inline ulong toULong(bool *ok = nullptr, int base = 10) const;
    [[nodiscard]] inline qlonglong toLongLong(bool *ok = nullptr, int base = 10) const;
    [[nodiscard]] inline qulonglong toULongLong(bool *ok = nullptr, int base = 10) const;
    [[nodiscard]] Q_CORE_EXPORT float toFloat(bool *ok = nullptr) const;
    [[nodiscard]] Q_CORE_EXPORT double toDouble(bool *ok = nullptr) const;

    [[nodiscard]] inline qsizetype toWCharArray(wchar_t *array) const; // defined in qstring.h


    [[nodiscard]] Q_CORE_EXPORT
    QList<QStringView> split(QStringView sep,
                             Qt::SplitBehavior behavior = Qt::KeepEmptyParts,
                             Qt::CaseSensitivity cs = Qt::CaseSensitive) const;
    [[nodiscard]] Q_CORE_EXPORT
    QList<QStringView> split(QChar sep, Qt::SplitBehavior behavior = Qt::KeepEmptyParts,
                             Qt::CaseSensitivity cs = Qt::CaseSensitive) const;

#if QT_CONFIG(regularexpression)
    [[nodiscard]] Q_CORE_EXPORT
    QList<QStringView> split(const QRegularExpression &sep,
                             Qt::SplitBehavior behavior = Qt::KeepEmptyParts) const;
#endif

    // QStringView <> QStringView
    friend bool comparesEqual(const QStringView &lhs, const QStringView &rhs) noexcept
    { return lhs.size() == rhs.size() && QtPrivate::equalStrings(lhs, rhs); }
    friend Qt::strong_ordering
    compareThreeWay(const QStringView &lhs, const QStringView &rhs) noexcept
    {
        const int res = QtPrivate::compareStrings(lhs, rhs);
        return Qt::compareThreeWay(res, 0);
    }
    Q_DECLARE_STRONGLY_ORDERED(QStringView)

    // QStringView <> QChar
    friend bool comparesEqual(const QStringView &lhs, QChar rhs) noexcept
    { return lhs.size() == 1 && lhs[0] == rhs; }
    friend Qt::strong_ordering compareThreeWay(const QStringView &lhs, QChar rhs) noexcept
    { return compareThreeWay(lhs, QStringView(&rhs, 1)); }
    Q_DECLARE_STRONGLY_ORDERED(QStringView, QChar)

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
    [[nodiscard]] constexpr QChar front() const { return Q_PRE(!empty()), QChar(m_data[0]); }
    [[nodiscard]] constexpr QChar back()  const { return Q_PRE(!empty()), QChar(m_data[m_size - 1]); }

    [[nodiscard]] constexpr Q_IMPLICIT operator std::u16string_view() const noexcept
    { return std::u16string_view(m_data, size_t(m_size)); }

    [[nodiscard]] constexpr qsizetype max_size() const noexcept { return maxSize(); }

    //
    // Qt compatibility API:
    //
    [[nodiscard]] const_iterator constBegin() const noexcept { return begin(); }
    [[nodiscard]] const_iterator constEnd() const noexcept { return end(); }
    [[nodiscard]] constexpr bool isNull() const noexcept { return !m_data; }
    [[nodiscard]] constexpr bool isEmpty() const noexcept { return empty(); }
    [[nodiscard]] constexpr qsizetype length() const noexcept
    { return size(); }
    [[nodiscard]] constexpr QChar first() const { return front(); }
    [[nodiscard]] constexpr QChar last()  const { return back(); }

    [[nodiscard]] static constexpr qsizetype maxSize() noexcept
    {
        // -1 to deal with the pointer one-past-the-end;
        return QtPrivate::MaxAllocSize / sizeof(storage_type) - 1;
    }
private:
#if QT_VERSION >= QT_VERSION_CHECK(7, 0, 0) || defined(QT_BOOTSTRAPPED)
    const storage_type *m_data = nullptr;
    qsizetype m_size = 0;
#else
    qsizetype m_size = 0;
    const storage_type *m_data = nullptr;
#endif

    Q_ALWAYS_INLINE constexpr void verify([[maybe_unused]] qsizetype pos = 0,
                                          [[maybe_unused]] qsizetype n = 1) const
    {
        Q_PRE(pos >= 0);
        Q_PRE(pos <= size());
        Q_PRE(n >= 0);
        Q_PRE(n <= size() - pos);
    }

    constexpr int compare_single_char_helper(int diff) const noexcept
    { return diff ? diff : size() > 1 ? 1 : 0; }

    Q_CORE_EXPORT static bool equal_helper(QStringView sv, const char *data, qsizetype len);
    Q_CORE_EXPORT static int compare_helper(QStringView sv, const char *data, qsizetype len);

#if !defined(QT_NO_CAST_FROM_ASCII) && !defined(QT_RESTRICTED_CAST_FROM_ASCII)
    friend bool comparesEqual(const QStringView &lhs, const QByteArrayView &rhs) noexcept
    { return equal_helper(lhs, rhs.data(), rhs.size()); }
    friend Qt::strong_ordering
    compareThreeWay(const QStringView &lhs, const QByteArrayView &rhs) noexcept
    {
        const int res = compare_helper(lhs, rhs.data(), rhs.size());
        return Qt::compareThreeWay(res, 0);
    }
    Q_DECLARE_STRONGLY_ORDERED(QStringView, QByteArrayView, QT_ASCII_CAST_WARN)
    Q_DECLARE_STRONGLY_ORDERED(QStringView, QByteArray, QT_ASCII_CAST_WARN)
    Q_DECLARE_STRONGLY_ORDERED(QStringView, const char *, QT_ASCII_CAST_WARN)
#endif // !defined(QT_NO_CAST_FROM_ASCII) && !defined(QT_RESTRICTED_CAST_FROM_ASCII)
};
Q_DECLARE_TYPEINFO(QStringView, Q_PRIMITIVE_TYPE);

template <typename QStringLike, typename std::enable_if<
    std::is_same<QStringLike, QString>::value,
    bool>::type = true>
inline QStringView qToStringViewIgnoringNull(const QStringLike &s) noexcept
{ return QStringView(s.begin(), s.size()); }

// QChar inline functions:

[[nodiscard]] constexpr auto QChar::fromUcs4(char32_t c) noexcept
{
    struct R {
        char16_t chars[2];
        [[nodiscard]] constexpr operator QStringView() const noexcept { return {begin(), end()}; }
        [[nodiscard]] constexpr qsizetype size() const noexcept { return chars[1] ? 2 : 1; }
        [[nodiscard]] constexpr const char16_t *begin() const noexcept { return chars; }
        [[nodiscard]] constexpr const char16_t *end() const noexcept { return begin() + size(); }
    };
    return requiresSurrogates(c) ? R{{QChar::highSurrogate(c),
                                      QChar::lowSurrogate(c)}} :
                                   R{{char16_t(c), u'\0'}} ;
}

qsizetype QtPrivate::findString(QStringView str, qsizetype from, QChar ch, Qt::CaseSensitivity cs) noexcept
{
    if (from < -str.size()) // from < 0 && abs(from) > str.size(), avoiding overflow
        return -1;
    if (from < 0)
        from = qMax(from + str.size(), qsizetype(0));
    if (from < str.size()) {
        const char16_t *s = str.utf16();
        char16_t c = ch.unicode();
        const char16_t *n = s + from;
        const char16_t *e = s + str.size();
        if (cs == Qt::CaseSensitive)
            n = qustrchr(QStringView(n, e), c);
        else
            n = qustrcasechr(QStringView(n, e), c);
        if (n != e)
            return n - s;
    }
    return -1;
}

namespace Qt {
inline namespace Literals {
inline namespace StringLiterals {
constexpr QStringView operator""_sv(const char16_t *str, size_t size) noexcept
{
    return QStringView(str, qsizetype(size));
}
} // StringLiterals
} // Literals
} // Qt

QT_END_NAMESPACE

#endif /* QSTRINGVIEW_H */
