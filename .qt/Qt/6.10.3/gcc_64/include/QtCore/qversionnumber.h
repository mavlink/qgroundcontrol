// Copyright (C) 2020 The Qt Company Ltd.
// Copyright (C) 2022 Intel Corporation.
// Copyright (C) 2015 Keith Gardner <kreios4004@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:data-parser

#ifndef QVERSIONNUMBER_H
#define QVERSIONNUMBER_H

#include <QtCore/qcompare.h>
#include <QtCore/qcontainertools_impl.h>
#include <QtCore/qlist.h>
#include <QtCore/qmetatype.h>
#include <QtCore/qnamespace.h>
#include <QtCore/qspan.h>
#include <QtCore/qstring.h>
#include <QtCore/qtypeinfo.h>
#if !defined(QT_LEAN_HEADERS) || QT_LEAN_HEADERS < 2
#include <QtCore/qtyperevision.h>
#endif // lean headers level 2

QT_BEGIN_NAMESPACE

class QVersionNumber;
Q_CORE_EXPORT size_t qHash(const QVersionNumber &key, size_t seed = 0);

#ifndef QT_NO_DATASTREAM
Q_CORE_EXPORT QDataStream &operator<<(QDataStream &out, const QVersionNumber &version);
Q_CORE_EXPORT QDataStream &operator>>(QDataStream &in, QVersionNumber &version);
#endif

class QVersionNumber
{
    /*
     * QVersionNumber stores small values inline, without memory allocation.
     * We do that by setting the LSB in the pointer that would otherwise hold
     * the longer form of the segments.
     * The constants below help us deal with the permutations for 32- and 64-bit,
     * little- and big-endian architectures.
     */
    enum {
        // in little-endian, inline_segments[0] is shared with the pointer's LSB, while
        // in big-endian, it's inline_segments[7]
        InlineSegmentMarker = Q_BYTE_ORDER == Q_LITTLE_ENDIAN ? 0 : sizeof(void *) - 1,
        InlineSegmentStartIdx = !InlineSegmentMarker, // 0 for BE, 1 for LE
        InlineSegmentCount = sizeof(void *) - 1
    };
    static_assert(InlineSegmentCount >= 3);   // at least major, minor, micro

    struct SegmentStorage
    {
        // Note: we alias the use of dummy and inline_segments in the use of the
        // union below. This is undefined behavior in C++98, but most compilers implement
        // the C++11 behavior. The one known exception is older versions of Sun Studio.
        union {
            quintptr dummy;
            qint8 inline_segments[sizeof(void *)];
            QList<int> *pointer_segments;
        };

        // set the InlineSegmentMarker and set length to zero
        SegmentStorage() noexcept : dummy(1) {}

        SegmentStorage(const QList<int> &seg)
        {
            if (dataFitsInline(seg.data(), seg.size()))
                setInlineData(seg.data(), seg.size());
            else
                setListData(seg);
        }

        Q_CORE_EXPORT void setListData(const QList<int> &seg);

        SegmentStorage(const SegmentStorage &other)
        {
            if (other.isUsingPointer())
                setListData(*other.pointer_segments);
            else
                dummy = other.dummy;
        }

        SegmentStorage &operator=(const SegmentStorage &other)
        {
            if (isUsingPointer() && other.isUsingPointer()) {
                *pointer_segments = *other.pointer_segments;
            } else if (other.isUsingPointer()) {
                setListData(*other.pointer_segments);
            } else {
                if (isUsingPointer())
                    delete pointer_segments;
                dummy = other.dummy;
            }
            return *this;
        }

        SegmentStorage(SegmentStorage &&other) noexcept
            : dummy(other.dummy)
        {
            other.dummy = 1;
        }

        QT_MOVE_ASSIGNMENT_OPERATOR_IMPL_VIA_PURE_SWAP(SegmentStorage)

        void swap(SegmentStorage &other) noexcept
        {
            std::swap(dummy, other.dummy);
        }

        explicit SegmentStorage(QList<int> &&seg)
        {
            if (dataFitsInline(std::as_const(seg).data(), seg.size()))
                setInlineData(std::as_const(seg).data(), seg.size());
            else
                setListData(std::move(seg));
        }

        Q_CORE_EXPORT void setListData(QList<int> &&seg);

        explicit SegmentStorage(QSpan<const int> args)
            : SegmentStorage(args.begin(), args.end()) {}

        explicit SegmentStorage(const int *first, const int *last)
        {
            if (dataFitsInline(first, last - first)) {
                setInlineData(first, last - first);
            } else {
                setListData(first, last);
            }
        }

        Q_CORE_EXPORT void setListData(const int *first, const int *last);

        ~SegmentStorage() { if (isUsingPointer()) delete pointer_segments; }

        bool isUsingPointer() const noexcept
        { return (inline_segments[InlineSegmentMarker] & 1) == 0; }

        qsizetype size() const noexcept
        { return isUsingPointer() ? pointer_segments->size() : (inline_segments[InlineSegmentMarker] >> 1); }

        void setInlineSize(qsizetype len)
        {
            Q_ASSERT(len <= InlineSegmentCount);
            inline_segments[InlineSegmentMarker] = qint8(1 + 2 * len);
        }

        Q_CORE_EXPORT void resize(qsizetype len);

        int at(qsizetype index) const
        {
            return isUsingPointer() ?
                        pointer_segments->at(index) :
                        inline_segments[InlineSegmentStartIdx + index];
        }

        void setSegments(int len, int maj, int min = 0, int mic = 0)
        {
            if (maj == qint8(maj) && min == qint8(min) && mic == qint8(mic)) {
                int data[] = { maj, min, mic };
                setInlineData(data, len);
            } else {
                setVector(len, maj, min, mic);
            }
        }

    private:
        static bool dataFitsInline(const int *data, qsizetype len)
        {
            if (len > InlineSegmentCount)
                return false;
            for (qsizetype i = 0; i < len; ++i)
                if (data[i] != qint8(data[i]))
                    return false;
            return true;
        }
        void setInlineData(const int *data, qsizetype len)
        {
            Q_ASSERT(len <= InlineSegmentCount);
            dummy = 1 + len * 2;
#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
            for (qsizetype i = 0; i < len; ++i)
                dummy |= quintptr(data[i] & 0xFF) << (8 * (i + 1));
#elif Q_BYTE_ORDER == Q_BIG_ENDIAN
            for (qsizetype i = 0; i < len; ++i)
                dummy |= quintptr(data[i] & 0xFF) << (8 * (sizeof(void *) - i - 1));
#else
            // the code above is equivalent to:
            setInlineSize(len);
            for (qsizetype i = 0; i < len; ++i)
                inline_segments[InlineSegmentStartIdx + i] = data[i] & 0xFF;
#endif
        }

        Q_CORE_EXPORT void setVector(int len, int maj, int min, int mic);
    } m_segments;

    class It
    {
        const QVersionNumber *v;
        qsizetype i;

        friend class QVersionNumber;
        explicit constexpr It(const QVersionNumber *vn, qsizetype idx) noexcept : v(vn), i(idx) {}

        friend constexpr bool comparesEqual(const It &lhs, const It &rhs)
        { Q_ASSERT(lhs.v == rhs.v); return lhs.i == rhs.i; }
        friend constexpr Qt::strong_ordering compareThreeWay(const It &lhs, const It &rhs)
        { Q_ASSERT(lhs.v == rhs.v); return Qt::compareThreeWay(lhs.i, rhs.i); }
        // macro variant does not exist: Q_DECLARE_STRONGLY_ORDERED_LITERAL_TYPE_NON_NOEXCEPT(It)
        friend constexpr bool operator==(It lhs, It rhs) {
            return comparesEqual(lhs, rhs);
        }
#ifdef __cpp_lib_three_way_comparison
        friend constexpr std::strong_ordering operator<=>(It lhs, It rhs) {
            return compareThreeWay(lhs, rhs);
        }
#else
        friend constexpr bool operator!=(It lhs, It rhs) {
            return !operator==(lhs, rhs);
        }
        friend constexpr bool operator<(It lhs, It rhs) {
            return is_lt(compareThreeWay(lhs, rhs));
        }
        friend constexpr bool operator<=(It lhs, It rhs) {
            return is_lteq(compareThreeWay(lhs, rhs));
        }
        friend constexpr bool operator>(It lhs, It rhs) {
            return is_gt(compareThreeWay(lhs, rhs));
        }
        friend constexpr bool operator>=(It lhs, It rhs) {
            return is_gteq(compareThreeWay(lhs, rhs));
        }
#endif

    public:
        // Rule Of Zero applies
        It() = default;

        using iterator_category = std::random_access_iterator_tag;
        using value_type = int;
#ifdef QT_COMPILER_HAS_LWG3346
        using element_type = const int;
#endif
        using difference_type = qptrdiff; // difference to container requirements
        using size_type = qsizetype;      // difference to container requirements
        using reference = value_type;     // difference to container requirements
        using pointer = QtPrivate::ArrowProxy<reference>;

        reference operator*() const { return v->segmentAt(i); }
        pointer operator->() const { return {**this}; }

        It &operator++() { ++i; return *this; }
        It operator++(int) { auto copy = *this; ++*this; return copy; }

        It &operator--() { --i; return *this; }
        It operator--(int) { auto copy = *this; --*this; return copy; }

        It &operator+=(difference_type n) { i += n; return *this; }
        friend It operator+(It it, difference_type n) { it += n; return it; }
        friend It operator+(difference_type n, It it) { return it + n; }

        It &operator-=(difference_type n) { i -= n; return *this; }
        friend It operator-(It it, difference_type n) { it -= n; return it; }

        friend difference_type operator-(It lhs, It rhs)
        { Q_ASSERT(lhs.v == rhs.v); return lhs.i - rhs.i; }

        reference operator[](difference_type n) const { return *(*this + n); }
    };

public:
    using const_iterator = It;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    using value_type = It::value_type;
    using difference_type = It::difference_type;
    using size_type = It::size_type;
    using reference = It::reference;
    using const_reference = reference;
    using pointer = It::pointer;
    using const_pointer = pointer;

    inline QVersionNumber() noexcept
        : m_segments()
    {}
    Q_WEAK_OVERLOAD
    inline explicit QVersionNumber(const QList<int> &seg) : m_segments(seg) { }

    // compiler-generated copy/move ctor/assignment operators and the destructor are ok

    Q_WEAK_OVERLOAD
    explicit QVersionNumber(QList<int> &&seg) : m_segments(std::move(seg)) { }

    inline QVersionNumber(std::initializer_list<int> args)
        : m_segments(QSpan{args})
    {}

    explicit QVersionNumber(QSpan<const int> args)
        : m_segments(args)
    {}

    inline explicit QVersionNumber(int maj)
    { m_segments.setSegments(1, maj); }

    inline explicit QVersionNumber(int maj, int min)
    { m_segments.setSegments(2, maj, min); }

    inline explicit QVersionNumber(int maj, int min, int mic)
    { m_segments.setSegments(3, maj, min, mic); }

    [[nodiscard]] inline bool isNull() const noexcept
    { return segmentCount() == 0; }

    [[nodiscard]] inline bool isNormalized() const noexcept
    { return isNull() || segmentAt(segmentCount() - 1) != 0; }

    [[nodiscard]] inline int majorVersion() const noexcept
    { return segmentAt(0); }

    [[nodiscard]] inline int minorVersion() const noexcept
    { return segmentAt(1); }

    [[nodiscard]] inline int microVersion() const noexcept
    { return segmentAt(2); }

    [[nodiscard]] Q_CORE_EXPORT QVersionNumber normalized() const;

    [[nodiscard]] Q_CORE_EXPORT QList<int> segments() const;

    [[nodiscard]] inline int segmentAt(qsizetype index) const noexcept
    { return (m_segments.size() > index) ? m_segments.at(index) : 0; }

    [[nodiscard]] inline qsizetype segmentCount() const noexcept
    { return m_segments.size(); }

    [[nodiscard]] const_iterator begin()  const noexcept { return const_iterator{this, 0}; }
    [[nodiscard]] const_iterator end()    const noexcept { return begin() + segmentCount(); }
    [[nodiscard]] const_iterator cbegin() const noexcept { return begin(); }
    [[nodiscard]] const_iterator cend()   const noexcept { return end(); }

    [[nodiscard]] const_reverse_iterator rbegin()  const noexcept { return const_reverse_iterator{end()}; }
    [[nodiscard]] const_reverse_iterator rend()    const noexcept { return const_reverse_iterator{begin()}; }
    [[nodiscard]] const_reverse_iterator crbegin() const noexcept { return rbegin(); }
    [[nodiscard]] const_reverse_iterator crend()   const noexcept { return rend(); }

    [[nodiscard]] const_iterator constBegin() const noexcept { return begin(); }
    [[nodiscard]] const_iterator constEnd() const noexcept { return end(); }

    [[nodiscard]] Q_CORE_EXPORT bool isPrefixOf(const QVersionNumber &other) const noexcept;

    [[nodiscard]] Q_CORE_EXPORT static int compare(const QVersionNumber &v1, const QVersionNumber &v2) noexcept;

    [[nodiscard]] Q_CORE_EXPORT static QVersionNumber commonPrefix(const QVersionNumber &v1, const QVersionNumber &v2);

    [[nodiscard]] Q_CORE_EXPORT QString toString() const;
    [[nodiscard]] Q_CORE_EXPORT static QVersionNumber fromString(QAnyStringView string, qsizetype *suffixIndex = nullptr);

#if QT_DEPRECATED_SINCE(6, 4) && QT_POINTER_SIZE != 4
    Q_WEAK_OVERLOAD
    QT_DEPRECATED_VERSION_X_6_4("Use the 'qsizetype *suffixIndex' overload.")
    [[nodiscard]] static QVersionNumber fromString(QAnyStringView string, int *suffixIndex)
    {
        QT_WARNING_PUSH
        // fromString() writes to *n unconditionally, but GCC can't know that
        QT_WARNING_DISABLE_GCC("-Wmaybe-uninitialized")
        qsizetype n;
        auto r = fromString(string, &n);
        if (suffixIndex) {
            Q_ASSERT(int(n) == n);
            *suffixIndex = int(n);
        }
        return r;
        QT_WARNING_POP
    }
#endif


#if QT_CORE_REMOVED_SINCE(6, 4)
    [[nodiscard]] Q_CORE_EXPORT static QVersionNumber fromString(const QString &string, int *suffixIndex);
    [[nodiscard]] Q_CORE_EXPORT static QVersionNumber fromString(QLatin1StringView string, int *suffixIndex);
    [[nodiscard]] Q_CORE_EXPORT static QVersionNumber fromString(QStringView string, int *suffixIndex);
#endif

private:
    [[nodiscard]] friend bool comparesEqual(const QVersionNumber &lhs,
                                            const QVersionNumber &rhs) noexcept
    {
        return lhs.segmentCount() == rhs.segmentCount() && compare(lhs, rhs) == 0;
    }
    [[nodiscard]] friend Qt::strong_ordering compareThreeWay(const QVersionNumber &lhs,
                                                             const QVersionNumber &rhs) noexcept
    {
        int c = compare(lhs, rhs);
        return Qt::compareThreeWay(c, 0);
    }
    Q_DECLARE_STRONGLY_ORDERED(QVersionNumber)

#ifndef QT_NO_DATASTREAM
    friend Q_CORE_EXPORT QDataStream& operator>>(QDataStream &in, QVersionNumber &version);
#endif
    friend Q_CORE_EXPORT size_t qHash(const QVersionNumber &key, size_t seed);
};

Q_DECLARE_TYPEINFO(QVersionNumber, Q_RELOCATABLE_TYPE);

#ifndef QT_NO_DEBUG_STREAM
Q_CORE_EXPORT QDebug operator<<(QDebug, const QVersionNumber &version);
#endif

QT_END_NAMESPACE

QT_DECL_METATYPE_EXTERN(QVersionNumber, Q_CORE_EXPORT)

#endif // QVERSIONNUMBER_H
