// Copyright (C) 2020 The Qt Company Ltd.
// Copyright (C) 2019 Intel Corporation.
// Copyright (C) 2019 Mail.ru Group.
// Copyright (C) 2020 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Marc Mutz <marc.mutz@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:data-parser

#ifndef QSTRING_H
#define QSTRING_H

#if defined(QT_NO_CAST_FROM_ASCII) && defined(QT_RESTRICTED_CAST_FROM_ASCII)
#error QT_NO_CAST_FROM_ASCII and QT_RESTRICTED_CAST_FROM_ASCII must not be defined at the same time
#endif

#include <QtCore/qchar.h>
#include <QtCore/qcompare.h>
#include <QtCore/qbytearray.h>
#include <QtCore/qbytearrayview.h>
#include <QtCore/qarraydata.h>
#include <QtCore/qarraydatapointer.h>
#include <QtCore/qlatin1stringview.h>
#include <QtCore/qnamespace.h>
#include <QtCore/qstringalgorithms.h>
#include <QtCore/qanystringview.h>
#include <QtCore/qstringtokenizer.h>

#include <string>
#include <iterator>
#include <QtCore/q20memory.h>
#include <string_view>
#include <QtCore/q23type_traits.h>

#include <stdarg.h>

#ifdef truncate
#error qstring.h must be included before any header file that defines truncate
#endif

#if defined(Q_OS_DARWIN) || defined(Q_QDOC)
Q_FORWARD_DECLARE_CF_TYPE(CFString);
Q_FORWARD_DECLARE_OBJC_CLASS(NSString);
#endif

class tst_QString;

QT_BEGIN_NAMESPACE

class qfloat16;
class QRegularExpression;
class QRegularExpressionMatch;
class QString;

namespace QtPrivate {
template <bool...B> class BoolList;

template <typename Char>
using IsCompatibleChar32TypeHelper =
    std::is_same<Char, char32_t>;
template <typename Char>
using IsCompatibleChar32Type
    = IsCompatibleChar32TypeHelper<q20::remove_cvref_t<Char>>;

// hack to work around ushort/uchar etc being treated as both characters and
// integers, depending on which Qt API you look at:
template <typename T> struct treat_as_integral_arg : std::false_type {};
template <> struct treat_as_integral_arg<unsigned short> : std::true_type {};
template <> struct treat_as_integral_arg<  signed short> : std::true_type {};
template <> struct treat_as_integral_arg<unsigned  char> : std::true_type {};
template <> struct treat_as_integral_arg<  signed  char> : std::true_type {};
}

// Qt 4.x compatibility

//
// QLatin1StringView inline implementations
//
constexpr bool QtPrivate::isLatin1(QLatin1StringView) noexcept
{ return true; }

//
// QStringView members that require QLatin1StringView:
//
int QStringView::compare(QLatin1StringView s, Qt::CaseSensitivity cs) const noexcept
{ return QtPrivate::compareStrings(*this, s, cs); }
bool QStringView::startsWith(QLatin1StringView s, Qt::CaseSensitivity cs) const noexcept
{ return QtPrivate::startsWith(*this, s, cs); }
bool QStringView::endsWith(QLatin1StringView s, Qt::CaseSensitivity cs) const noexcept
{ return QtPrivate::endsWith(*this, s, cs); }
qsizetype QStringView::indexOf(QLatin1StringView s, qsizetype from, Qt::CaseSensitivity cs) const noexcept
{ return QtPrivate::findString(*this, from, s, cs); }
bool QStringView::contains(QLatin1StringView s, Qt::CaseSensitivity cs) const noexcept
{ return indexOf(s, 0, cs) != qsizetype(-1); }
qsizetype QStringView::lastIndexOf(QLatin1StringView s, Qt::CaseSensitivity cs) const noexcept
{ return QtPrivate::lastIndexOf(*this, size(), s, cs); }
qsizetype QStringView::lastIndexOf(QLatin1StringView s, qsizetype from, Qt::CaseSensitivity cs) const noexcept
{ return QtPrivate::lastIndexOf(*this, from, s, cs); }
qsizetype QStringView::count(QLatin1StringView s, Qt::CaseSensitivity cs) const
{ return QtPrivate::count(*this, s, cs); }

//
// QAnyStringView members that require QLatin1StringView
//

constexpr QAnyStringView::QAnyStringView(QLatin1StringView str) noexcept
    : m_data{str.data()}, m_size{size_t(str.size() << SizeShift) | Tag::Latin1} {}

constexpr QLatin1StringView QAnyStringView::asLatin1StringView() const
{
    Q_ASSERT(isLatin1());
    return {m_data_utf8, size()};
}


template <typename Visitor>
constexpr decltype(auto) QAnyStringView::visit(Visitor &&v) const
{
    if (isUtf16())
        return std::forward<Visitor>(v)(asStringView());
    else if (isLatin1())
        return std::forward<Visitor>(v)(asLatin1StringView());
    else
        return std::forward<Visitor>(v)(asUtf8StringView());
}

//
// QAnyStringView members that require QAnyStringView::visit()
//

constexpr QChar QAnyStringView::front() const
{
    return visit([] (auto that) { return QAnyStringView::toQChar(that.front()); });
}
constexpr QChar QAnyStringView::back() const
{
    return visit([] (auto that) { return QAnyStringView::toQChar(that.back()); });
}

using QStringPrivate = QArrayDataPointer<char16_t>;

class Q_CORE_EXPORT QString
{
    typedef QTypedArrayData<char16_t> Data;

    friend class ::tst_QString;

    template <typename Iterator>
    static constexpr bool is_contiguous_iterator_v =
        // Can't use contiguous_iterator_tag here, as STL impls can't agree on feature macro.
        // To avoid differences in C++20 and C++17 builds, treat only pointers as contiguous
        // for now:
        // std::contiguous_iterator<Iterator>;
        std::is_pointer_v<Iterator>;

    template <typename Char>
    using is_compatible_char_helper = std::disjunction<
            QtPrivate::IsCompatibleCharType<Char>,
            QtPrivate::IsCompatibleChar32Type<Char>,
            QtPrivate::IsCompatibleChar8Type<Char>,
            std::is_same<Char, QLatin1Char> // special case
        >;

    template <typename T>
    using is_string_like = std::conjunction<
            std::negation<QtPrivate::treat_as_integral_arg<std::remove_cv_t<T>>>, // used to be integral, so keep
            std::is_convertible<T, QAnyStringView>
        >;

    template <typename T>
    using if_string_like = std::enable_if_t<is_string_like<T>::value, bool>;

    template <typename T>
    using is_floating_point_like = std::disjunction<
        #if QFLOAT16_IS_NATIVE
            std::is_same<q20::remove_cvref_t<T>, QtPrivate::NativeFloat16Type>,
        #endif
            std::is_same<q20::remove_cvref_t<T>, qfloat16>,
            std::is_floating_point<T>
        >;

    template <typename T>
    using if_floating_point = std::enable_if_t<is_floating_point_like<T>::value, bool>;

    template <typename T>
    using if_integral_non_char = std::enable_if_t<std::conjunction_v<
            std::disjunction<
                std::is_integral<T>,
                std::conjunction<
                    std::is_enum<T>,                       // (unscoped) enums yes,
                    std::negation<q23::is_scoped_enum<T>>  // but not scoped ones
                >
            >,
            std::negation<is_floating_point_like<T>>, // has its own overload
            std::negation<is_string_like<T>>          // ditto
        >, bool>;

    template <typename Iterator>
    static constexpr bool is_compatible_iterator_v = std::conjunction_v<
            std::is_convertible<
                typename std::iterator_traits<Iterator>::iterator_category,
                std::input_iterator_tag
            >,
            is_compatible_char_helper<typename std::iterator_traits<Iterator>::value_type>
        >;

    template <typename Iterator>
    using if_compatible_iterator = std::enable_if_t<is_compatible_iterator_v<Iterator>, bool>;

public:
    typedef QStringPrivate DataPointer;

    constexpr QString() noexcept;
    explicit QString(const QChar *unicode, qsizetype size = -1);
    QString(QChar c);
    QString(qsizetype size, QChar c);
    inline QString(QLatin1StringView latin1);
    explicit QString(QStringView sv) : QString(sv.data(), sv.size()) {}
#if defined(__cpp_char8_t) || defined(Q_QDOC)
    Q_WEAK_OVERLOAD
    inline QString(const char8_t *str)
        : QString(fromUtf8(str))
    {}
#endif
    inline QString(const QString &) noexcept;
    inline ~QString();
    QString &operator=(QChar c);
    QString &operator=(const QString &) noexcept;
    QString &operator=(QLatin1StringView latin1);
    inline QString(QString &&other) noexcept
        = default;
    QT_MOVE_ASSIGNMENT_OPERATOR_IMPL_VIA_PURE_SWAP(QString)
    void swap(QString &other) noexcept { d.swap(other.d); }

    static constexpr qsizetype maxSize() noexcept
    {
        // -1 to deal with the NUL terminator
        return Data::maxSize() - 1;
    }
    constexpr qsizetype size() const noexcept
    {
        constexpr size_t MaxSize = maxSize();
        Q_PRESUME(size_t(d.size) <= MaxSize);
        return d.size;
    }
#if QT_DEPRECATED_SINCE(6, 4)
    QT_DEPRECATED_VERSION_X_6_4("Use size() or length() instead.")
    constexpr qsizetype count() const { return size(); }
#endif
    constexpr qsizetype length() const noexcept { return size(); }
    constexpr bool isEmpty() const noexcept { return size() == 0; }
    void resize(qsizetype size);
    void resize(qsizetype size, QChar fillChar);
    void resizeForOverwrite(qsizetype size);

    QString &fill(QChar c, qsizetype size = -1);
    void truncate(qsizetype pos);
    void chop(qsizetype n);

    QString &slice(qsizetype pos)
    { verify(pos, 0); return remove(0, pos); }
    QString &slice(qsizetype pos, qsizetype n)
    {
        verify(pos, n);
        if (isNull())
            return *this;
        resize(pos + n);
        return remove(0, pos);
    }

    inline qsizetype capacity() const;
    inline void reserve(qsizetype size);
    inline void squeeze();

    inline const QChar *unicode() const;
    inline QChar *data();
    inline const QChar *data() const;
    inline const QChar *constData() const;

    inline void detach();
    inline bool isDetached() const;
    inline bool isSharedWith(const QString &other) const { return d.isSharedWith(other.d); }
    inline void clear();

    inline const QChar at(qsizetype i) const;
    inline const QChar operator[](qsizetype i) const;
    [[nodiscard]] inline QChar &operator[](qsizetype i);

    [[nodiscard]] inline QChar front() const { return at(0); }
    [[nodiscard]] inline QChar &front();
    [[nodiscard]] inline QChar back() const { return at(size() - 1); }
    [[nodiscard]] inline QChar &back();

#if QT_CORE_REMOVED_SINCE(6, 9)
    [[nodiscard]] QString arg(qlonglong a, int fieldwidth=0, int base=10,
                QChar fillChar = u' ') const;
    [[nodiscard]] QString arg(qulonglong a, int fieldwidth=0, int base=10,
                QChar fillChar = u' ') const;
    [[nodiscard]] inline QString arg(long a, int fieldwidth=0, int base=10,
                QChar fillChar = u' ') const;
    [[nodiscard]] inline QString arg(ulong a, int fieldwidth=0, int base=10,
                QChar fillChar = u' ') const;
    [[nodiscard]] inline QString arg(int a, int fieldWidth = 0, int base = 10,
                QChar fillChar = u' ') const;
    [[nodiscard]] inline QString arg(uint a, int fieldWidth = 0, int base = 10,
                QChar fillChar = u' ') const;
    [[nodiscard]] inline QString arg(short a, int fieldWidth = 0, int base = 10,
                QChar fillChar = u' ') const;
    [[nodiscard]] inline QString arg(ushort a, int fieldWidth = 0, int base = 10,
                QChar fillChar = u' ') const;
    [[nodiscard]] QString arg(double a, int fieldWidth = 0, char format = 'g', int precision = -1,
                QChar fillChar = u' ') const;
    [[nodiscard]] QString arg(char a, int fieldWidth = 0,
                QChar fillChar = u' ') const;
    [[nodiscard]] QString arg(QChar a, int fieldWidth = 0,
                QChar fillChar = u' ') const;
    [[nodiscard]] QString arg(const QString &a, int fieldWidth = 0,
                QChar fillChar = u' ') const;
    [[nodiscard]] QString arg(QStringView a, int fieldWidth = 0,
                QChar fillChar = u' ') const;
    [[nodiscard]] QString arg(QLatin1StringView a, int fieldWidth = 0,
                QChar fillChar = u' ') const;
#endif

    template <typename T, if_integral_non_char<T> = true>
    [[nodiscard]] QString arg(T a, int fieldWidth = 0, int base = 10,
                              QChar fillChar = u' ') const
    {
        using U = typename std::conditional<
                // underlying_type_t<non-enum> is UB in C++17/SFINAE in C++20, so wrap:
                std::is_enum_v<T>, std::underlying_type<T>,
                                   q20::type_identity<T>
            >::type::type;
        if constexpr (std::is_signed_v<U>)
            return arg_impl(qlonglong(a), fieldWidth, base, fillChar);
        else
            return arg_impl(qulonglong(a), fieldWidth, base, fillChar);
    }

    template <typename T, if_floating_point<T> = true>
    [[nodiscard]] QString arg(T a, int fieldWidth = 0, char format = 'g', int precision = -1,
                              QChar fillChar = u' ') const
    { return arg_impl(double(a), fieldWidth, format, precision, fillChar); }

    template <typename T, if_string_like<T> = true>
    [[nodiscard]] QString arg(const T &a, int fieldWidth = 0, QChar fillChar = u' ') const
    { return arg_impl(QAnyStringView(a), fieldWidth, fillChar); }

private:
    QString arg_impl(qlonglong a, int fieldwidth, int base, QChar fillChar) const;
    QString arg_impl(qulonglong a, int fieldwidth, int base, QChar fillChar) const;
    QString arg_impl(double a, int fieldWidth, char format, int precision, QChar fillChar) const;
    QString arg_impl(QAnyStringView a, int fieldWidth, QChar fillChar) const;

public:
    template <typename...Args>
    [[nodiscard]]
#ifdef Q_QDOC
    QString
#else
    typename std::enable_if<
        sizeof...(Args) >= 2 && std::conjunction_v<is_string_like<Args>...>,
        QString
    >::type
#endif
    arg(Args &&...args) const
    { return qToStringViewIgnoringNull(*this).arg(std::forward<Args>(args)...); }

    static QString vasprintf(const char *format, va_list ap) Q_ATTRIBUTE_FORMAT_PRINTF(1, 0);
    static QString asprintf(const char *format, ...) Q_ATTRIBUTE_FORMAT_PRINTF(1, 2);

    [[nodiscard]] QT_CORE_INLINE_SINCE(6, 8)
    qsizetype indexOf(QChar c, qsizetype from = 0, Qt::CaseSensitivity cs = Qt::CaseSensitive) const;
    [[nodiscard]] qsizetype indexOf(QLatin1StringView s, qsizetype from = 0, Qt::CaseSensitivity cs = Qt::CaseSensitive) const;
    [[nodiscard]] qsizetype indexOf(const QString &s, qsizetype from = 0, Qt::CaseSensitivity cs = Qt::CaseSensitive) const;
    [[nodiscard]] qsizetype indexOf(QStringView s, qsizetype from = 0, Qt::CaseSensitivity cs = Qt::CaseSensitive) const noexcept
    { return QtPrivate::findString(*this, from, s, cs); }
    [[nodiscard]] qsizetype lastIndexOf(QChar c, Qt::CaseSensitivity cs = Qt::CaseSensitive) const noexcept
    { return lastIndexOf(c, -1, cs); }
    [[nodiscard]] QT_CORE_INLINE_SINCE(6, 8)
    qsizetype lastIndexOf(QChar c, qsizetype from, Qt::CaseSensitivity cs = Qt::CaseSensitive) const;
    [[nodiscard]] qsizetype lastIndexOf(QLatin1StringView s, Qt::CaseSensitivity cs = Qt::CaseSensitive) const
    { return lastIndexOf(s, size(), cs); }
    [[nodiscard]] qsizetype lastIndexOf(QLatin1StringView s, qsizetype from, Qt::CaseSensitivity cs = Qt::CaseSensitive) const;
    [[nodiscard]] qsizetype lastIndexOf(const QString &s, Qt::CaseSensitivity cs = Qt::CaseSensitive) const
    { return lastIndexOf(s, size(), cs); }
    [[nodiscard]] qsizetype lastIndexOf(const QString &s, qsizetype from, Qt::CaseSensitivity cs = Qt::CaseSensitive) const;

    [[nodiscard]] qsizetype lastIndexOf(QStringView s, Qt::CaseSensitivity cs = Qt::CaseSensitive) const noexcept
    { return lastIndexOf(s, size(), cs); }
    [[nodiscard]] qsizetype lastIndexOf(QStringView s, qsizetype from, Qt::CaseSensitivity cs = Qt::CaseSensitive) const noexcept
    { return QtPrivate::lastIndexOf(*this, from, s, cs); }

    [[nodiscard]] inline bool contains(QChar c, Qt::CaseSensitivity cs = Qt::CaseSensitive) const;
    [[nodiscard]] inline bool contains(const QString &s, Qt::CaseSensitivity cs = Qt::CaseSensitive) const;
    [[nodiscard]] inline bool contains(QLatin1StringView s, Qt::CaseSensitivity cs = Qt::CaseSensitive) const;
    [[nodiscard]] inline bool contains(QStringView s, Qt::CaseSensitivity cs = Qt::CaseSensitive) const noexcept;
    [[nodiscard]] qsizetype count(QChar c, Qt::CaseSensitivity cs = Qt::CaseSensitive) const;
    [[nodiscard]] qsizetype count(const QString &s, Qt::CaseSensitivity cs = Qt::CaseSensitive) const;
    [[nodiscard]] qsizetype count(QStringView s, Qt::CaseSensitivity cs = Qt::CaseSensitive) const;

#if QT_CONFIG(regularexpression)
    [[nodiscard]] qsizetype indexOf(const QRegularExpression &re, qsizetype from = 0,
                                    QRegularExpressionMatch *rmatch = nullptr) const;
#ifdef Q_QDOC
    [[nodiscard]] qsizetype lastIndexOf(const QRegularExpression &re, QRegularExpressionMatch *rmatch = nullptr) const;
#else
    // prevent an ambiguity when called like this: lastIndexOf(re, 0)
    template <typename T = QRegularExpressionMatch, std::enable_if_t<std::is_same_v<T, QRegularExpressionMatch>, bool> = false>
    [[nodiscard]] qsizetype lastIndexOf(const QRegularExpression &re, T *rmatch = nullptr) const
    { return lastIndexOf(re, size(), rmatch); }
#endif
    [[nodiscard]] qsizetype lastIndexOf(const QRegularExpression &re, qsizetype from,
                                        QRegularExpressionMatch *rmatch = nullptr) const;
    [[nodiscard]] bool contains(const QRegularExpression &re, QRegularExpressionMatch *rmatch = nullptr) const;
    [[nodiscard]] qsizetype count(const QRegularExpression &re) const;
#endif

    enum SectionFlag {
        SectionDefault             = 0x00,
        SectionSkipEmpty           = 0x01,
        SectionIncludeLeadingSep   = 0x02,
        SectionIncludeTrailingSep  = 0x04,
        SectionCaseInsensitiveSeps = 0x08
    };
    Q_DECLARE_FLAGS(SectionFlags, SectionFlag)

    [[nodiscard]] inline QString section(QChar sep, qsizetype start, qsizetype end = -1, SectionFlags flags = SectionDefault) const;
    [[nodiscard]] QString section(const QString &in_sep, qsizetype start, qsizetype end = -1, SectionFlags flags = SectionDefault) const;
#if QT_CONFIG(regularexpression)
    [[nodiscard]] QString section(const QRegularExpression &re, qsizetype start, qsizetype end = -1, SectionFlags flags = SectionDefault) const;
#endif

#if QT_CORE_REMOVED_SINCE(6, 7)
    QString left(qsizetype n) const;
    QString right(qsizetype n) const;
    QString mid(qsizetype position, qsizetype n = -1) const;

    QString first(qsizetype n) const;
    QString last(qsizetype n) const;
    QString sliced(qsizetype pos) const;
    QString sliced(qsizetype pos, qsizetype n) const;
    QString chopped(qsizetype n) const;
#else
    [[nodiscard]] QString left(qsizetype n) const &
    {
        if (size_t(n) >= size_t(size()))
            return *this;
        return first(n);
    }
    [[nodiscard]] QString left(qsizetype n) &&
    {
        if (size_t(n) >= size_t(size()))
            return std::move(*this);
        return std::move(*this).first(n);
    }
    [[nodiscard]] QString right(qsizetype n) const &
    {
        if (size_t(n) >= size_t(size()))
            return *this;
        return last(n);
    }
    [[nodiscard]] QString right(qsizetype n) &&
    {
        if (size_t(n) >= size_t(size()))
            return std::move(*this);
        return std::move(*this).last(n);
    }
    [[nodiscard]] QString mid(qsizetype position, qsizetype n = -1) const &;
    [[nodiscard]] QString mid(qsizetype position, qsizetype n = -1) &&;

    [[nodiscard]] QString first(qsizetype n) const &
    { verify(0, n); return sliced(0, n); }
    [[nodiscard]] QString last(qsizetype n) const &
    { verify(0, n); return sliced(size() - n, n); }
    [[nodiscard]] QString sliced(qsizetype pos) const &
    { verify(pos, 0); return sliced(pos, size() - pos); }
    [[nodiscard]] QString sliced(qsizetype pos, qsizetype n) const &
    { verify(pos, n); return QString(begin() + pos, n); }
    [[nodiscard]] QString chopped(qsizetype n) const &
    { verify(0, n); return sliced(0, size() - n); }

    [[nodiscard]] QString first(qsizetype n) &&
    {
        verify(0, n);
        resize(n);      // may detach and allocate memory
        return std::move(*this);
    }
    [[nodiscard]] QString last(qsizetype n) &&
    { verify(0, n); return sliced_helper(*this, size() - n, n); }
    [[nodiscard]] QString sliced(qsizetype pos) &&
    { verify(pos, 0); return sliced_helper(*this, pos, size() - pos); }
    [[nodiscard]] QString sliced(qsizetype pos, qsizetype n) &&
    { verify(pos, n); return sliced_helper(*this, pos, n); }
    [[nodiscard]] QString chopped(qsizetype n) &&
    { verify(0, n); return std::move(*this).first(size() - n); }
#endif
    bool startsWith(const QString &s, Qt::CaseSensitivity cs = Qt::CaseSensitive) const;
    [[nodiscard]] bool startsWith(QStringView s, Qt::CaseSensitivity cs = Qt::CaseSensitive) const noexcept
    { return QtPrivate::startsWith(*this, s, cs); }
    bool startsWith(QLatin1StringView s, Qt::CaseSensitivity cs = Qt::CaseSensitive) const;
    bool startsWith(QChar c, Qt::CaseSensitivity cs = Qt::CaseSensitive) const;

    bool endsWith(const QString &s, Qt::CaseSensitivity cs = Qt::CaseSensitive) const;
    [[nodiscard]] bool endsWith(QStringView s, Qt::CaseSensitivity cs = Qt::CaseSensitive) const noexcept
    { return QtPrivate::endsWith(*this, s, cs); }
    bool endsWith(QLatin1StringView s, Qt::CaseSensitivity cs = Qt::CaseSensitive) const;
    bool endsWith(QChar c, Qt::CaseSensitivity cs = Qt::CaseSensitive) const;

    bool isUpper() const;
    bool isLower() const;

    [[nodiscard]] QString leftJustified(qsizetype width, QChar fill = u' ', bool trunc = false) const;
    [[nodiscard]] QString rightJustified(qsizetype width, QChar fill = u' ', bool trunc = false) const;

#if !defined(Q_QDOC)
    [[nodiscard]] QString toLower() const &
    { return toLower_helper(*this); }
    [[nodiscard]] QString toLower() &&
    { return toLower_helper(*this); }
    [[nodiscard]] QString toUpper() const &
    { return toUpper_helper(*this); }
    [[nodiscard]] QString toUpper() &&
    { return toUpper_helper(*this); }
    [[nodiscard]] QString toCaseFolded() const &
    { return toCaseFolded_helper(*this); }
    [[nodiscard]] QString toCaseFolded() &&
    { return toCaseFolded_helper(*this); }
    [[nodiscard]] QString trimmed() const &
    { return trimmed_helper(*this); }
    [[nodiscard]] QString trimmed() &&
    { return trimmed_helper(*this); }
    [[nodiscard]] QString simplified() const &
    { return simplified_helper(*this); }
    [[nodiscard]] QString simplified() &&
    { return simplified_helper(*this); }
#else
    [[nodiscard]] QString toLower() const;
    [[nodiscard]] QString toUpper() const;
    [[nodiscard]] QString toCaseFolded() const;
    [[nodiscard]] QString trimmed() const;
    [[nodiscard]] QString simplified() const;
#endif
    [[nodiscard]] QString toHtmlEscaped() const;

    QString &insert(qsizetype i, QChar c);
    QString &insert(qsizetype i, const QChar *uc, qsizetype len);
    inline QString &insert(qsizetype i, const QString &s) { return insert(i, s.constData(), s.size()); }
    inline QString &insert(qsizetype i, QStringView v) { return insert(i, v.data(), v.size()); }
    QString &insert(qsizetype i, QLatin1StringView s);
    QString &insert(qsizetype i, QUtf8StringView s);

    QString &append(QChar c);
    QString &append(const QChar *uc, qsizetype len);
    QString &append(const QString &s);
    inline QString &append(QStringView v) { return append(v.data(), v.size()); }
    QString &append(QLatin1StringView s);
    QString &append(QUtf8StringView s);

    inline QString &prepend(QChar c) { return insert(0, c); }
    inline QString &prepend(const QChar *uc, qsizetype len) { return insert(0, uc, len); }
    inline QString &prepend(const QString &s) { return insert(0, s); }
    inline QString &prepend(QStringView v) { return prepend(v.data(), v.size()); }
    inline QString &prepend(QLatin1StringView s) { return insert(0, s); }
    QString &prepend(QUtf8StringView s) { return insert(0, s); }

    QString &assign(QAnyStringView s);
    inline QString &assign(qsizetype n, QChar c)
    {
        Q_ASSERT(n >= 0);
        return fill(c, n);
    }
    template <typename InputIterator, if_compatible_iterator<InputIterator> = true>
    QString &assign(InputIterator first, InputIterator last)
    {
        using V = typename std::iterator_traits<InputIterator>::value_type;
        constexpr bool IsL1C = std::is_same_v<std::remove_cv_t<V>, QLatin1Char>;
        constexpr bool IsFwdIt = std::is_convertible_v<
                typename std::iterator_traits<InputIterator>::iterator_category,
                std::forward_iterator_tag
            >;

        if constexpr (is_contiguous_iterator_v<InputIterator>) {
            const auto p = q20::to_address(first);
            const auto len = qsizetype(last - first);
            if constexpr (IsL1C)
                return assign(QLatin1StringView(reinterpret_cast<const char*>(p), len));
            else if constexpr (sizeof(V) == 4)
                return assign_helper(p, len);
            else
                return assign(QAnyStringView(p, len));
        } else if constexpr (sizeof(V) == 4) { // non-contiguous iterator, feed data piecemeal
            resize(0);
            if constexpr (IsFwdIt) {
                const qsizetype requiredCapacity = 2 * std::distance(first, last);
                reserve(requiredCapacity);
            }
            while (first != last) {
                append(QChar::fromUcs4(*first));
                ++first;
            }
            return *this;
        } else if constexpr (QtPrivate::IsCompatibleChar8Type<V>::value) {
            assign_helper_char8(first, last);
            if (d.constAllocatedCapacity())
                d.data()[d.size] = u'\0';
            return *this;
        } else {
            d.assign(first, last, [](QChar ch) noexcept -> char16_t { return ch.unicode(); });
            if (d.constAllocatedCapacity())
                d.data()[d.size] = u'\0';
            return *this;
        }
    }

    inline QString &operator+=(QChar c) { return append(c); }

    inline QString &operator+=(const QString &s) { return append(s); }
    inline QString &operator+=(QStringView v) { return append(v); }
    inline QString &operator+=(QLatin1StringView s) { return append(s); }
    QString &operator+=(QUtf8StringView s) { return append(s); }

#if defined(QT_RESTRICTED_CAST_FROM_ASCII)
    template <qsizetype N>
    QString &insert(qsizetype i, const char (&ch)[N]) { return insert(i, QUtf8StringView(ch)); }
    template <qsizetype N>
    QString &append(const char (&ch)[N]) { return append(QUtf8StringView(ch)); }
    template <qsizetype N>
    QString &prepend(const char (&ch)[N]) { return prepend(QUtf8StringView(ch)); }
    template <qsizetype N>
    QString &operator+=(const char (&ch)[N]) { return append(QUtf8StringView(ch)); }
#endif

    QString &remove(qsizetype i, qsizetype len);
    QString &remove(QChar c, Qt::CaseSensitivity cs = Qt::CaseSensitive);
    QString &remove(QLatin1StringView s, Qt::CaseSensitivity cs = Qt::CaseSensitive);
    QString &remove(const QString &s, Qt::CaseSensitivity cs = Qt::CaseSensitive);

    QString &removeAt(qsizetype pos)
    { return size_t(pos) < size_t(size()) ? remove(pos, 1) : *this; }
    QString &removeFirst() { return !isEmpty() ? remove(0, 1) : *this; }
    QString &removeLast() { return !isEmpty() ? remove(size() - 1, 1) : *this; }

    template <typename Predicate>
    QString &removeIf(Predicate pred)
    {
        removeIf_helper(pred);
        return *this;
    }

    QString &replace(qsizetype i, qsizetype len, QChar after);
    QString &replace(qsizetype i, qsizetype len, const QChar *s, qsizetype slen);
    QString &replace(qsizetype i, qsizetype len, const QString &after);
    QString &replace(QChar before, QChar after, Qt::CaseSensitivity cs = Qt::CaseSensitive);
    QString &replace(const QChar *before, qsizetype blen, const QChar *after, qsizetype alen, Qt::CaseSensitivity cs = Qt::CaseSensitive);
    QString &replace(QLatin1StringView before, QLatin1StringView after, Qt::CaseSensitivity cs = Qt::CaseSensitive);
    QString &replace(QLatin1StringView before, const QString &after, Qt::CaseSensitivity cs = Qt::CaseSensitive);
    QString &replace(const QString &before, QLatin1StringView after, Qt::CaseSensitivity cs = Qt::CaseSensitive);
    QString &replace(const QString &before, const QString &after,
                     Qt::CaseSensitivity cs = Qt::CaseSensitive);
    QString &replace(QChar c, const QString &after, Qt::CaseSensitivity cs = Qt::CaseSensitive);
    QString &replace(QChar c, QLatin1StringView after, Qt::CaseSensitivity cs = Qt::CaseSensitive);
#if QT_CONFIG(regularexpression)
    QString &replace(const QRegularExpression &re, const QString  &after);
    inline QString &remove(const QRegularExpression &re)
    { return replace(re, QString()); }
#endif

public:
    [[nodiscard]]
    QStringList split(const QString &sep, Qt::SplitBehavior behavior = Qt::KeepEmptyParts,
                      Qt::CaseSensitivity cs = Qt::CaseSensitive) const;
    [[nodiscard]]
    QStringList split(QChar sep, Qt::SplitBehavior behavior = Qt::KeepEmptyParts,
                      Qt::CaseSensitivity cs = Qt::CaseSensitive) const;
#ifndef QT_NO_REGULAREXPRESSION
    [[nodiscard]]
    QStringList split(const QRegularExpression &sep,
                      Qt::SplitBehavior behavior = Qt::KeepEmptyParts) const;
#endif

    template <typename Needle, typename...Flags>
    [[nodiscard]] inline auto tokenize(Needle &&needle, Flags...flags) const &
        noexcept(noexcept(qTokenize(std::declval<const QString &>(), std::forward<Needle>(needle), flags...)))
            -> decltype(qTokenize(*this, std::forward<Needle>(needle), flags...))
    { return qTokenize(qToStringViewIgnoringNull(*this), std::forward<Needle>(needle), flags...); }

    template <typename Needle, typename...Flags>
    [[nodiscard]] inline auto tokenize(Needle &&needle, Flags...flags) const &&
        noexcept(noexcept(qTokenize(std::declval<const QString>(), std::forward<Needle>(needle), flags...)))
            -> decltype(qTokenize(std::move(*this), std::forward<Needle>(needle), flags...))
    { return qTokenize(std::move(*this), std::forward<Needle>(needle), flags...); }

    template <typename Needle, typename...Flags>
    [[nodiscard]] inline auto tokenize(Needle &&needle, Flags...flags) &&
        noexcept(noexcept(qTokenize(std::declval<QString>(), std::forward<Needle>(needle), flags...)))
            -> decltype(qTokenize(std::move(*this), std::forward<Needle>(needle), flags...))
    { return qTokenize(std::move(*this), std::forward<Needle>(needle), flags...); }


    enum NormalizationForm {
        NormalizationForm_D,
        NormalizationForm_C,
        NormalizationForm_KD,
        NormalizationForm_KC
    };
    [[nodiscard]] QString normalized(NormalizationForm mode, QChar::UnicodeVersion version = QChar::Unicode_Unassigned) const;

    [[nodiscard]] QString repeated(qsizetype times) const;

    const ushort *utf16() const; // ### Qt 7 char16_t
    [[nodiscard]] QString nullTerminated() const &;
    [[nodiscard]] QString nullTerminated() &&;
    QString &nullTerminate();

#if !defined(Q_QDOC)
    [[nodiscard]] QByteArray toLatin1() const &
    { return toLatin1_helper(*this); }
    [[nodiscard]] QByteArray toLatin1() &&
    { return toLatin1_helper_inplace(*this); }
    [[nodiscard]] QByteArray toUtf8() const &
    { return toUtf8_helper(*this); }
    [[nodiscard]] QByteArray toUtf8() &&
    { return toUtf8_helper(*this); }
    [[nodiscard]] QByteArray toLocal8Bit() const &
    { return toLocal8Bit_helper(isNull() ? nullptr : constData(), size()); }
    [[nodiscard]] QByteArray toLocal8Bit() &&
    { return toLocal8Bit_helper(isNull() ? nullptr : constData(), size()); }
#else
    [[nodiscard]] QByteArray toLatin1() const;
    [[nodiscard]] QByteArray toUtf8() const;
    [[nodiscard]] QByteArray toLocal8Bit() const;
#endif
    [[nodiscard]] QList<uint> toUcs4() const; // ### Qt 7 char32_t

    // note - this are all inline so we can benefit from strlen() compile time optimizations
    static QString fromLatin1(QByteArrayView ba);
    Q_WEAK_OVERLOAD
    static inline QString fromLatin1(const QByteArray &ba) { return fromLatin1(QByteArrayView(ba)); }
    static inline QString fromLatin1(const char *str, qsizetype size)
    {
        return fromLatin1(QByteArrayView(str, !str || size < 0 ? qstrlen(str) : size));
    }
    static QString fromUtf8(QByteArrayView utf8);
    Q_WEAK_OVERLOAD
    static inline QString fromUtf8(const QByteArray &ba) { return fromUtf8(QByteArrayView(ba)); }
    static inline QString fromUtf8(const char *utf8, qsizetype size)
    {
        return fromUtf8(QByteArrayView(utf8, !utf8 || size < 0 ? qstrlen(utf8) : size));
    }
#if defined(__cpp_char8_t) || defined(Q_QDOC)
    Q_WEAK_OVERLOAD
    static inline QString fromUtf8(const char8_t *str)
    { return fromUtf8(reinterpret_cast<const char *>(str)); }
    Q_WEAK_OVERLOAD
    static inline QString fromUtf8(const char8_t *str, qsizetype size)
    { return fromUtf8(reinterpret_cast<const char *>(str), size); }
#endif
    static QString fromLocal8Bit(QByteArrayView ba);
    Q_WEAK_OVERLOAD
    static inline QString fromLocal8Bit(const QByteArray &ba) { return fromLocal8Bit(QByteArrayView(ba)); }
    static inline QString fromLocal8Bit(const char *str, qsizetype size)
    {
        return fromLocal8Bit(QByteArrayView(str, !str || size < 0 ? qstrlen(str) : size));
    }
    static QString fromUtf16(const char16_t *, qsizetype size = -1);
    static QString fromUcs4(const char32_t *, qsizetype size = -1);
    static QString fromRawData(const char16_t *unicode, qsizetype size)
    {
        return QString(DataPointer::fromRawData(unicode, size));
    }
    QT_CORE_INLINE_SINCE(6, 10)
    static QString fromRawData(const QChar *, qsizetype size);

#if QT_DEPRECATED_SINCE(6, 0)
    QT_DEPRECATED_VERSION_X_6_0("Use char16_t* overload.")
    static QString fromUtf16(const ushort *str, qsizetype size = -1)
    { return fromUtf16(reinterpret_cast<const char16_t *>(str), size); }
    QT_DEPRECATED_VERSION_X_6_0("Use char32_t* overload.")
    static QString fromUcs4(const uint *str, qsizetype size = -1)
    { return fromUcs4(reinterpret_cast<const char32_t *>(str), size); }
#endif

    inline qsizetype toWCharArray(wchar_t *array) const;
    [[nodiscard]] static inline QString fromWCharArray(const wchar_t *string, qsizetype size = -1);

    QString &setRawData(const QChar *unicode, qsizetype size);
    QString &setUnicode(const QChar *unicode, qsizetype size);
    Q_WEAK_OVERLOAD
    QString &setUnicode(const char16_t *utf16, qsizetype size)
    { return setUnicode(reinterpret_cast<const QChar *>(utf16), size); }
    QString &setUtf16(const char16_t *utf16, qsizetype size)
    { return setUnicode(reinterpret_cast<const QChar *>(utf16), size); }

#if !QT_CORE_REMOVED_SINCE(6, 9)
    Q_WEAK_OVERLOAD
#endif
    QString &setUtf16(const ushort *autf16, qsizetype asize)
    { return setUnicode(reinterpret_cast<const QChar *>(autf16), asize); }

    int compare(const QString &s, Qt::CaseSensitivity cs = Qt::CaseSensitive) const noexcept;
    int compare(QLatin1StringView other, Qt::CaseSensitivity cs = Qt::CaseSensitive) const noexcept;
    inline int compare(QStringView s, Qt::CaseSensitivity cs = Qt::CaseSensitive) const noexcept;
    int compare(QChar ch, Qt::CaseSensitivity cs = Qt::CaseSensitive) const noexcept
    { return compare(QStringView{&ch, 1}, cs); }

    static inline int compare(const QString &s1, const QString &s2,
                              Qt::CaseSensitivity cs = Qt::CaseSensitive) noexcept
    { return s1.compare(s2, cs); }

    static inline int compare(const QString &s1, QLatin1StringView s2,
                              Qt::CaseSensitivity cs = Qt::CaseSensitive) noexcept
    { return s1.compare(s2, cs); }
    static inline int compare(QLatin1StringView s1, const QString &s2,
                              Qt::CaseSensitivity cs = Qt::CaseSensitive) noexcept
    { return -s2.compare(s1, cs); }
    static int compare(const QString &s1, QStringView s2, Qt::CaseSensitivity cs = Qt::CaseSensitive) noexcept
    { return s1.compare(s2, cs); }
    static int compare(QStringView s1, const QString &s2, Qt::CaseSensitivity cs = Qt::CaseSensitive) noexcept
    { return -s2.compare(s1, cs); }

    int localeAwareCompare(const QString& s) const;
    inline int localeAwareCompare(QStringView s) const;
    static int localeAwareCompare(const QString& s1, const QString& s2)
    { return s1.localeAwareCompare(s2); }

    static inline int localeAwareCompare(QStringView s1, QStringView s2);

    short toShort(bool *ok=nullptr, int base=10) const
    { return toIntegral_helper<short>(*this, ok, base); }
    ushort toUShort(bool *ok=nullptr, int base=10) const
    { return toIntegral_helper<ushort>(*this, ok, base); }
    int toInt(bool *ok=nullptr, int base=10) const
    { return toIntegral_helper<int>(*this, ok, base); }
    uint toUInt(bool *ok=nullptr, int base=10) const
    { return toIntegral_helper<uint>(*this, ok, base); }
    long toLong(bool *ok=nullptr, int base=10) const
    { return toIntegral_helper<long>(*this, ok, base); }
    ulong toULong(bool *ok=nullptr, int base=10) const
    { return toIntegral_helper<ulong>(*this, ok, base); }
    QT_CORE_INLINE_SINCE(6, 5)
    qlonglong toLongLong(bool *ok=nullptr, int base=10) const;
    QT_CORE_INLINE_SINCE(6, 5)
    qulonglong toULongLong(bool *ok=nullptr, int base=10) const;
    float toFloat(bool *ok=nullptr) const;
    double toDouble(bool *ok=nullptr) const;

    inline QString &setNum(short, int base=10);
    inline QString &setNum(ushort, int base=10);
    inline QString &setNum(int, int base=10);
    inline QString &setNum(uint, int base=10);
    inline QString &setNum(long, int base=10);
    inline QString &setNum(ulong, int base=10);
    QString &setNum(qlonglong, int base=10);
    QString &setNum(qulonglong, int base=10);
    inline QString &setNum(float, char format='g', int precision=6);
    QString &setNum(double, char format='g', int precision=6);

    static QString number(int, int base=10);
    static QString number(uint, int base=10);
    static QString number(long, int base=10);
    static QString number(ulong, int base=10);
    static QString number(qlonglong, int base=10);
    static QString number(qulonglong, int base=10);
    static QString number(double, char format='g', int precision=6);

    friend bool comparesEqual(const QString &s1, const QString &s2) noexcept
    { return comparesEqual(QStringView(s1), QStringView(s2)); }
    friend Qt::strong_ordering compareThreeWay(const QString &s1, const QString &s2) noexcept
    { return compareThreeWay(QStringView(s1), QStringView(s2)); }
    Q_DECLARE_STRONGLY_ORDERED(QString)

    Q_WEAK_OVERLOAD
    friend bool comparesEqual(const QString &s1, QUtf8StringView s2) noexcept
    { return QtPrivate::equalStrings(s1, s2); }
    Q_WEAK_OVERLOAD
    friend Qt::strong_ordering compareThreeWay(const QString &s1, QUtf8StringView s2) noexcept
    {
        const int res = QtPrivate::compareStrings(s1, s2, Qt::CaseSensitive);
        return Qt::compareThreeWay(res, 0);
    }
    Q_DECLARE_STRONGLY_ORDERED(QString, QUtf8StringView, Q_WEAK_OVERLOAD)

#ifdef __cpp_char8_t
    friend bool comparesEqual(const QString &s1, const char8_t *s2) noexcept
    { return comparesEqual(s1, QUtf8StringView(s2)); }
    friend Qt::strong_ordering compareThreeWay(const QString &s1, const char8_t *s2) noexcept
    { return compareThreeWay(s1, QUtf8StringView(s2)); }
    Q_DECLARE_STRONGLY_ORDERED(QString, const char8_t *)
#endif // __cpp_char8_t

    friend bool comparesEqual(const QString &s1, QLatin1StringView s2) noexcept
    { return (s1.size() == s2.size()) && QtPrivate::equalStrings(s1, s2); }
    friend Qt::strong_ordering
    compareThreeWay(const QString &s1, QLatin1StringView s2) noexcept
    {
        const int res = QtPrivate::compareStrings(s1, s2, Qt::CaseSensitive);
        return Qt::compareThreeWay(res, 0);
    }
    Q_DECLARE_STRONGLY_ORDERED(QString, QLatin1StringView)

    // Check isEmpty() instead of isNull() for backwards compatibility.
    friend bool comparesEqual(const QString &s1, std::nullptr_t) noexcept
    { return s1.isEmpty(); }
    friend Qt::strong_ordering compareThreeWay(const QString &s1, std::nullptr_t) noexcept
    { return s1.isEmpty() ? Qt::strong_ordering::equivalent : Qt::strong_ordering::greater; }
    Q_DECLARE_STRONGLY_ORDERED(QString, std::nullptr_t)

    friend bool comparesEqual(const QString &s1, const char16_t *s2) noexcept
    { return comparesEqual(s1, QStringView(s2)); }
    friend Qt::strong_ordering compareThreeWay(const QString &s1, const char16_t *s2) noexcept
    { return compareThreeWay(s1, QStringView(s2)); }
    Q_DECLARE_STRONGLY_ORDERED(QString, const char16_t *)

    // QChar <> QString
    friend bool comparesEqual(const QString &lhs, QChar rhs) noexcept
    { return lhs.size() == 1 && rhs == lhs.front(); }
    friend Qt::strong_ordering compareThreeWay(const QString &lhs, QChar rhs) noexcept
    {
        const int res = compare_helper(lhs.data(), lhs.size(), &rhs, 1);
        return Qt::compareThreeWay(res, 0);
    }
    Q_DECLARE_STRONGLY_ORDERED(QString, QChar)

    // ASCII compatibility
#if defined(QT_RESTRICTED_CAST_FROM_ASCII)
    template <qsizetype N>
    inline QString(const char (&ch)[N])
        : QString(fromUtf8(ch))
    {}
    template <qsizetype N>
    QString(char (&)[N]) = delete;
    template <qsizetype N>
    inline QString &operator=(const char (&ch)[N])
    { return (*this = fromUtf8(ch, N - 1)); }
    template <qsizetype N>
    QString &operator=(char (&)[N]) = delete;
#endif
#if !defined(QT_NO_CAST_FROM_ASCII) && !defined(QT_RESTRICTED_CAST_FROM_ASCII)
    QT_ASCII_CAST_WARN inline QString(const char *ch)
        : QString(fromUtf8(ch))
    {}
    QT_ASCII_CAST_WARN inline QString(const QByteArray &a)
        : QString(fromUtf8(a))
    {}
    QT_ASCII_CAST_WARN inline QString &operator=(const char *ch)
    {
        if (!ch) {
            clear();
            return *this;
        }
        return assign(ch);
    }
    QT_ASCII_CAST_WARN inline QString &operator=(const QByteArray &a)
    {
        if (a.isNull()) {
            clear();
            return *this;
        }
        return assign(a);
    }
    // these are needed, so it compiles with STL support enabled
    QT_ASCII_CAST_WARN inline QString &prepend(const char *s)
    { return prepend(QUtf8StringView(s)); }
    QT_ASCII_CAST_WARN inline QString &prepend(const QByteArray &s)
    { return prepend(QUtf8StringView(s)); }
    QT_ASCII_CAST_WARN inline QString &append(const char *s)
    { return append(QUtf8StringView(s)); }
    QT_ASCII_CAST_WARN inline QString &append(const QByteArray &s)
    { return append(QUtf8StringView(s)); }
    QT_ASCII_CAST_WARN inline QString &insert(qsizetype i, const char *s)
    { return insert(i, QUtf8StringView(s)); }
    QT_ASCII_CAST_WARN inline QString &insert(qsizetype i, const QByteArray &s)
    { return insert(i, QUtf8StringView(s)); }
    QT_ASCII_CAST_WARN inline QString &operator+=(const char *s)
    { return append(QUtf8StringView(s)); }
    QT_ASCII_CAST_WARN inline QString &operator+=(const QByteArray &s)
    { return append(QUtf8StringView(s)); }

#if QT_CORE_REMOVED_SINCE(6, 8)
    QT_ASCII_CAST_WARN inline bool operator==(const char *s) const;
    QT_ASCII_CAST_WARN inline bool operator!=(const char *s) const;
    QT_ASCII_CAST_WARN inline bool operator<(const char *s) const;
    QT_ASCII_CAST_WARN inline bool operator<=(const char *s) const;
    QT_ASCII_CAST_WARN inline bool operator>(const char *s) const;
    QT_ASCII_CAST_WARN inline bool operator>=(const char *s) const;

    QT_ASCII_CAST_WARN inline bool operator==(const QByteArray &s) const;
    QT_ASCII_CAST_WARN inline bool operator!=(const QByteArray &s) const;
    QT_ASCII_CAST_WARN inline bool operator<(const QByteArray &s) const;
    QT_ASCII_CAST_WARN inline bool operator>(const QByteArray &s) const;
    QT_ASCII_CAST_WARN inline bool operator<=(const QByteArray &s) const;
    QT_ASCII_CAST_WARN inline bool operator>=(const QByteArray &s) const;
#else
    friend bool comparesEqual(const QString &lhs, QByteArrayView rhs) noexcept
    {
        return QString::compare_helper(lhs.constData(), lhs.size(),
                                       rhs.constData(), rhs.size()) == 0;
    }
    friend Qt::strong_ordering
    compareThreeWay(const QString &lhs, QByteArrayView rhs) noexcept
    {
        const int res = QString::compare_helper(lhs.constData(), lhs.size(),
                                                rhs.constData(), rhs.size());
        return Qt::compareThreeWay(res, 0);
    }
    Q_DECLARE_STRONGLY_ORDERED(QString, QByteArrayView, QT_ASCII_CAST_WARN)

    friend bool comparesEqual(const QString &lhs, const QByteArray &rhs) noexcept
    { return comparesEqual(lhs, QByteArrayView(rhs)); }
    friend Qt::strong_ordering
    compareThreeWay(const QString &lhs, const QByteArray &rhs) noexcept
    {
        return compareThreeWay(lhs, QByteArrayView(rhs));
    }
    Q_DECLARE_STRONGLY_ORDERED(QString, QByteArray, QT_ASCII_CAST_WARN)

    friend bool comparesEqual(const QString &lhs, const char *rhs) noexcept
    { return comparesEqual(lhs, QByteArrayView(rhs)); }
    friend Qt::strong_ordering
    compareThreeWay(const QString &lhs, const char *rhs) noexcept
    {
        return compareThreeWay(lhs, QByteArrayView(rhs));
    }
    Q_DECLARE_STRONGLY_ORDERED(QString, const char *, QT_ASCII_CAST_WARN)
#endif // QT_CORE_REMOVED_SINCE(6, 8)

#endif // !defined(QT_NO_CAST_FROM_ASCII) && !defined(QT_RESTRICTED_CAST_FROM_ASCII)

    typedef QChar *iterator;
    typedef const QChar *const_iterator;
    typedef iterator Iterator;
    typedef const_iterator ConstIterator;
    typedef std::reverse_iterator<iterator> reverse_iterator;
    typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
    inline iterator begin();
    inline const_iterator begin() const;
    inline const_iterator cbegin() const;
    inline const_iterator constBegin() const;
    inline iterator end();
    inline const_iterator end() const;
    inline const_iterator cend() const;
    inline const_iterator constEnd() const;
    reverse_iterator rbegin() { return reverse_iterator(end()); }
    reverse_iterator rend() { return reverse_iterator(begin()); }
    const_reverse_iterator rbegin() const { return const_reverse_iterator(end()); }
    const_reverse_iterator rend() const { return const_reverse_iterator(begin()); }
    const_reverse_iterator crbegin() const { return const_reverse_iterator(end()); }
    const_reverse_iterator crend() const { return const_reverse_iterator(begin()); }

    // STL compatibility
    typedef qsizetype size_type;
    typedef qptrdiff difference_type;
    typedef const QChar & const_reference;
    typedef QChar & reference;
    typedef QChar *pointer;
    typedef const QChar *const_pointer;
    typedef QChar value_type;
    inline void push_back(QChar c) { append(c); }
    inline void push_back(const QString &s) { append(s); }
    inline void push_front(QChar c) { prepend(c); }
    inline void push_front(const QString &s) { prepend(s); }
    void shrink_to_fit() { squeeze(); }
    iterator erase(const_iterator first, const_iterator last);
    inline iterator erase(const_iterator it) { return erase(it, it + 1); }
    constexpr qsizetype max_size() const noexcept
    {
        return maxSize();
    }

    static inline QString fromStdString(const std::string &s);
    std::string toStdString() const;
    static inline QString fromStdWString(const std::wstring &s);
    inline std::wstring toStdWString() const;

    static inline QString fromStdU16String(const std::u16string &s);
    inline std::u16string toStdU16String() const;
    static inline QString fromStdU32String(const std::u32string &s);
    inline std::u32string toStdU32String() const;

    Q_IMPLICIT inline operator std::u16string_view() const noexcept;

#if defined(Q_OS_DARWIN) || defined(Q_QDOC)
    static QString fromCFString(CFStringRef string);
    CFStringRef toCFString() const Q_DECL_CF_RETURNS_RETAINED;
    static QString fromNSString(const NSString *string);
    NSString *toNSString() const Q_DECL_NS_RETURNS_AUTORELEASED;
#endif

#if defined(Q_OS_WASM) || defined(Q_QDOC)
    static QString fromEcmaString(emscripten::val jsString);
    emscripten::val toEcmaString() const;
#endif

    constexpr bool isNull() const { return d.isNull(); }

    bool isRightToLeft() const;
    [[nodiscard]] bool isValidUtf16() const noexcept
    { return QStringView(*this).isValidUtf16(); }

    QString(qsizetype size, Qt::Initialization);
    explicit QString(DataPointer &&dd) : d(std::move(dd)) {}

private:
#if defined(QT_NO_CAST_FROM_ASCII)
#define QSTRING_DECL_DELETED_ASCII_OP Q_DECL_EQ_DELETE_X("This function is not available under QT_NO_CAST_FROM_ASCII")
    QString &operator+=(const char *s) QSTRING_DECL_DELETED_ASCII_OP;
    QString &operator+=(const QByteArray &s) QSTRING_DECL_DELETED_ASCII_OP;
    QString(const char *ch) QSTRING_DECL_DELETED_ASCII_OP;
    QString(const QByteArray &a) QSTRING_DECL_DELETED_ASCII_OP;
    QString &operator=(const char  *ch) QSTRING_DECL_DELETED_ASCII_OP;
    QString &operator=(const QByteArray &a) QSTRING_DECL_DELETED_ASCII_OP;
#undef QSTRING_DECL_DELETED_ASCII_OP
#endif

    DataPointer d;
    static const char16_t _empty;

    void reallocData(qsizetype alloc, QArrayData::AllocationOption option);
    void reallocGrowData(qsizetype n);
    // ### remove once QAnyStringView supports UTF-32:
    QString &assign_helper(const char32_t *data, qsizetype len);
    // Defined in qstringconverter.h
    template <typename InputIterator>
    void assign_helper_char8(InputIterator first, InputIterator last);
    static int compare_helper(const QChar *data1, qsizetype length1,
                              const QChar *data2, qsizetype length2,
                              Qt::CaseSensitivity cs = Qt::CaseSensitive) noexcept;
    static int compare_helper(const QChar *data1, qsizetype length1,
                              const char *data2, qsizetype length2,
                              Qt::CaseSensitivity cs = Qt::CaseSensitive) noexcept;
    static int localeAwareCompare_helper(const QChar *data1, qsizetype length1,
                                         const QChar *data2, qsizetype length2);
    static QString sliced_helper(QString &str, qsizetype pos, qsizetype n);
    static QString toLower_helper(const QString &str);
    static QString toLower_helper(QString &str);
    static QString toUpper_helper(const QString &str);
    static QString toUpper_helper(QString &str);
    static QString toCaseFolded_helper(const QString &str);
    static QString toCaseFolded_helper(QString &str);
    static QString trimmed_helper(const QString &str);
    static QString trimmed_helper(QString &str);
    static QString simplified_helper(const QString &str);
    static QString simplified_helper(QString &str);
    static QByteArray toLatin1_helper(const QString &);
    static QByteArray toLatin1_helper_inplace(QString &);
    static QByteArray toUtf8_helper(const QString &);
    static QByteArray toLocal8Bit_helper(const QChar *data, qsizetype size);
#if QT_CORE_REMOVED_SINCE(6, 6)
    static qsizetype toUcs4_helper(const ushort *uc, qsizetype length, uint *out);
#endif
    static qsizetype toUcs4_helper(const char16_t *uc, qsizetype length, char32_t *out);
    static qlonglong toIntegral_helper(QStringView string, bool *ok, int base);
    static qulonglong toIntegral_helper(QStringView string, bool *ok, uint base);
    template <typename Predicate>
    qsizetype removeIf_helper(Predicate pred)
    {
        const qsizetype result = d->eraseIf(pred);
        if (result > 0)
            d.data()[d.size] = u'\0';
        return result;
    }

    friend class QStringView;
    friend class QByteArray;
    friend struct QAbstractConcatenable;
    template <typename T> friend qsizetype erase(QString &s, const T &t);
    template <typename Predicate> friend qsizetype erase_if(QString &s, Predicate pred);

    template <typename T> static
    T toIntegral_helper(QStringView string, bool *ok, int base)
    {
        using Int64 = typename std::conditional<std::is_unsigned<T>::value, qulonglong, qlonglong>::type;
        using Int32 = typename std::conditional<std::is_unsigned<T>::value, uint, int>::type;

        // we select the right overload by casting base to int or uint
        Int64 val = toIntegral_helper(string, ok, Int32(base));
        if (T(val) != val) {
            if (ok)
                *ok = false;
            val = 0;
        }
        return T(val);
    }

    Q_ALWAYS_INLINE constexpr void verify([[maybe_unused]] qsizetype pos = 0,
                                          [[maybe_unused]] qsizetype n = 1) const
    {
        Q_ASSERT(pos >= 0);
        Q_ASSERT(pos <= d.size);
        Q_ASSERT(n >= 0);
        Q_ASSERT(n <= d.size - pos);
    }

public:
    inline DataPointer &data_ptr() { return d; }
    inline const DataPointer &data_ptr() const { return d; }
};

//
// QLatin1StringView inline members that require QUtf8StringView:
//

int QLatin1StringView::compare(QUtf8StringView other, Qt::CaseSensitivity cs) const noexcept
{ return QtPrivate::compareStrings(*this, other, cs); }

//
// QLatin1StringView inline members that require QString:
//

QString QLatin1StringView::toString() const { return *this; }

//
// QStringView inline members that require QUtf8StringView:
//

int QStringView::compare(QUtf8StringView other, Qt::CaseSensitivity cs) const noexcept
{ return QtPrivate::compareStrings(*this, other, cs); }

//
// QStringView inline members that require QString:
//

QString QStringView::toString() const
{ return QString(*this); }

qint64 QStringView::toLongLong(bool *ok, int base) const
{ return QString::toIntegral_helper<qint64>(*this, ok, base); }
quint64 QStringView::toULongLong(bool *ok, int base) const
{ return QString::toIntegral_helper<quint64>(*this, ok, base); }
long QStringView::toLong(bool *ok, int base) const
{ return QString::toIntegral_helper<long>(*this, ok, base); }
ulong QStringView::toULong(bool *ok, int base) const
{ return QString::toIntegral_helper<ulong>(*this, ok, base); }
int QStringView::toInt(bool *ok, int base) const
{ return QString::toIntegral_helper<int>(*this, ok, base); }
uint QStringView::toUInt(bool *ok, int base) const
{ return QString::toIntegral_helper<uint>(*this, ok, base); }
short QStringView::toShort(bool *ok, int base) const
{ return QString::toIntegral_helper<short>(*this, ok, base); }
ushort QStringView::toUShort(bool *ok, int base) const
{ return QString::toIntegral_helper<ushort>(*this, ok, base); }

//
// QUtf8StringView inline members that require QStringView:
//

template <bool UseChar8T>
int QBasicUtf8StringView<UseChar8T>::compare(QChar other, Qt::CaseSensitivity cs) const noexcept
{
    return QtPrivate::compareStrings(*this, QStringView(&other, 1), cs);
}

template <bool UseChar8T>
int QBasicUtf8StringView<UseChar8T>::compare(QStringView other, Qt::CaseSensitivity cs) const noexcept
{
    return QtPrivate::compareStrings(*this, other, cs);
}

template <bool UseChar8T>
[[nodiscard]] bool QBasicUtf8StringView<UseChar8T>::equal(QChar other) const noexcept
{
    return QtPrivate::equalStrings(*this, QStringView(&other, 1));
}

template <bool UseChar8T>
[[nodiscard]] bool QBasicUtf8StringView<UseChar8T>::equal(QStringView other) const noexcept
{
    return QtPrivate::equalStrings(*this, other);
}

//
// QUtf8StringView inline members that require QString, QL1SV or QBA:
//

template <bool UseChar8T>
QString QBasicUtf8StringView<UseChar8T>::toString() const
{
    return QString::fromUtf8(data(), size());
}

template<bool UseChar8T>
[[nodiscard]] int QBasicUtf8StringView<UseChar8T>::compare(QLatin1StringView other,
                                                           Qt::CaseSensitivity cs) const noexcept
{
    return QtPrivate::compareStrings(*this, other, cs);
}

template<bool UseChar8T>
[[nodiscard]] int QBasicUtf8StringView<UseChar8T>::compare(const QByteArray &other,
                                                           Qt::CaseSensitivity cs) const noexcept
{
    return QtPrivate::compareStrings(*this,
                                     QBasicUtf8StringView<UseChar8T>(other.data(), other.size()),
                                     cs);
}

template <bool UseChar8T>
[[nodiscard]] bool QBasicUtf8StringView<UseChar8T>::equal(QLatin1StringView other) const noexcept
{
    return QtPrivate::equalStrings(*this, other);
}

template <bool UseChar8T>
[[nodiscard]] bool QBasicUtf8StringView<UseChar8T>::equal(const QByteArray &other) const noexcept
{
    return size() == other.size()
            && QtPrivate::equalStrings(*this, QBasicUtf8StringView<UseChar8T>(other.data(),
                                                                              other.size()));
}

//
// QAnyStringView inline members that require QString:
//

QAnyStringView::QAnyStringView(const QByteArray &str) noexcept
    : QAnyStringView{str.begin(), str.size()} {}
QAnyStringView::QAnyStringView(const QString &str) noexcept
    : QAnyStringView{str.begin(), str.size()} {}

QString QAnyStringView::toString() const
{ return QtPrivate::convertToQString(*this); }

//
// QString inline members
//
QString::QString(QLatin1StringView latin1)
    : QString{QString::fromLatin1(latin1.data(), latin1.size())} {}
const QChar QString::at(qsizetype i) const
{ verify(i, 1); return QChar(d.data()[i]); }
const QChar QString::operator[](qsizetype i) const
{ verify(i, 1); return QChar(d.data()[i]); }
const QChar *QString::unicode() const
{ return data(); }
const QChar *QString::data() const
{
#if QT5_NULL_STRINGS == 1
    return reinterpret_cast<const QChar *>(d.data() ? d.data() : &_empty);
#else
    return reinterpret_cast<const QChar *>(d.data());
#endif
}
QChar *QString::data()
{
    detach();
    Q_ASSERT(d.data());
    return reinterpret_cast<QChar *>(d.data());
}
const QChar *QString::constData() const
{ return data(); }
void QString::detach()
{ if (d.needsDetach()) reallocData(d.size, QArrayData::KeepSize); }
bool QString::isDetached() const
{ return !d.isShared(); }
void QString::clear()
{ if (!isNull()) *this = QString(); }
QString::QString(const QString &other) noexcept : d(other.d)
{ }
qsizetype QString::capacity() const { return qsizetype(d.constAllocatedCapacity()); }
QString &QString::setNum(short n, int base)
{ return setNum(qlonglong(n), base); }
QString &QString::setNum(ushort n, int base)
{ return setNum(qulonglong(n), base); }
QString &QString::setNum(int n, int base)
{ return setNum(qlonglong(n), base); }
QString &QString::setNum(uint n, int base)
{ return setNum(qulonglong(n), base); }
QString &QString::setNum(long n, int base)
{ return setNum(qlonglong(n), base); }
QString &QString::setNum(ulong n, int base)
{ return setNum(qulonglong(n), base); }
QString &QString::setNum(float n, char f, int prec)
{ return setNum(double(n),f,prec); }
#if QT_CORE_REMOVED_SINCE(6, 9)
QString QString::arg(int a, int fieldWidth, int base, QChar fillChar) const
{ return arg(qlonglong(a), fieldWidth, base, fillChar); }
QString QString::arg(uint a, int fieldWidth, int base, QChar fillChar) const
{ return arg(qulonglong(a), fieldWidth, base, fillChar); }
QString QString::arg(long a, int fieldWidth, int base, QChar fillChar) const
{ return arg(qlonglong(a), fieldWidth, base, fillChar); }
QString QString::arg(ulong a, int fieldWidth, int base, QChar fillChar) const
{ return arg(qulonglong(a), fieldWidth, base, fillChar); }
QString QString::arg(short a, int fieldWidth, int base, QChar fillChar) const
{ return arg(qlonglong(a), fieldWidth, base, fillChar); }
QString QString::arg(ushort a, int fieldWidth, int base, QChar fillChar) const
{ return arg(qulonglong(a), fieldWidth, base, fillChar); }
#endif // QT_CORE_REMOVED_SINCE

QString QString::section(QChar asep, qsizetype astart, qsizetype aend, SectionFlags aflags) const
{ return section(QString(asep), astart, aend, aflags); }

QT_WARNING_PUSH
QT_WARNING_DISABLE_MSVC(4127)   // "conditional expression is constant"
QT_WARNING_DISABLE_INTEL(111)   // "statement is unreachable"

qsizetype QString::toWCharArray(wchar_t *array) const
{
    return qToStringViewIgnoringNull(*this).toWCharArray(array);
}

qsizetype QStringView::toWCharArray(wchar_t *array) const
{
    if (sizeof(wchar_t) == sizeof(QChar)) {
        if (auto src = data())
            memcpy(array, src, sizeof(QChar) * size());
        return size();
    } else {
        return QString::toUcs4_helper(utf16(), size(), reinterpret_cast<char32_t *>(array));
    }
}

QT_WARNING_POP

QString QString::fromWCharArray(const wchar_t *string, qsizetype size)
{
    if constexpr (sizeof(wchar_t) == sizeof(QChar)) {
        return QString(reinterpret_cast<const QChar *>(string), size);
    } else {
#ifdef QT_BOOTSTRAPPED
        Q_UNREACHABLE_RETURN(QString());
#else
        return fromUcs4(reinterpret_cast<const char32_t *>(string), size);
#endif
    }
}

constexpr QString::QString() noexcept {}
QString::~QString() {}

void QString::reserve(qsizetype asize)
{
    if (d.needsDetach() || asize >= capacity() - d.freeSpaceAtBegin())
        reallocData(qMax(asize, size()), QArrayData::KeepSize);
    if (d.constAllocatedCapacity())
        d.setFlag(Data::CapacityReserved);
}

void QString::squeeze()
{
    if (!d.isMutable())
        return;
    if (d.needsDetach() || size() < capacity())
        reallocData(d.size, QArrayData::KeepSize);
    if (d.constAllocatedCapacity())
        d.clearFlag(Data::CapacityReserved);
}

QChar &QString::operator[](qsizetype i)
{ verify(i, 1); return data()[i]; }
QChar &QString::front() { return operator[](0); }
QChar &QString::back() { return operator[](size() - 1); }
QString::iterator QString::begin()
{ detach(); return reinterpret_cast<QChar*>(d.data()); }
QString::const_iterator QString::begin() const
{ return reinterpret_cast<const QChar*>(d.data()); }
QString::const_iterator QString::cbegin() const
{ return reinterpret_cast<const QChar*>(d.data()); }
QString::const_iterator QString::constBegin() const
{ return reinterpret_cast<const QChar*>(d.data()); }
QString::iterator QString::end()
{ detach(); return reinterpret_cast<QChar*>(d.data() + d.size); }
QString::const_iterator QString::end() const
{ return reinterpret_cast<const QChar*>(d.data() + d.size); }
QString::const_iterator QString::cend() const
{ return reinterpret_cast<const QChar*>(d.data() + d.size); }
QString::const_iterator QString::constEnd() const
{ return reinterpret_cast<const QChar*>(d.data() + d.size); }
bool QString::contains(const QString &s, Qt::CaseSensitivity cs) const
{ return indexOf(s, 0, cs) != -1; }
bool QString::contains(QLatin1StringView s, Qt::CaseSensitivity cs) const
{ return indexOf(s, 0, cs) != -1; }
bool QString::contains(QChar c, Qt::CaseSensitivity cs) const
{ return indexOf(c, 0, cs) != -1; }
bool QString::contains(QStringView s, Qt::CaseSensitivity cs) const noexcept
{ return indexOf(s, 0, cs) != -1; }

#if !defined(QT_NO_CAST_FROM_ASCII) && !defined(QT_RESTRICTED_CAST_FROM_ASCII)
#if QT_CORE_REMOVED_SINCE(6, 8)
bool QString::operator==(const char *s) const
{ return QString::compare_helper(constData(), size(), s, -1) == 0; }
bool QString::operator!=(const char *s) const
{ return QString::compare_helper(constData(), size(), s, -1) != 0; }
bool QString::operator<(const char *s) const
{ return QString::compare_helper(constData(), size(), s, -1) < 0; }
bool QString::operator>(const char *s) const
{ return QString::compare_helper(constData(), size(), s, -1) > 0; }
bool QString::operator<=(const char *s) const
{ return QString::compare_helper(constData(), size(), s, -1) <= 0; }
bool QString::operator>=(const char *s) const
{ return QString::compare_helper(constData(), size(), s, -1) >= 0; }

QT_ASCII_CAST_WARN bool QString::operator==(const QByteArray &s) const
{ return QString::compare_helper(constData(), size(), s.constData(), s.size()) == 0; }
QT_ASCII_CAST_WARN bool QString::operator!=(const QByteArray &s) const
{ return QString::compare_helper(constData(), size(), s.constData(), s.size()) != 0; }
QT_ASCII_CAST_WARN bool QString::operator<(const QByteArray &s) const
{ return QString::compare_helper(constData(), size(), s.constData(), s.size()) < 0; }
QT_ASCII_CAST_WARN bool QString::operator>(const QByteArray &s) const
{ return QString::compare_helper(constData(), size(), s.constData(), s.size()) > 0; }
QT_ASCII_CAST_WARN bool QString::operator<=(const QByteArray &s) const
{ return QString::compare_helper(constData(), size(), s.constData(), s.size()) <= 0; }
QT_ASCII_CAST_WARN bool QString::operator>=(const QByteArray &s) const
{ return QString::compare_helper(constData(), size(), s.constData(), s.size()) >= 0; }

bool QByteArray::operator==(const QString &s) const
{ return QString::compare_helper(s.constData(), s.size(), constData(), size()) == 0; }
bool QByteArray::operator!=(const QString &s) const
{ return QString::compare_helper(s.constData(), s.size(), constData(), size()) != 0; }
bool QByteArray::operator<(const QString &s) const
{ return QString::compare_helper(s.constData(), s.size(), constData(), size()) > 0; }
bool QByteArray::operator>(const QString &s) const
{ return QString::compare_helper(s.constData(), s.size(), constData(), size()) < 0; }
bool QByteArray::operator<=(const QString &s) const
{ return QString::compare_helper(s.constData(), s.size(), constData(), size()) >= 0; }
bool QByteArray::operator>=(const QString &s) const
{ return QString::compare_helper(s.constData(), s.size(), constData(), size()) <= 0; }
#endif // QT_CORE_REMOVED_SINCE(6, 8)
#endif // !defined(QT_NO_CAST_FROM_ASCII) && !defined(QT_RESTRICTED_CAST_FROM_ASCII)

#if !defined(QT_USE_FAST_OPERATOR_PLUS) && !defined(QT_USE_QSTRINGBUILDER)
// QString + QString
inline QString operator+(const QString &s1, const QString &s2)
{ QString t(s1); t += s2; return t; }
inline QString operator+(QString &&lhs, const QString &rhs)
{ return std::move(lhs += rhs); }
inline QString operator+(const QString &s1, QChar s2)
{ QString t(s1); t += s2; return t; }
inline QString operator+(QString &&lhs, QChar rhs)
{ return std::move(lhs += rhs); }
inline QString operator+(QChar s1, const QString &s2)
{ QString t(s1); t += s2; return t; }
inline QString operator+(const QString &lhs, QStringView rhs)
{
    QString ret{lhs.size() + rhs.size(), Qt::Uninitialized};
    return ret.assign(lhs).append(rhs);
}
inline QString operator+(QStringView lhs, const QString &rhs)
{
    QString ret{lhs.size() + rhs.size(), Qt::Uninitialized};
    return ret.assign(lhs).append(rhs);
}

#  if !defined(QT_NO_CAST_FROM_ASCII) && !defined(QT_RESTRICTED_CAST_FROM_ASCII)
QT_ASCII_CAST_WARN inline QString operator+(const QString &s1, const char *s2)
{ QString t(s1); t += QUtf8StringView(s2); return t; }
QT_ASCII_CAST_WARN inline QString operator+(QString &&lhs, const char *rhs)
{ QT_IGNORE_DEPRECATIONS(return std::move(lhs += rhs);) }
QT_ASCII_CAST_WARN inline QString operator+(const char *s1, const QString &s2)
{ QString t = QString::fromUtf8(s1); t += s2; return t; }
QT_ASCII_CAST_WARN inline QString operator+(const QByteArray &ba, const QString &s)
{ QString t = QString::fromUtf8(ba); t += s; return t; }
QT_ASCII_CAST_WARN inline QString operator+(const QString &s, const QByteArray &ba)
{ QString t(s); t += QUtf8StringView(ba); return t; }
QT_ASCII_CAST_WARN inline QString operator+(QString &&lhs, const QByteArray &rhs)
{ QT_IGNORE_DEPRECATIONS(return std::move(lhs += rhs);) }
#  endif // QT_NO_CAST_FROM_ASCII
#endif // QT_USE_QSTRINGBUILDER

QString QString::fromStdString(const std::string &s)
{ return fromUtf8(s.data(), qsizetype(s.size())); }

std::wstring QString::toStdWString() const
{
    std::wstring str;
    str.resize(size());
    str.resize(toWCharArray(str.data()));
    return str;
}

QString QString::fromStdWString(const std::wstring &s)
{ return fromWCharArray(s.data(), qsizetype(s.size())); }

QString QString::fromStdU16String(const std::u16string &s)
{ return fromUtf16(s.data(), qsizetype(s.size())); }

std::u16string QString::toStdU16String() const
{ return std::u16string(reinterpret_cast<const char16_t*>(data()), size()); }

QString QString::fromStdU32String(const std::u32string &s)
{ return fromUcs4(s.data(), qsizetype(s.size())); }

std::u32string QString::toStdU32String() const
{
    std::u32string u32str(size(), char32_t(0));
    const qsizetype len = toUcs4_helper(reinterpret_cast<const char16_t *>(data()),
                                        size(), u32str.data());
    u32str.resize(len);
    return u32str;
}

QString::operator std::u16string_view() const noexcept
{
    return std::u16string_view(d.data(), size_t(d.size));
}

#if !defined(QT_NO_DATASTREAM) || defined(QT_BOOTSTRAPPED)
Q_CORE_EXPORT QDataStream &operator<<(QDataStream &, const QString &);
Q_CORE_EXPORT QDataStream &operator>>(QDataStream &, QString &);
#endif

Q_DECLARE_SHARED(QString)
Q_DECLARE_OPERATORS_FOR_FLAGS(QString::SectionFlags)

int QString::compare(QStringView s, Qt::CaseSensitivity cs) const noexcept
{ return -s.compare(*this, cs); }

int QString::localeAwareCompare(QStringView s) const
{ return localeAwareCompare_helper(constData(), size(), s.constData(), s.size()); }
int QString::localeAwareCompare(QStringView s1, QStringView s2)
{ return localeAwareCompare_helper(s1.constData(), s1.size(), s2.constData(), s2.size()); }
int QStringView::localeAwareCompare(QStringView other) const
{ return QString::localeAwareCompare(*this, other); }

#if QT_CORE_INLINE_IMPL_SINCE(6, 5)
qint64 QString::toLongLong(bool *ok, int base) const
{
    return toIntegral_helper<qlonglong>(*this, ok, base);
}

quint64 QString::toULongLong(bool *ok, int base) const
{
    return toIntegral_helper<qulonglong>(*this, ok, base);
}
#endif
#if QT_CORE_INLINE_IMPL_SINCE(6, 8)
qsizetype QString::indexOf(QChar ch, qsizetype from, Qt::CaseSensitivity cs) const
{
    return qToStringViewIgnoringNull(*this).indexOf(ch, from, cs);
}
qsizetype QString::lastIndexOf(QChar ch, qsizetype from, Qt::CaseSensitivity cs) const
{
    return qToStringViewIgnoringNull(*this).lastIndexOf(ch, from, cs);
}
#endif
#if QT_CORE_INLINE_IMPL_SINCE(6, 10)
QString QString::fromRawData(const QChar *unicode, qsizetype size)
{
    return fromRawData(reinterpret_cast<const char16_t *>(unicode), size);
}
#endif

namespace QtPrivate {
// used by qPrintable() and qUtf8Printable() macros
inline const QString &asString(const QString &s)    { return s; }
inline QString &&asString(QString &&s)              { return std::move(s); }
}

#ifndef qPrintable
#  define qPrintable(string) QtPrivate::asString(string).toLocal8Bit().constData()
#endif

#ifndef qUtf8Printable
#  define qUtf8Printable(string) QtPrivate::asString(string).toUtf8().constData()
#endif

/*
    Wrap QString::constData() with enough casts to allow passing it
    to QString::asprintf("%ls") without warnings.
*/
#ifndef qUtf16Printable
#  define qUtf16Printable(string) \
    static_cast<const wchar_t *>( \
        static_cast<const void *>(QtPrivate::asString(string).nullTerminated().constData()))
#endif

//
// QStringView::arg() implementation
//

namespace QtPrivate {

struct ArgBase {
    enum Tag : uchar { L1, Any, U16 } tag;
};

struct QStringViewArg : ArgBase {
    QStringView string;
    QStringViewArg() = default;
    constexpr explicit QStringViewArg(QStringView v) noexcept : ArgBase{U16}, string{v} {}
};

struct QLatin1StringArg : ArgBase {
    QLatin1StringView string;
    QLatin1StringArg() = default;
    constexpr explicit QLatin1StringArg(QLatin1StringView v) noexcept : ArgBase{L1}, string{v} {}
};

struct QAnyStringArg : ArgBase {
    QAnyStringView string;
    QAnyStringArg() = default;
    constexpr explicit QAnyStringArg(QAnyStringView v) noexcept : ArgBase{Any}, string{v} {}
};

#if QT_CORE_REMOVED_SINCE(6, 9)
[[nodiscard]] Q_CORE_EXPORT QString argToQString(QStringView pattern, size_t n, const ArgBase **args);
[[nodiscard]] Q_CORE_EXPORT QString argToQString(QLatin1StringView pattern, size_t n, const ArgBase **args);
#endif
[[nodiscard]] Q_CORE_EXPORT QString argToQString(QAnyStringView pattern, size_t n, const ArgBase **args);

template <typename...Args>
[[nodiscard]] Q_ALWAYS_INLINE QString argToQStringDispatch(QAnyStringView pattern, const Args &...args)
{
    const ArgBase *argBases[] = {&args..., /* avoid zero-sized array */ nullptr};
    return QtPrivate::argToQString(pattern, sizeof...(Args), argBases);
}

constexpr inline QAnyStringArg qStringLikeToArg(QAnyStringView s) noexcept { return QAnyStringArg{s}; }

} // namespace QtPrivate

template <typename...Args>
Q_ALWAYS_INLINE
QString QStringView::arg(Args &&...args) const
{
    return QtPrivate::argToQStringDispatch(*this, QtPrivate::qStringLikeToArg(args)...);
}

template <typename...Args>
Q_ALWAYS_INLINE
QString QLatin1StringView::arg(Args &&...args) const
{
    return QtPrivate::argToQStringDispatch(*this, QtPrivate::qStringLikeToArg(args)...);
}

template <bool HasChar8T>
template <typename...Args>
QString QBasicUtf8StringView<HasChar8T>::arg(Args &&...args) const
{
    return QtPrivate::argToQStringDispatch(*this, QtPrivate::qStringLikeToArg(args)...);
}

template <typename...Args>
QString QAnyStringView::arg(Args &&...args) const
{
    return QtPrivate::argToQStringDispatch(*this, QtPrivate::qStringLikeToArg(args)...);
}

template <typename T>
qsizetype erase(QString &s, const T &t)
{
    return s.removeIf_helper([&t](const auto &e) { return t == e; });
}

template <typename Predicate>
qsizetype erase_if(QString &s, Predicate pred)
{
    return s.removeIf_helper(pred);
}

namespace Qt {
inline namespace Literals {
inline namespace StringLiterals {
inline QString operator""_s(const char16_t *str, size_t size) noexcept
{
    return QString(QStringPrivate(nullptr, const_cast<char16_t *>(str), qsizetype(size)));
}

} // StringLiterals
} // Literals
} // Qt

inline namespace QtLiterals {
#if QT_DEPRECATED_SINCE(6, 8)

QT_DEPRECATED_VERSION_X_6_8("Use _s from Qt::StringLiterals namespace instead.")
inline QString operator""_qs(const char16_t *str, size_t size) noexcept
{
    return Qt::StringLiterals::operator""_s(str, size);
}

#endif // QT_DEPRECATED_SINCE(6, 8)
} // QtLiterals

// all our supported compilers support Unicode string literals,
// even if their Q_COMPILER_UNICODE_STRING has been revoked due
// to lacking stdlib support. But QStringLiteral only needs the
// core language feature, so just use u"" here unconditionally:

#define QT_UNICODE_LITERAL(str) u"" str

namespace QtPrivate {
template <qsizetype N>
Q_ALWAYS_INLINE static QStringPrivate qMakeStringPrivate(const char16_t (&literal)[N])
{
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
    auto str = const_cast<char16_t *>(literal);
    return { nullptr, str, N - 1 };
}
} // namespace QtPrivate

#define QStringLiteral(str) (QString(QtPrivate::qMakeStringPrivate(QT_UNICODE_LITERAL(str)))) /**/

QT_END_NAMESPACE

#include <QtCore/qstringbuilder.h>
#include <QtCore/qstringconverter.h>

#ifdef Q_L1S_VIEW_IS_PRIMARY
#    undef Q_L1S_VIEW_IS_PRIMARY
#endif

#endif // QSTRING_H
