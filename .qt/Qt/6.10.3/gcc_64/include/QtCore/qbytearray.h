// Copyright (C) 2022 The Qt Company Ltd.
// Copyright (C) 2016 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:data-parser

#ifndef QBYTEARRAY_H
#define QBYTEARRAY_H

#include <QtCore/qrefcount.h>
#include <QtCore/qnamespace.h>
#include <QtCore/qarraydata.h>
#include <QtCore/qarraydatapointer.h>
#include <QtCore/qcompare.h>
#include <QtCore/qcontainerfwd.h>
#include <QtCore/qbytearrayalgorithms.h>
#include <QtCore/qbytearrayview.h>

#include <stdlib.h>
#include <string.h>

#include <string>
#include <iterator>

#ifndef QT5_NULL_STRINGS
// Would ideally be off, but in practice breaks too much (Qt 6.0).
#define QT5_NULL_STRINGS 1
#endif

#ifdef truncate
#error qbytearray.h must be included before any header file that defines truncate
#endif

#if defined(Q_OS_DARWIN) || defined(Q_QDOC)
Q_FORWARD_DECLARE_CF_TYPE(CFData);
Q_FORWARD_DECLARE_OBJC_CLASS(NSData);
#endif

#if defined(Q_OS_WASM) || defined(Q_QDOC)
namespace emscripten {
    class val;
}
#endif

class tst_QByteArray;

QT_BEGIN_NAMESPACE

class QString;
class QDataStream;

using QByteArrayData = QArrayDataPointer<char>;

#  define QByteArrayLiteral(str) \
    (QByteArray(QByteArrayData(nullptr, const_cast<char *>(str), sizeof(str) - 1))) \
    /**/

class Q_CORE_EXPORT QByteArray
{
public:
    using DataPointer = QByteArrayData;
private:
    typedef QTypedArrayData<char> Data;

    DataPointer d;
    static const char _empty;

    friend class ::tst_QByteArray;

    template <typename InputIterator>
    using if_input_iterator = QtPrivate::IfIsInputIterator<InputIterator>;
public:
    enum Base64Option {
        Base64Encoding = 0,
        Base64UrlEncoding = 1,

        KeepTrailingEquals = 0,
        OmitTrailingEquals = 2,

        IgnoreBase64DecodingErrors = 0,
        AbortOnBase64DecodingErrors = 4,
    };
    Q_DECLARE_FLAGS(Base64Options, Base64Option)

    enum class Base64DecodingStatus {
        Ok,
        IllegalInputLength,
        IllegalCharacter,
        IllegalPadding,
    };

    inline constexpr QByteArray() noexcept;
    QByteArray(const char *, qsizetype size = -1);
    QByteArray(qsizetype size, char c);
    QByteArray(qsizetype size, Qt::Initialization);
    explicit QByteArray(QByteArrayView v) : QByteArray(v.data(), v.size()) {}
    inline QByteArray(const QByteArray &) noexcept;
    inline ~QByteArray();

    QByteArray &operator=(const QByteArray &) noexcept;
    QByteArray &operator=(const char *str);
    inline QByteArray(QByteArray && other) noexcept
        = default;
    QT_MOVE_ASSIGNMENT_OPERATOR_IMPL_VIA_PURE_SWAP(QByteArray)
    inline void swap(QByteArray &other) noexcept
    { d.swap(other.d); }

    constexpr bool isEmpty() const noexcept { return size() == 0; }
    void resize(qsizetype size);
    void resize(qsizetype size, char c);
    void resizeForOverwrite(qsizetype size);

    QByteArray &fill(char c, qsizetype size = -1);

    inline qsizetype capacity() const;
    inline void reserve(qsizetype size);
    inline void squeeze();

#ifndef QT_NO_CAST_FROM_BYTEARRAY
    inline operator const char *() const;
    inline operator const void *() const;
#endif

    // Some compilers consider this conversion ambiguous, so
    // we're not offering it there:
    // * QCC 8.3 on QNX
    // * GHS 2022.1.4 on INTEGRITY
#if (!defined(Q_OS_QNX) || !defined(Q_CC_GNU_ONLY) || Q_CC_GNU_ONLY > 803) && \
    (!defined(Q_CC_GHS) || !defined(__GHS_VERSION_NUMBER) || __GHS_VERSION_NUMBER > 202214)
# define QT_BYTEARRAY_CONVERTS_TO_STD_STRING_VIEW
    Q_IMPLICIT operator std::string_view() const noexcept
    { return std::string_view(data(), std::size_t(size())); }
#endif

    inline char *data();
    inline const char *data() const noexcept;
    const char *constData() const noexcept { return data(); }
    inline void detach();
    inline bool isDetached() const;
    inline bool isSharedWith(const QByteArray &other) const noexcept
    { return data() == other.data() && size() == other.size(); }
    void clear();

    inline char at(qsizetype i) const;
    inline char operator[](qsizetype i) const;
    [[nodiscard]] inline char &operator[](qsizetype i);
    [[nodiscard]] char front() const { return at(0); }
    [[nodiscard]] inline char &front();
    [[nodiscard]] char back() const { return at(size() - 1); }
    [[nodiscard]] inline char &back();

    QT_CORE_INLINE_SINCE(6, 8)
    qsizetype indexOf(char c, qsizetype from = 0) const;
    qsizetype indexOf(QByteArrayView bv, qsizetype from = 0) const
    { return QtPrivate::findByteArray(qToByteArrayViewIgnoringNull(*this), from, bv); }

    QT_CORE_INLINE_SINCE(6, 8)
    qsizetype lastIndexOf(char c, qsizetype from = -1) const;
    qsizetype lastIndexOf(QByteArrayView bv) const
    { return lastIndexOf(bv, size()); }
    qsizetype lastIndexOf(QByteArrayView bv, qsizetype from) const
    { return QtPrivate::lastIndexOf(qToByteArrayViewIgnoringNull(*this), from, bv); }

    inline bool contains(char c) const;
    inline bool contains(QByteArrayView bv) const;
    qsizetype count(char c) const;
    qsizetype count(QByteArrayView bv) const
    { return QtPrivate::count(qToByteArrayViewIgnoringNull(*this), bv); }

    inline int compare(QByteArrayView a, Qt::CaseSensitivity cs = Qt::CaseSensitive) const noexcept;

#if QT_CORE_REMOVED_SINCE(6, 7)
    QByteArray left(qsizetype len) const;
    QByteArray right(qsizetype len) const;
    QByteArray mid(qsizetype index, qsizetype len = -1) const;
    QByteArray first(qsizetype n) const;
    QByteArray last(qsizetype n) const;
    QByteArray sliced(qsizetype pos) const;
    QByteArray sliced(qsizetype pos, qsizetype n) const;
    QByteArray chopped(qsizetype len) const;
#else
    [[nodiscard]] QByteArray left(qsizetype n) const &
    {
        if (n >= size())
            return *this;
        return first(qMax(n, 0));
    }
    [[nodiscard]] QByteArray left(qsizetype n) &&
    {
        if (n >= size())
            return std::move(*this);
        return std::move(*this).first(qMax(n, 0));
    }
    [[nodiscard]] QByteArray right(qsizetype n) const &
    {
        if (n >= size())
            return *this;
        return last(qMax(n, 0));
    }
    [[nodiscard]] QByteArray right(qsizetype n) &&
    {
        if (n >= size())
            return std::move(*this);
        return std::move(*this).last(qMax(n, 0));
    }
    [[nodiscard]] QByteArray mid(qsizetype index, qsizetype len = -1) const &;
    [[nodiscard]] QByteArray mid(qsizetype index, qsizetype len = -1) &&;

    [[nodiscard]] QByteArray first(qsizetype n) const &
    { verify(0, n); return sliced(0, n); }
    [[nodiscard]] QByteArray last(qsizetype n) const &
    { verify(0, n); return sliced(size() - n, n); }
    [[nodiscard]] QByteArray sliced(qsizetype pos) const &
    { verify(pos, 0); return sliced(pos, size() - pos); }
    [[nodiscard]] QByteArray sliced(qsizetype pos, qsizetype n) const &
    { verify(pos, n); return QByteArray(d.data() + pos, n); }
    [[nodiscard]] QByteArray chopped(qsizetype len) const &
    { verify(0, len); return sliced(0, size() - len); }

    [[nodiscard]] QByteArray first(qsizetype n) &&
    {
        verify(0, n);
        resize(n);      // may detach and allocate memory
        return std::move(*this);
    }
    [[nodiscard]] QByteArray last(qsizetype n) &&
    { verify(0, n); return sliced_helper(*this, size() - n, n); }
    [[nodiscard]] QByteArray sliced(qsizetype pos) &&
    { verify(pos, 0); return sliced_helper(*this, pos, size() - pos); }
    [[nodiscard]] QByteArray sliced(qsizetype pos, qsizetype n) &&
    { verify(pos, n); return sliced_helper(*this, pos, n); }
    [[nodiscard]] QByteArray chopped(qsizetype len) &&
    { verify(0, len); return std::move(*this).first(size() - len); }
#endif

    bool startsWith(QByteArrayView bv) const
    { return QtPrivate::startsWith(qToByteArrayViewIgnoringNull(*this), bv); }
    bool startsWith(char c) const { return size() > 0 && front() == c; }

    bool endsWith(char c) const { return size() > 0 && back() == c; }
    bool endsWith(QByteArrayView bv) const
    { return QtPrivate::endsWith(qToByteArrayViewIgnoringNull(*this), bv); }

    bool isUpper() const;
    bool isLower() const;

    [[nodiscard]] bool isValidUtf8() const noexcept
    {
        return QtPrivate::isValidUtf8(qToByteArrayViewIgnoringNull(*this));
    }

    void truncate(qsizetype pos);
    void chop(qsizetype n);

    QByteArray &slice(qsizetype pos)
    { verify(pos, 0); return remove(0, pos); }
    QByteArray &slice(qsizetype pos, qsizetype n)
    {
        verify(pos, n);
        if (isNull())
            return *this;
        resize(pos + n);
        return remove(0, pos);
    }

#if !defined(Q_QDOC)
    [[nodiscard]] QByteArray toLower() const &
    { return toLower_helper(*this); }
    [[nodiscard]] QByteArray toLower() &&
    { return toLower_helper(*this); }
    [[nodiscard]] QByteArray toUpper() const &
    { return toUpper_helper(*this); }
    [[nodiscard]] QByteArray toUpper() &&
    { return toUpper_helper(*this); }
    [[nodiscard]] QByteArray trimmed() const &
    { return trimmed_helper(*this); }
    [[nodiscard]] QByteArray trimmed() &&
    { return trimmed_helper(*this); }
    [[nodiscard]] QByteArray simplified() const &
    { return simplified_helper(*this); }
    [[nodiscard]] QByteArray simplified() &&
    { return simplified_helper(*this); }
#else
    [[nodiscard]] QByteArray toLower() const;
    [[nodiscard]] QByteArray toUpper() const;
    [[nodiscard]] QByteArray trimmed() const;
    [[nodiscard]] QByteArray simplified() const;
#endif

    [[nodiscard]] QByteArray leftJustified(qsizetype width, char fill = ' ', bool truncate = false) const;
    [[nodiscard]] QByteArray rightJustified(qsizetype width, char fill = ' ', bool truncate = false) const;

    QByteArray &prepend(char c)
    { return insert(0, QByteArrayView(&c, 1)); }
    inline QByteArray &prepend(qsizetype count, char c);
    QByteArray &prepend(const char *s)
    { return insert(0, QByteArrayView(s, qsizetype(qstrlen(s)))); }
    QByteArray &prepend(const char *s, qsizetype len)
    { return insert(0, QByteArrayView(s, len)); }
    QByteArray &prepend(const QByteArray &a);
    QByteArray &prepend(QByteArrayView a)
    { return insert(0, a); }

    QByteArray &append(char c);
    inline QByteArray &append(qsizetype count, char c);
    QByteArray &append(const char *s)
    { return append(s, -1); }
    QByteArray &append(const char *s, qsizetype len)
    { return append(QByteArrayView(s, len < 0 ? qsizetype(qstrlen(s)) : len)); }
    QByteArray &append(const QByteArray &a);
    QByteArray &append(QByteArrayView a)
    { return insert(size(), a); }

    QByteArray &assign(QByteArrayView v);
    QByteArray &assign(qsizetype n, char c)
    {
        Q_ASSERT(n >= 0);
        return fill(c, n);
    }
    template <typename InputIterator, if_input_iterator<InputIterator> = true>
    QByteArray &assign(InputIterator first, InputIterator last)
    {
        if constexpr (std::is_same_v<InputIterator, iterator> || std::is_same_v<InputIterator, const_iterator>)
            return assign(QByteArrayView(first, last));
        d.assign(first, last);
        if (d.data())
            d.data()[d.size] = '\0';
        return *this;
    }

    QByteArray &insert(qsizetype i, QByteArrayView data);
    inline QByteArray &insert(qsizetype i, const char *s)
    { return insert(i, QByteArrayView(s)); }
    inline QByteArray &insert(qsizetype i, const QByteArray &data)
    { return insert(i, QByteArrayView(data)); }
    QByteArray &insert(qsizetype i, qsizetype count, char c);
    QByteArray &insert(qsizetype i, char c)
    { return insert(i, QByteArrayView(&c, 1)); }
    QByteArray &insert(qsizetype i, const char *s, qsizetype len)
    { return insert(i, QByteArrayView(s, len)); }

    QByteArray &remove(qsizetype index, qsizetype len);
    QByteArray &removeAt(qsizetype pos)
    { return size_t(pos) < size_t(size()) ? remove(pos, 1) : *this; }
    QByteArray &removeFirst() { return !isEmpty() ? remove(0, 1) : *this; }
    QByteArray &removeLast() { return !isEmpty() ? remove(size() - 1, 1) : *this; }

    template <typename Predicate>
    QByteArray &removeIf(Predicate pred)
    {
        removeIf_helper(pred);
        return *this;
    }

    QByteArray &replace(qsizetype index, qsizetype len, const char *s, qsizetype alen)
    { return replace(index, len, QByteArrayView(s, alen)); }
    QByteArray &replace(qsizetype index, qsizetype len, QByteArrayView s);
    QByteArray &replace(char before, QByteArrayView after)
    { return replace(QByteArrayView(&before, 1), after); }
    QByteArray &replace(const char *before, qsizetype bsize, const char *after, qsizetype asize)
    { return replace(QByteArrayView(before, bsize), QByteArrayView(after, asize)); }
    QByteArray &replace(QByteArrayView before, QByteArrayView after);
    QByteArray &replace(char before, char after);

    QByteArray &operator+=(char c)
    { return append(c); }
    QByteArray &operator+=(const char *s)
    { return append(s); }
    QByteArray &operator+=(const QByteArray &a)
    { return append(a); }
    QByteArray &operator+=(QByteArrayView a)
    { return append(a); }

    QList<QByteArray> split(char sep) const;

    [[nodiscard]] QByteArray repeated(qsizetype times) const;

#if !defined(QT_NO_CAST_FROM_ASCII) && !defined(QT_RESTRICTED_CAST_FROM_ASCII)
#if QT_CORE_REMOVED_SINCE(6, 8)
    QT_ASCII_CAST_WARN inline bool operator==(const QString &s2) const;
    QT_ASCII_CAST_WARN inline bool operator!=(const QString &s2) const;
    QT_ASCII_CAST_WARN inline bool operator<(const QString &s2) const;
    QT_ASCII_CAST_WARN inline bool operator>(const QString &s2) const;
    QT_ASCII_CAST_WARN inline bool operator<=(const QString &s2) const;
    QT_ASCII_CAST_WARN inline bool operator>=(const QString &s2) const;
#endif // QT_CORE_REMOVED_SINCE(6, 8)
#endif // !defined(QT_NO_CAST_FROM_ASCII) && !defined(QT_RESTRICTED_CAST_FROM_ASCII)

    short toShort(bool *ok = nullptr, int base = 10) const;
    ushort toUShort(bool *ok = nullptr, int base = 10) const;
    int toInt(bool *ok = nullptr, int base = 10) const;
    uint toUInt(bool *ok = nullptr, int base = 10) const;
    long toLong(bool *ok = nullptr, int base = 10) const;
    ulong toULong(bool *ok = nullptr, int base = 10) const;
    qlonglong toLongLong(bool *ok = nullptr, int base = 10) const;
    qulonglong toULongLong(bool *ok = nullptr, int base = 10) const;
    float toFloat(bool *ok = nullptr) const;
    double toDouble(bool *ok = nullptr) const;
    QByteArray toBase64(Base64Options options = Base64Encoding) const;
    QByteArray toHex(char separator = '\0') const;
    QByteArray toPercentEncoding(const QByteArray &exclude = QByteArray(),
                                 const QByteArray &include = QByteArray(),
                                 char percent = '%') const;
    [[nodiscard]] QByteArray percentDecoded(char percent = '%') const;

    inline QByteArray &setNum(short, int base = 10);
    inline QByteArray &setNum(ushort, int base = 10);
    inline QByteArray &setNum(int, int base = 10);
    inline QByteArray &setNum(uint, int base = 10);
    inline QByteArray &setNum(long, int base = 10);
    inline QByteArray &setNum(ulong, int base = 10);
    QByteArray &setNum(qlonglong, int base = 10);
    QByteArray &setNum(qulonglong, int base = 10);
    inline QByteArray &setNum(float, char format = 'g', int precision = 6);
    QByteArray &setNum(double, char format = 'g', int precision = 6);
    QByteArray &setRawData(const char *a, qsizetype n);

    [[nodiscard]] static QByteArray number(int, int base = 10);
    [[nodiscard]] static QByteArray number(uint, int base = 10);
    [[nodiscard]] static QByteArray number(long, int base = 10);
    [[nodiscard]] static QByteArray number(ulong, int base = 10);
    [[nodiscard]] static QByteArray number(qlonglong, int base = 10);
    [[nodiscard]] static QByteArray number(qulonglong, int base = 10);
    [[nodiscard]] static QByteArray number(double, char format = 'g', int precision = 6);
    [[nodiscard]] static QByteArray fromRawData(const char *data, qsizetype size)
    {
        return QByteArray(DataPointer::fromRawData(data, size));
    }

    class FromBase64Result;
    [[nodiscard]] static FromBase64Result fromBase64Encoding(QByteArray &&base64, Base64Options options = Base64Encoding);
    [[nodiscard]] static FromBase64Result fromBase64Encoding(const QByteArray &base64, Base64Options options = Base64Encoding);
    [[nodiscard]] static QByteArray fromBase64(const QByteArray &base64, Base64Options options = Base64Encoding);
    [[nodiscard]] static QByteArray fromHex(const QByteArray &hexEncoded);
    [[nodiscard]] static QByteArray fromPercentEncoding(const QByteArray &pctEncoded, char percent = '%');

#if defined(Q_OS_DARWIN) || defined(Q_QDOC)
    static QByteArray fromCFData(CFDataRef data);
    static QByteArray fromRawCFData(CFDataRef data);
    CFDataRef toCFData() const Q_DECL_CF_RETURNS_RETAINED;
    CFDataRef toRawCFData() const Q_DECL_CF_RETURNS_RETAINED;
    static QByteArray fromNSData(const NSData *data);
    static QByteArray fromRawNSData(const NSData *data);
    NSData *toNSData() const Q_DECL_NS_RETURNS_AUTORELEASED;
    NSData *toRawNSData() const Q_DECL_NS_RETURNS_AUTORELEASED;
#endif

#if defined(Q_OS_WASM) || defined(Q_QDOC)
    static QByteArray fromEcmaUint8Array(emscripten::val uint8array);
    emscripten::val toEcmaUint8Array();
#endif

    typedef char *iterator;
    typedef const char *const_iterator;
    typedef iterator Iterator;
    typedef const_iterator ConstIterator;
    typedef std::reverse_iterator<iterator> reverse_iterator;
    typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
    iterator begin() { return data(); }
    const_iterator begin() const noexcept { return d.data(); }
    const_iterator cbegin() const noexcept { return begin(); }
    const_iterator constBegin() const noexcept { return begin(); }
    iterator end() { return begin() + size(); }
    const_iterator end() const noexcept { return begin() + size(); }
    const_iterator cend() const noexcept { return end(); }
    const_iterator constEnd() const noexcept { return end(); }
    reverse_iterator rbegin() { return reverse_iterator(end()); }
    reverse_iterator rend() { return reverse_iterator(begin()); }
    const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(end()); }
    const_reverse_iterator rend() const noexcept { return const_reverse_iterator(begin()); }
    const_reverse_iterator crbegin() const noexcept { return rbegin(); }
    const_reverse_iterator crend() const noexcept { return rend(); }

    // stl compatibility
    typedef qsizetype size_type;
    typedef qptrdiff difference_type;
    typedef const char & const_reference;
    typedef char & reference;
    typedef char *pointer;
    typedef const char *const_pointer;
    typedef char value_type;
    void push_back(char c)
    { append(c); }
    void push_back(const char *s)
    { append(s); }
    void push_back(const QByteArray &a)
    { append(a); }
    void push_back(QByteArrayView a)
    { append(a); }
    void push_front(char c)
    { prepend(c); }
    void push_front(const char *c)
    { prepend(c); }
    void push_front(const QByteArray &a)
    { prepend(a); }
    void push_front(QByteArrayView a)
    { prepend(a); }
    void shrink_to_fit() { squeeze(); }
    iterator erase(const_iterator first, const_iterator last);
    inline iterator erase(const_iterator it) { return erase(it, it + 1); }
    constexpr qsizetype max_size() const noexcept
    {
        return maxSize();
    }

    static QByteArray fromStdString(const std::string &s);
    std::string toStdString() const;

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
    constexpr qsizetype count() const noexcept { return size(); }
#endif
    constexpr qsizetype length() const noexcept { return size(); }
    QT_CORE_CONSTEXPR_INLINE_SINCE(6, 4)
    bool isNull() const noexcept;

    inline const DataPointer &data_ptr() const { return d; }
    inline DataPointer &data_ptr() { return d; }
#if QT_VERSION < QT_VERSION_CHECK(7, 0, 0)
    explicit inline QByteArray(const DataPointer &dd) : d(dd) {}
#endif
    explicit inline QByteArray(DataPointer &&dd) : d(std::move(dd)) {}

    [[nodiscard]] QByteArray nullTerminated() const &;
    [[nodiscard]] QByteArray nullTerminated() &&;
    QByteArray &nullTerminate();

private:
    friend bool comparesEqual(const QByteArray &lhs, const QByteArrayView &rhs) noexcept
    { return QByteArrayView(lhs) == rhs; }
    friend Qt::strong_ordering
    compareThreeWay(const QByteArray &lhs, const QByteArrayView &rhs) noexcept
    {
        const int res = QtPrivate::compareMemory(QByteArrayView(lhs), rhs);
        return Qt::compareThreeWay(res, 0);
    }
    Q_DECLARE_STRONGLY_ORDERED(QByteArray)
    Q_DECLARE_STRONGLY_ORDERED(QByteArray, const char *)
#if defined(__GLIBCXX__) && defined(__cpp_lib_three_way_comparison)
    // libstdc++ has a bug [0] when `operator const void *()` is preferred over
    // `operator<=>()` when calling std::less<> and other similar methods.
    // Fix it by explicitly providing relational operators in such case.
    // [0]: https://gcc.gnu.org/bugzilla/show_bug.cgi?id=114153
    friend bool operator<(const QByteArray &lhs, const QByteArray &rhs) noexcept
    { return is_lt(compareThreeWay(lhs, rhs)); }
    friend bool operator<=(const QByteArray &lhs, const QByteArray &rhs) noexcept
    { return is_lteq(compareThreeWay(lhs, rhs)); }
    friend bool operator>(const QByteArray &lhs, const QByteArray &rhs) noexcept
    { return is_gt(compareThreeWay(lhs, rhs)); }
    friend bool operator>=(const QByteArray &lhs, const QByteArray &rhs) noexcept
    { return is_gteq(compareThreeWay(lhs, rhs)); }
#endif // defined(__GLIBCXX__) && defined(__cpp_lib_three_way_comparison)

    // Check isEmpty() instead of isNull() for backwards compatibility.
    friend bool comparesEqual(const QByteArray &lhs, std::nullptr_t) noexcept
    { return lhs.isEmpty(); }
    friend Qt::strong_ordering compareThreeWay(const QByteArray &lhs, std::nullptr_t) noexcept
    { return lhs.isEmpty() ? Qt::strong_ordering::equivalent : Qt::strong_ordering::greater; }
    Q_DECLARE_STRONGLY_ORDERED(QByteArray, std::nullptr_t)

    // defined in qstring.cpp
    friend Q_CORE_EXPORT bool comparesEqual(const QByteArray &lhs, const QChar &rhs) noexcept;
    friend Q_CORE_EXPORT Qt::strong_ordering
    compareThreeWay(const QByteArray &lhs, const QChar &rhs) noexcept;
    friend Q_CORE_EXPORT bool comparesEqual(const QByteArray &lhs, char16_t rhs) noexcept;
    friend Q_CORE_EXPORT Qt::strong_ordering
    compareThreeWay(const QByteArray &lhs, char16_t rhs) noexcept;
#if !defined(QT_NO_CAST_FROM_ASCII) && !defined(QT_RESTRICTED_CAST_FROM_ASCII)
    Q_DECLARE_STRONGLY_ORDERED(QByteArray, QChar, QT_ASCII_CAST_WARN)
    Q_DECLARE_STRONGLY_ORDERED(QByteArray, char16_t, QT_ASCII_CAST_WARN)
#endif // !defined(QT_NO_CAST_FROM_ASCII) && !defined(QT_RESTRICTED_CAST_FROM_ASCII)


    void reallocData(qsizetype alloc, QArrayData::AllocationOption option);
    void reallocGrowData(qsizetype n);
    void expand(qsizetype i);

    Q_ALWAYS_INLINE constexpr void verify([[maybe_unused]] qsizetype pos = 0,
                                          [[maybe_unused]] qsizetype n = 1) const
    {
        Q_ASSERT(pos >= 0);
        Q_ASSERT(pos <= d.size);
        Q_ASSERT(n >= 0);
        Q_ASSERT(n <= d.size - pos);
    }

    static QByteArray sliced_helper(QByteArray &a, qsizetype pos, qsizetype n);
    static QByteArray toLower_helper(const QByteArray &a);
    static QByteArray toLower_helper(QByteArray &a);
    static QByteArray toUpper_helper(const QByteArray &a);
    static QByteArray toUpper_helper(QByteArray &a);
    static QByteArray trimmed_helper(const QByteArray &a);
    static QByteArray trimmed_helper(QByteArray &a);
    static QByteArray simplified_helper(const QByteArray &a);
    static QByteArray simplified_helper(QByteArray &a);
    template <typename Predicate>
    qsizetype removeIf_helper(Predicate pred)
    {
        const qsizetype result = d->eraseIf(pred);
        if (result > 0)
            d.data()[d.size] = '\0';
        return result;
    }

    friend class QString;
    friend Q_CORE_EXPORT QByteArray qUncompress(const uchar *data, qsizetype nbytes);

    template <typename T> friend qsizetype erase(QByteArray &ba, const T &t);
    template <typename Predicate> friend qsizetype erase_if(QByteArray &ba, Predicate pred);
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QByteArray::Base64Options)

inline constexpr QByteArray::QByteArray() noexcept {}
inline QByteArray::~QByteArray() {}

inline char QByteArray::at(qsizetype i) const
{ verify(i, 1); return d.data()[i]; }
inline char QByteArray::operator[](qsizetype i) const
{ verify(i, 1); return d.data()[i]; }

#ifndef QT_NO_CAST_FROM_BYTEARRAY
inline QByteArray::operator const char *() const
{ return data(); }
inline QByteArray::operator const void *() const
{ return data(); }
#endif
inline char *QByteArray::data()
{
    detach();
    Q_ASSERT(d.data());
    return d.data();
}
inline const char *QByteArray::data() const noexcept
{
#if QT5_NULL_STRINGS == 1
    return d.data() ? d.data() : &_empty;
#else
    return d.data();
#endif
}
inline void QByteArray::detach()
{ if (d.needsDetach()) reallocData(size(), QArrayData::KeepSize); }
inline bool QByteArray::isDetached() const
{ return !d.isShared(); }
inline QByteArray::QByteArray(const QByteArray &a) noexcept : d(a.d)
{}

inline qsizetype QByteArray::capacity() const { return qsizetype(d.constAllocatedCapacity()); }

inline void QByteArray::reserve(qsizetype asize)
{
    if (d.needsDetach() || asize > capacity() - d.freeSpaceAtBegin())
        reallocData(qMax(size(), asize), QArrayData::KeepSize);
    if (d.constAllocatedCapacity())
        d.setFlag(Data::CapacityReserved);
}

inline void QByteArray::squeeze()
{
    if (!d.isMutable())
        return;
    if (d.needsDetach() || size() < capacity())
        reallocData(size(), QArrayData::KeepSize);
    if (d.constAllocatedCapacity())
        d.clearFlag(Data::CapacityReserved);
}

inline char &QByteArray::operator[](qsizetype i)
{ verify(i, 1); return data()[i]; }
inline char &QByteArray::front() { return operator[](0); }
inline char &QByteArray::back() { return operator[](size() - 1); }
inline QByteArray &QByteArray::append(qsizetype n, char ch)
{ return insert(size(), n, ch); }
inline QByteArray &QByteArray::prepend(qsizetype n, char ch)
{ return insert(0, n, ch); }
inline bool QByteArray::contains(char c) const
{ return indexOf(c) != -1; }
inline bool QByteArray::contains(QByteArrayView bv) const
{ return indexOf(bv) != -1; }
inline int QByteArray::compare(QByteArrayView a, Qt::CaseSensitivity cs) const noexcept
{
    return cs == Qt::CaseSensitive ? QtPrivate::compareMemory(*this, a) :
                                     qstrnicmp(data(), size(), a.data(), a.size());
}
#if !defined(QT_USE_QSTRINGBUILDER)
inline QByteArray operator+(const QByteArray &a1, const QByteArray &a2)
{ return QByteArray(a1) += a2; }
inline QByteArray operator+(QByteArray &&lhs, const QByteArray &rhs)
{ return std::move(lhs += rhs); }
inline QByteArray operator+(const QByteArray &a1, const char *a2)
{ return QByteArray(a1) += a2; }
inline QByteArray operator+(QByteArray &&lhs, const char *rhs)
{ return std::move(lhs += rhs); }
inline QByteArray operator+(const QByteArray &a1, char a2)
{ return QByteArray(a1) += a2; }
inline QByteArray operator+(QByteArray &&lhs, char rhs)
{ return std::move(lhs += rhs); }
inline QByteArray operator+(const char *a1, const QByteArray &a2)
{ return QByteArray(a1) += a2; }
inline QByteArray operator+(char a1, const QByteArray &a2)
{ return QByteArray(&a1, 1) += a2; }
Q_WEAK_OVERLOAD
inline QByteArray operator+(const QByteArray &lhs, QByteArrayView rhs)
{
    QByteArray tmp{lhs.size() + rhs.size(), Qt::Uninitialized};
    return tmp.assign(lhs).append(rhs);
}
Q_WEAK_OVERLOAD
inline QByteArray operator+(QByteArrayView lhs, const QByteArray &rhs)
{
    QByteArray tmp{lhs.size() + rhs.size(), Qt::Uninitialized};
    return tmp.assign(lhs).append(rhs);
}
#endif // QT_USE_QSTRINGBUILDER

inline QByteArray &QByteArray::setNum(short n, int base)
{ return setNum(qlonglong(n), base); }
inline QByteArray &QByteArray::setNum(ushort n, int base)
{ return setNum(qulonglong(n), base); }
inline QByteArray &QByteArray::setNum(int n, int base)
{ return setNum(qlonglong(n), base); }
inline QByteArray &QByteArray::setNum(uint n, int base)
{ return setNum(qulonglong(n), base); }
inline QByteArray &QByteArray::setNum(long n, int base)
{ return setNum(qlonglong(n), base); }
inline QByteArray &QByteArray::setNum(ulong n, int base)
{ return setNum(qulonglong(n), base); }
inline QByteArray &QByteArray::setNum(float n, char format, int precision)
{ return setNum(double(n), format, precision); }

#if QT_CORE_INLINE_IMPL_SINCE(6, 4)
QT_CORE_CONSTEXPR_INLINE_SINCE(6, 4)
bool QByteArray::isNull() const noexcept
{
    return d.isNull();
}
#endif
#if QT_CORE_INLINE_IMPL_SINCE(6, 8)
qsizetype QByteArray::indexOf(char ch, qsizetype from) const
{
    return qToByteArrayViewIgnoringNull(*this).indexOf(ch, from);
}
qsizetype QByteArray::lastIndexOf(char ch, qsizetype from) const
{
    return qToByteArrayViewIgnoringNull(*this).lastIndexOf(ch, from);
}
#endif

#if !defined(QT_NO_DATASTREAM) || defined(QT_BOOTSTRAPPED)
Q_CORE_EXPORT QDataStream &operator<<(QDataStream &, const QByteArray &);
Q_CORE_EXPORT QDataStream &operator>>(QDataStream &, QByteArray &);
#endif

#ifndef QT_NO_COMPRESS
Q_CORE_EXPORT QByteArray qCompress(const uchar* data, qsizetype nbytes, int compressionLevel = -1);
Q_CORE_EXPORT QByteArray qUncompress(const uchar* data, qsizetype nbytes);
inline QByteArray qCompress(const QByteArray& data, int compressionLevel = -1)
{ return qCompress(reinterpret_cast<const uchar *>(data.constData()), data.size(), compressionLevel); }
inline QByteArray qUncompress(const QByteArray& data)
{ return qUncompress(reinterpret_cast<const uchar*>(data.constData()), data.size()); }
#endif

Q_DECLARE_SHARED(QByteArray)

class QByteArray::FromBase64Result
{
public:
    QByteArray decoded;
    QByteArray::Base64DecodingStatus decodingStatus;

    void swap(QByteArray::FromBase64Result &other) noexcept
    {
        decoded.swap(other.decoded);
        std::swap(decodingStatus, other.decodingStatus);
    }

    explicit operator bool() const noexcept { return decodingStatus == QByteArray::Base64DecodingStatus::Ok; }

#if defined(Q_COMPILER_REF_QUALIFIERS) && !defined(Q_QDOC)
    QByteArray &operator*() & noexcept { return decoded; }
    const QByteArray &operator*() const & noexcept { return decoded; }
    QByteArray &&operator*() && noexcept { return std::move(decoded); }
#else
    QByteArray &operator*() noexcept { return decoded; }
    const QByteArray &operator*() const noexcept { return decoded; }
#endif

    friend inline bool operator==(const QByteArray::FromBase64Result &lhs, const QByteArray::FromBase64Result &rhs) noexcept
    {
        if (lhs.decodingStatus != rhs.decodingStatus)
            return false;

        if (lhs.decodingStatus == QByteArray::Base64DecodingStatus::Ok && lhs.decoded != rhs.decoded)
            return false;

        return true;
    }

    friend inline bool operator!=(const QByteArray::FromBase64Result &lhs, const QByteArray::FromBase64Result &rhs) noexcept
    {
        return !(lhs == rhs);
    }
};

Q_DECLARE_SHARED(QByteArray::FromBase64Result)


Q_CORE_EXPORT Q_DECL_PURE_FUNCTION size_t qHash(const QByteArray::FromBase64Result &key, size_t seed = 0) noexcept;

template <typename T>
qsizetype erase(QByteArray &ba, const T &t)
{
    return ba.removeIf_helper([&t](const auto &e) { return t == e; });
}

template <typename Predicate>
qsizetype erase_if(QByteArray &ba, Predicate pred)
{
    return ba.removeIf_helper(pred);
}

//
// QByteArrayView members that require QByteArray:
//
QByteArray QByteArrayView::toByteArray() const
{
    return QByteArray(*this);
}

namespace Qt {
inline namespace Literals {
inline namespace StringLiterals {

inline QByteArray operator""_ba(const char *str, size_t size) noexcept
{
    return QByteArray(QByteArrayData(nullptr, const_cast<char *>(str), qsizetype(size)));
}

} // StringLiterals
} // Literals
} // Qt

inline namespace QtLiterals {
#if QT_DEPRECATED_SINCE(6, 8)

QT_DEPRECATED_VERSION_X_6_8("Use _ba from Qt::StringLiterals namespace instead.")
inline QByteArray operator""_qba(const char *str, size_t size) noexcept
{
    return Qt::StringLiterals::operator""_ba(str, size);
}

#endif // QT_DEPRECATED_SINCE(6, 8)
} // QtLiterals

QT_END_NAMESPACE

#endif // QBYTEARRAY_H
