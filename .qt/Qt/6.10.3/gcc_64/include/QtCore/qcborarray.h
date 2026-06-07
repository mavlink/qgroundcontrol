// Copyright (C) 2022 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:data-parser

#ifndef QCBORARRAY_H
#define QCBORARRAY_H

#include <QtCore/qcborvalue.h>

#include <initializer_list>

QT_BEGIN_NAMESPACE

class QJsonArray;
class QDataStream;

namespace QJsonPrivate { class Variant; }

class QCborContainerPrivate;
class Q_CORE_EXPORT QCborArray
{
public:
    class ConstIterator;
    class Iterator {
        QCborValueRef item {};
        friend class ConstIterator;
        friend class QCborArray;
        Iterator(QCborContainerPrivate *dd, qsizetype ii) : item(dd, ii) {}
    public:
        typedef std::random_access_iterator_tag iterator_category;
        typedef qsizetype difference_type;
        typedef QCborValue value_type;
        typedef QCborValueRef reference;
        typedef QCborValueRef *pointer;

        constexpr Iterator() = default;
        constexpr Iterator(const Iterator &) = default;
        Iterator &operator=(const Iterator &other)
        {
            // rebind the reference
            item.d = other.item.d;
            item.i = other.item.i;
            return *this;
        }

        QCborValueRef operator*() const { return item; }
        QCborValueRef *operator->() { return &item; }
        const QCborValueConstRef *operator->() const { return &item; }
        QCborValueRef operator[](qsizetype j) const { return { item.d, item.i + j }; }
#if QT_CORE_REMOVED_SINCE(6, 8)
        bool operator==(const Iterator &o) const { return item.d == o.item.d && item.i == o.item.i; }
        bool operator!=(const Iterator &o) const { return !operator==(o); }
        bool operator<(const Iterator& other) const { Q_ASSERT(item.d == other.item.d); return item.i < other.item.i; }
        bool operator<=(const Iterator& other) const { Q_ASSERT(item.d == other.item.d); return item.i <= other.item.i; }
        bool operator>(const Iterator& other) const { Q_ASSERT(item.d == other.item.d); return item.i > other.item.i; }
        bool operator>=(const Iterator& other) const { Q_ASSERT(item.d == other.item.d); return item.i >= other.item.i; }
        bool operator==(const ConstIterator &o) const { return item.d == o.item.d && item.i == o.item.i; }
        bool operator!=(const ConstIterator &o) const { return !operator==(o); }
        bool operator<(const ConstIterator& other) const { Q_ASSERT(item.d == other.item.d); return item.i < other.item.i; }
        bool operator<=(const ConstIterator& other) const { Q_ASSERT(item.d == other.item.d); return item.i <= other.item.i; }
        bool operator>(const ConstIterator& other) const { Q_ASSERT(item.d == other.item.d); return item.i > other.item.i; }
        bool operator>=(const ConstIterator& other) const { Q_ASSERT(item.d == other.item.d); return item.i >= other.item.i; }
#endif
        Iterator &operator++() { ++item.i; return *this; }
        Iterator operator++(int) { Iterator n = *this; ++item.i; return n; }
        Iterator &operator--() { item.i--; return *this; }
        Iterator operator--(int) { Iterator n = *this; item.i--; return n; }
        Iterator &operator+=(qsizetype j) { item.i += j; return *this; }
        Iterator &operator-=(qsizetype j) { item.i -= j; return *this; }
        Iterator operator+(qsizetype j) const { return Iterator({ item.d, item.i + j }); }
        Iterator operator-(qsizetype j) const { return Iterator({ item.d, item.i - j }); }
        qsizetype operator-(Iterator j) const { return item.i - j.item.i; }
    private:
        // Helper functions
        static bool comparesEqual_helper(const Iterator &lhs, const Iterator &rhs) noexcept
        {
            return lhs.item.d == rhs.item.d && lhs.item.i == rhs.item.i;
        }

        static bool comparesEqual_helper(const Iterator &lhs, const ConstIterator &rhs) noexcept
        {
            return lhs.item.d == rhs.item.d && lhs.item.i == rhs.item.i;
        }

        static Qt::strong_ordering compareThreeWay_helper(const Iterator &lhs,
                                                          const Iterator &rhs) noexcept
        {
            Q_ASSERT(lhs.item.d == rhs.item.d);
            return Qt::compareThreeWay(lhs.item.i, rhs.item.i);
        }

        static Qt::strong_ordering compareThreeWay_helper(const Iterator &lhs,
                                                          const ConstIterator &rhs) noexcept
        {
            Q_ASSERT(lhs.item.d == rhs.item.d);
            return Qt::compareThreeWay(lhs.item.i, rhs.item.i);
        }

        // Compare friends
        friend bool comparesEqual(const Iterator &lhs, const Iterator &rhs) noexcept
        {
            return comparesEqual_helper(lhs, rhs);
        }
        friend Qt::strong_ordering compareThreeWay(const Iterator &lhs,
                                                   const Iterator &rhs) noexcept
        {
            return compareThreeWay_helper(lhs, rhs);
        }
        Q_DECLARE_STRONGLY_ORDERED(Iterator)
        friend bool comparesEqual(const Iterator &lhs, const ConstIterator &rhs) noexcept
        {
            return comparesEqual_helper(lhs, rhs);
        }
        friend Qt::strong_ordering compareThreeWay(const Iterator &lhs,
                                                   const ConstIterator &rhs) noexcept
        {
            return compareThreeWay_helper(lhs, rhs);
        }
        Q_DECLARE_STRONGLY_ORDERED(Iterator, ConstIterator)
    };

    class ConstIterator {
        QCborValueConstRef item;
        friend class Iterator;
        friend class QCborArray;
        ConstIterator(QCborContainerPrivate *dd, qsizetype ii) : item(dd, ii) {}
    public:
        typedef std::random_access_iterator_tag iterator_category;
        typedef qsizetype difference_type;
        typedef const QCborValue value_type;
        typedef const QCborValueRef reference;
        typedef const QCborValueRef *pointer;

        constexpr ConstIterator() = default;
        constexpr ConstIterator(const ConstIterator &) = default;
        ConstIterator &operator=(const ConstIterator &other)
        {
            // rebind the reference
            item.d = other.item.d;
            item.i = other.item.i;
            return *this;
        }

        QCborValueConstRef operator*() const { return item; }
        const QCborValueConstRef *operator->() const { return &item; }
        QCborValueConstRef operator[](qsizetype j) const { return QCborValueRef{ item.d, item.i + j }; }
#if QT_CORE_REMOVED_SINCE(6, 8)
        bool operator==(const Iterator &o) const { return item.d == o.item.d && item.i == o.item.i; }
        bool operator!=(const Iterator &o) const { return !operator==(o); }
        bool operator<(const Iterator& other) const { Q_ASSERT(item.d == other.item.d); return item.i < other.item.i; }
        bool operator<=(const Iterator& other) const { Q_ASSERT(item.d == other.item.d); return item.i <= other.item.i; }
        bool operator>(const Iterator& other) const { Q_ASSERT(item.d == other.item.d); return item.i > other.item.i; }
        bool operator>=(const Iterator& other) const { Q_ASSERT(item.d == other.item.d); return item.i >= other.item.i; }
        bool operator==(const ConstIterator &o) const { return item.d == o.item.d && item.i == o.item.i; }
        bool operator!=(const ConstIterator &o) const { return !operator==(o); }
        bool operator<(const ConstIterator& other) const { Q_ASSERT(item.d == other.item.d); return item.i < other.item.i; }
        bool operator<=(const ConstIterator& other) const { Q_ASSERT(item.d == other.item.d); return item.i <= other.item.i; }
        bool operator>(const ConstIterator& other) const { Q_ASSERT(item.d == other.item.d); return item.i > other.item.i; }
        bool operator>=(const ConstIterator& other) const { Q_ASSERT(item.d == other.item.d); return item.i >= other.item.i; }
#endif
        ConstIterator &operator++() { ++item.i; return *this; }
        ConstIterator operator++(int) { ConstIterator n = *this; ++item.i; return n; }
        ConstIterator &operator--() { item.i--; return *this; }
        ConstIterator operator--(int) { ConstIterator n = *this; item.i--; return n; }
        ConstIterator &operator+=(qsizetype j) { item.i += j; return *this; }
        ConstIterator &operator-=(qsizetype j) { item.i -= j; return *this; }
        ConstIterator operator+(qsizetype j) const { return ConstIterator({ item.d, item.i + j }); }
        ConstIterator operator-(qsizetype j) const { return ConstIterator({ item.d, item.i - j }); }
        qsizetype operator-(ConstIterator j) const { return item.i - j.item.i; }
    private:
        // Helper functions
        static bool comparesEqual_helper(const ConstIterator &lhs,
                                         const ConstIterator &rhs) noexcept
        {
            return lhs.item.d == rhs.item.d && lhs.item.i == rhs.item.i;
        }
        static Qt::strong_ordering compareThreeWay_helper(const ConstIterator &lhs,
                                                          const ConstIterator &rhs) noexcept
        {
            Q_ASSERT(lhs.item.d == rhs.item.d);
            return Qt::compareThreeWay(lhs.item.i, rhs.item.i);
        }

        // Compare friends
        friend bool comparesEqual(const ConstIterator &lhs, const ConstIterator &rhs) noexcept
        {
            return comparesEqual_helper(lhs, rhs);
        }
        friend Qt::strong_ordering compareThreeWay(const ConstIterator &lhs,
                                                   const ConstIterator &rhs) noexcept
        {
            return compareThreeWay_helper(lhs, rhs);
        }
        Q_DECLARE_STRONGLY_ORDERED(ConstIterator)
    };

    typedef qsizetype size_type;
    typedef QCborValue value_type;
    typedef value_type *pointer;
    typedef const value_type *const_pointer;
    typedef QCborValue &reference;
    typedef const QCborValue &const_reference;
    typedef qsizetype difference_type;

    QCborArray() noexcept;
    QCborArray(const QCborArray &other) noexcept;
    QCborArray(QCborArray &&other) noexcept = default;
    QCborArray &operator=(const QCborArray &other) noexcept;
    QT_MOVE_ASSIGNMENT_OPERATOR_IMPL_VIA_PURE_SWAP(QCborArray)
    QCborArray(std::initializer_list<QCborValue> args)
        : QCborArray()
    {
        detach(qsizetype(args.size()));
        for (const QCborValue &v : args)
            append(v);
    }
    ~QCborArray();

    void swap(QCborArray &other) noexcept
    {
        d.swap(other.d);
    }

    QCborValue toCborValue() const { return *this; }

    qsizetype size() const noexcept;
    bool isEmpty() const { return size() == 0; }
    void clear();

    QCborValue at(qsizetype i) const;
    QCborValue first() const { return at(0); }
    QCborValue last() const { return at(size() - 1); }
    const QCborValue operator[](qsizetype i) const { return at(i); }
    QCborValueRef first() { Q_ASSERT(!isEmpty()); return begin()[0]; }
    QCborValueRef last() { Q_ASSERT(!isEmpty()); return begin()[size() - 1]; }
    QCborValueRef operator[](qsizetype i)
    {
        if (i >= size())
            insert(i, QCborValue());
        return begin()[i];
    }

    void insert(qsizetype i, const QCborValue &value);
    void insert(qsizetype i, QCborValue &&value);
    void prepend(const QCborValue &value) { insert(0, value); }
    void prepend(QCborValue &&value) { insert(0, std::move(value)); }
    void append(const QCborValue &value) { insert(-1, value); }
    void append(QCborValue &&value) { insert(-1, std::move(value)); }
    QCborValue extract(ConstIterator it) { return extract(Iterator{ it.item.d, it.item.i }); }
    QCborValue extract(Iterator it);
    void removeAt(qsizetype i);
    QCborValue takeAt(qsizetype i) { Q_ASSERT(i < size()); return extract(begin() + i); }
    void removeFirst() { removeAt(0); }
    void removeLast() { removeAt(size() - 1); }
    QCborValue takeFirst() { return takeAt(0); }
    QCborValue takeLast() { return takeAt(size() - 1); }

    bool contains(const QCborValue &value) const;

    int compare(const QCborArray &other) const noexcept Q_DECL_PURE_FUNCTION;
#if QT_CORE_REMOVED_SINCE(6, 8)
    bool operator==(const QCborArray &other) const noexcept
    { return compare(other) == 0; }
    bool operator!=(const QCborArray &other) const noexcept
    { return !operator==(other); }
    bool operator<(const QCborArray &other) const
    { return compare(other) < 0; }
#endif

    typedef Iterator iterator;
    typedef ConstIterator const_iterator;
    iterator begin() { detach(); return iterator{d.data(), 0}; }
    const_iterator constBegin() const { return const_iterator{d.data(), 0}; }
    const_iterator begin() const { return constBegin(); }
    const_iterator cbegin() const { return constBegin(); }
    iterator end() { detach(); return iterator{d.data(), size()}; }
    const_iterator constEnd() const { return const_iterator{d.data(), size()}; }
    const_iterator end() const { return constEnd(); }
    const_iterator cend() const { return constEnd(); }
    iterator insert(iterator before, const QCborValue &value)
    { insert(before.item.i, value); return iterator{d.data(), before.item.i}; }
    iterator insert(const_iterator before, const QCborValue &value)
    { insert(before.item.i, value); return iterator{d.data(), before.item.i}; }
    iterator erase(iterator it) { removeAt(it.item.i); return iterator{d.data(), it.item.i}; }
    iterator erase(const_iterator it) { removeAt(it.item.i); return iterator{d.data(), it.item.i}; }

    void push_back(const QCborValue &t) { append(t); }
    void push_front(const QCborValue &t) { prepend(t); }
    void pop_front() { removeFirst(); }
    void pop_back() { removeLast(); }
    bool empty() const { return isEmpty(); }

    // convenience
    QCborArray operator+(const QCborValue &v) const
    { QCborArray n = *this; n += v; return n; }
    QCborArray &operator+=(const QCborValue &v)
    { append(v); return *this; }
    QCborArray &operator<<(const QCborValue &v)
    { append(v); return *this; }

    static QCborArray fromStringList(const QStringList &list);
    static QCborArray fromVariantList(const QVariantList &list);
    static QCborArray fromJsonArray(const QJsonArray &array);
    static QCborArray fromJsonArray(QJsonArray &&array) noexcept;
    QVariantList toVariantList() const;
    QJsonArray toJsonArray() const;

private:
    friend Q_CORE_EXPORT Q_DECL_PURE_FUNCTION bool
    comparesEqual(const QCborArray &lhs, const QCborArray &rhs) noexcept;
    friend Qt::strong_ordering compareThreeWay(const QCborArray &lhs,
                                               const QCborArray &rhs) noexcept
    {
        int c = lhs.compare(rhs);
        return Qt::compareThreeWay(c, 0);
    }
    Q_DECLARE_STRONGLY_ORDERED(QCborArray)

    static Q_DECL_PURE_FUNCTION bool
    comparesEqual_helper(const QCborArray &lhs, const QCborValue &rhs) noexcept;
    static Q_DECL_PURE_FUNCTION Qt::strong_ordering
    compareThreeWay_helper(const QCborArray &lhs, const QCborValue &rhs) noexcept;
    friend bool comparesEqual(const QCborArray &lhs,
                              const QCborValue &rhs) noexcept
    {
        return comparesEqual_helper(lhs, rhs);
    }
    friend Qt::strong_ordering compareThreeWay(const QCborArray &lhs,
                                               const QCborValue &rhs) noexcept
    {
        return compareThreeWay_helper(lhs, rhs);
    }
    Q_DECLARE_STRONGLY_ORDERED(QCborArray, QCborValue)

    static Q_DECL_PURE_FUNCTION bool
    comparesEqual_helper(const QCborArray &lhs, QCborValueConstRef rhs) noexcept;
    static Q_DECL_PURE_FUNCTION Qt::strong_ordering
    compareThreeWay_helper(const QCborArray &lhs, QCborValueConstRef rhs) noexcept;
    friend bool comparesEqual(const QCborArray &lhs,
                              const QCborValueConstRef &rhs) noexcept
    {
        return comparesEqual_helper(lhs, rhs);
    }
    friend Qt::strong_ordering compareThreeWay(const QCborArray &lhs,
                                               const QCborValueConstRef &rhs) noexcept
    {
        return compareThreeWay_helper(lhs, rhs);
    }
    Q_DECLARE_STRONGLY_ORDERED(QCborArray, QCborValueConstRef)

    void detach(qsizetype reserve = 0);

    friend QCborValue;
    friend QCborValueRef;
    friend class QJsonPrivate::Variant;
    explicit QCborArray(QCborContainerPrivate &dd) noexcept;
    QExplicitlySharedDataPointer<QCborContainerPrivate> d;
};

Q_DECLARE_SHARED(QCborArray)

inline QCborValue::QCborValue(QCborArray &&a)
    : n(-1), container(a.d.take()), t(Array)
{
}

#if QT_VERSION < QT_VERSION_CHECK(7, 0, 0) && !defined(QT_BOOTSTRAPPED)
inline QCborArray QCborValueRef::toArray() const
{
    return concrete().toArray();
}

inline QCborArray QCborValueRef::toArray(const QCborArray &a) const
{
    return concrete().toArray(a);
}
#endif

inline QCborArray QCborValueConstRef::toArray() const
{
    return concrete().toArray();
}

inline QCborArray QCborValueConstRef::toArray(const QCborArray &a) const
{
    return concrete().toArray(a);
}

Q_CORE_EXPORT size_t qHash(const QCborArray &array, size_t seed = 0);

#if !defined(QT_NO_DEBUG_STREAM)
Q_CORE_EXPORT QDebug operator<<(QDebug, const QCborArray &a);
#endif

#ifndef QT_NO_DATASTREAM
#if QT_CONFIG(cborstreamwriter)
Q_CORE_EXPORT QDataStream &operator<<(QDataStream &, const QCborArray &);
#endif
Q_CORE_EXPORT QDataStream &operator>>(QDataStream &, QCborArray &);
#endif

QT_END_NAMESPACE

#endif // QCBORARRAY_H
