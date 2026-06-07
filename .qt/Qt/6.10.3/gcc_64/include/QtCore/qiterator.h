// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QITERATOR_H
#define QITERATOR_H

#include <QtCore/qglobal.h>
#include <QtCore/qcontainertools_impl.h>

#ifdef __cpp_lib_ranges
#include <ranges>
#endif

QT_BEGIN_NAMESPACE

#if !defined(QT_NO_JAVA_STYLE_ITERATORS)

#ifdef Q_QDOC
#define Q_DISABLE_BACKWARD_ITERATOR
#else
#define Q_DISABLE_BACKWARD_ITERATOR \
        template<typename It = decltype(i), QtPrivate::IfIteratorCanMoveBackwards<It> = true>
#endif

#define Q_DECLARE_SEQUENTIAL_ITERATOR(C) \
\
template <class T> \
class Q##C##Iterator \
{ \
    typedef typename Q##C<T>::const_iterator const_iterator; \
    Q##C<T> c; \
    const_iterator i; \
public: \
    inline Q##C##Iterator(const Q##C<T> &container) \
        : c(container), i(c.constBegin()) {} \
    inline Q##C##Iterator &operator=(const Q##C<T> &container) \
    { c = container; i = c.constBegin(); return *this; } \
    inline void toFront() { i = c.constBegin(); } \
    inline void toBack() { i = c.constEnd(); } \
    inline bool hasNext() const { return i != c.constEnd(); } \
    inline const T &next() { return *i++; } \
    inline const T &peekNext() const { return *i; } \
    Q_DISABLE_BACKWARD_ITERATOR \
    inline bool hasPrevious() const { return i != c.constBegin(); } \
    Q_DISABLE_BACKWARD_ITERATOR \
    inline const T &previous() { return *--i; } \
    Q_DISABLE_BACKWARD_ITERATOR \
    inline const T &peekPrevious() const { const_iterator p = i; return *--p; } \
    inline bool findNext(const T &t) \
    { while (i != c.constEnd()) if (*i++ == t) return true; return false; } \
    Q_DISABLE_BACKWARD_ITERATOR \
    inline bool findPrevious(const T &t) \
    { while (i != c.constBegin()) if (*(--i) == t) return true; \
      return false;  } \
};

#define Q_DECLARE_MUTABLE_SEQUENTIAL_ITERATOR(C) \
\
template <class T> \
class QMutable##C##Iterator \
{ \
    typedef typename Q##C<T>::iterator iterator; \
    typedef typename Q##C<T>::const_iterator const_iterator; \
    Q##C<T> *c; \
    iterator i, n; \
    inline bool item_exists() const { return const_iterator(n) != c->constEnd(); } \
public: \
    inline QMutable##C##Iterator(Q##C<T> &container) \
        : c(&container) \
    { i = c->begin(); n = c->end(); } \
    inline QMutable##C##Iterator &operator=(Q##C<T> &container) \
    { c = &container; i = c->begin(); n = c->end(); return *this; } \
    inline void toFront() { i = c->begin(); n = c->end(); } \
    inline void toBack() { i = c->end(); n = i; } \
    inline bool hasNext() const { return c->constEnd() != const_iterator(i); } \
    inline T &next() { n = i++; return *n; } \
    inline T &peekNext() const { return *i; } \
    Q_DISABLE_BACKWARD_ITERATOR \
    inline bool hasPrevious() const { return c->constBegin() != const_iterator(i); } \
    Q_DISABLE_BACKWARD_ITERATOR \
    inline T &previous() { n = --i; return *n; } \
    Q_DISABLE_BACKWARD_ITERATOR \
    inline T &peekPrevious() const { iterator p = i; return *--p; } \
    inline void remove() \
    { if (c->constEnd() != const_iterator(n)) { i = c->erase(n); n = c->end(); } } \
    inline void setValue(const T &t) const { if (c->constEnd() != const_iterator(n)) *n = t; } \
    inline T &value() { Q_ASSERT(item_exists()); return *n; } \
    inline const T &value() const { Q_ASSERT(item_exists()); return *n; } \
    inline void insert(const T &t) { n = i = c->insert(i, t); ++i; } \
    inline bool findNext(const T &t) \
    { while (c->constEnd() != const_iterator(n = i)) if (*i++ == t) return true; return false; } \
    Q_DISABLE_BACKWARD_ITERATOR \
    inline bool findPrevious(const T &t) \
    { while (c->constBegin() != const_iterator(i)) if (*(n = --i) == t) return true; \
      n = c->end(); return false;  } \
};

#define Q_DECLARE_ASSOCIATIVE_ITERATOR(C) \
\
template <class Key, class T> \
class Q##C##Iterator \
{ \
    typedef typename Q##C<Key,T>::const_iterator const_iterator; \
    Q##C<Key,T> c; \
    const_iterator i, n; \
    inline bool item_exists() const { return n != c.constEnd(); } \
public: \
    typedef const_iterator Item; \
    inline Q##C##Iterator(const Q##C<Key,T> &container) \
        : c(container), i(c.constBegin()), n(c.constEnd()) {} \
    inline Q##C##Iterator &operator=(const Q##C<Key,T> &container) \
    { c = container; i = c.constBegin(); n = c.constEnd(); return *this; } \
    inline void toFront() { i = c.constBegin(); n = c.constEnd(); } \
    inline void toBack() { i = c.constEnd(); n = c.constEnd(); } \
    inline bool hasNext() const { return i != c.constEnd(); } \
    inline Item next() { n = i++; return n; } \
    inline Item peekNext() const { return i; } \
    Q_DISABLE_BACKWARD_ITERATOR \
    inline bool hasPrevious() const { return i != c.constBegin(); } \
    Q_DISABLE_BACKWARD_ITERATOR \
    inline Item previous() { n = --i; return n; } \
    Q_DISABLE_BACKWARD_ITERATOR \
    inline Item peekPrevious() const { const_iterator p = i; return --p; } \
    inline const T &value() const { Q_ASSERT(item_exists()); return *n; } \
    inline const Key &key() const { Q_ASSERT(item_exists()); return n.key(); } \
    inline bool findNext(const T &t) \
    { while ((n = i) != c.constEnd()) if (*i++ == t) return true; return false; } \
    Q_DISABLE_BACKWARD_ITERATOR \
    inline bool findPrevious(const T &t) \
    { while (i != c.constBegin()) if (*(n = --i) == t) return true; \
      n = c.constEnd(); return false; } \
};

#define Q_DECLARE_MUTABLE_ASSOCIATIVE_ITERATOR(C) \
\
template <class Key, class T> \
class QMutable##C##Iterator \
{ \
    typedef typename Q##C<Key,T>::iterator iterator; \
    typedef typename Q##C<Key,T>::const_iterator const_iterator; \
    Q##C<Key,T> *c; \
    iterator i, n; \
    inline bool item_exists() const { return const_iterator(n) != c->constEnd(); } \
public: \
    typedef iterator Item; \
    inline QMutable##C##Iterator(Q##C<Key,T> &container) \
        : c(&container) \
    { i = c->begin(); n = c->end(); } \
    inline QMutable##C##Iterator &operator=(Q##C<Key,T> &container) \
    { c = &container; i = c->begin(); n = c->end(); return *this; } \
    inline void toFront() { i = c->begin(); n = c->end(); } \
    inline void toBack() { i = c->end(); n = c->end(); } \
    inline bool hasNext() const { return const_iterator(i) != c->constEnd(); } \
    inline Item next() { n = i++; return n; } \
    inline Item peekNext() const { return i; } \
    Q_DISABLE_BACKWARD_ITERATOR \
    inline bool hasPrevious() const { return const_iterator(i) != c->constBegin(); } \
    Q_DISABLE_BACKWARD_ITERATOR \
    inline Item previous() { n = --i; return n; } \
    Q_DISABLE_BACKWARD_ITERATOR \
    inline Item peekPrevious() const { iterator p = i; return --p; } \
    inline void remove() \
    { if (const_iterator(n) != c->constEnd()) { i = c->erase(n); n = c->end(); } } \
    inline void setValue(const T &t) { if (const_iterator(n) != c->constEnd()) *n = t; } \
    inline T &value() { Q_ASSERT(item_exists()); return *n; } \
    inline const T &value() const { Q_ASSERT(item_exists()); return *n; } \
    inline const Key &key() const { Q_ASSERT(item_exists()); return n.key(); } \
    inline bool findNext(const T &t) \
    { while (const_iterator(n = i) != c->constEnd()) if (*i++ == t) return true; return false; } \
    Q_DISABLE_BACKWARD_ITERATOR \
    inline bool findPrevious(const T &t) \
    { while (const_iterator(i) != c->constBegin()) if (*(n = --i) == t) return true; \
      n = c->end(); return false; } \
};

#define Q_DECLARE_ASSOCIATIVE_FORWARD_ITERATOR(C) \
\
template <class Key, class T> \
class Q##C##Iterator \
{ \
    typedef typename Q##C<Key,T>::const_iterator const_iterator; \
    Q##C<Key,T> c; \
    const_iterator i, n; \
    inline bool item_exists() const { return n != c.constEnd(); } \
public: \
    typedef const_iterator Item; \
    inline Q##C##Iterator(const Q##C<Key,T> &container) \
        : c(container), i(c.constBegin()), n(c.constEnd()) {} \
    inline Q##C##Iterator &operator=(const Q##C<Key,T> &container) \
    { c = container; i = c.constBegin(); n = c.constEnd(); return *this; } \
    inline void toFront() { i = c.constBegin(); n = c.constEnd(); } \
    inline void toBack() { i = c.constEnd(); n = c.constEnd(); } \
    inline bool hasNext() const { return i != c.constEnd(); } \
    inline Item next() { n = i++; return n; } \
    inline Item peekNext() const { return i; } \
    inline const T &value() const { Q_ASSERT(item_exists()); return *n; } \
    inline const Key &key() const { Q_ASSERT(item_exists()); return n.key(); } \
    inline bool findNext(const T &t) \
    { while ((n = i) != c.constEnd()) if (*i++ == t) return true; return false; } \
};

#define Q_DECLARE_MUTABLE_ASSOCIATIVE_FORWARD_ITERATOR(C) \
\
template <class Key, class T> \
class QMutable##C##Iterator \
{ \
    typedef typename Q##C<Key,T>::iterator iterator; \
    typedef typename Q##C<Key,T>::const_iterator const_iterator; \
    Q##C<Key,T> *c; \
    iterator i, n; \
    inline bool item_exists() const { return const_iterator(n) != c->constEnd(); } \
public: \
    typedef iterator Item; \
    inline QMutable##C##Iterator(Q##C<Key,T> &container) \
        : c(&container) \
    { i = c->begin(); n = c->end(); } \
    inline QMutable##C##Iterator &operator=(Q##C<Key,T> &container) \
    { c = &container; i = c->begin(); n = c->end(); return *this; } \
    inline void toFront() { i = c->begin(); n = c->end(); } \
    inline void toBack() { i = c->end(); n = c->end(); } \
    inline bool hasNext() const { return const_iterator(i) != c->constEnd(); } \
    inline Item next() { n = i++; return n; } \
    inline Item peekNext() const { return i; } \
    inline void remove() \
    { if (const_iterator(n) != c->constEnd()) { i = c->erase(n); n = c->end(); } } \
    inline void setValue(const T &t) { if (const_iterator(n) != c->constEnd()) *n = t; } \
    inline T &value() { Q_ASSERT(item_exists()); return *n; } \
    inline const T &value() const { Q_ASSERT(item_exists()); return *n; } \
    inline const Key &key() const { Q_ASSERT(item_exists()); return n.key(); } \
    inline bool findNext(const T &t) \
    { while (const_iterator(n = i) != c->constEnd()) if (*i++ == t) return true; return false; } \
};


#else // QT_NO_JAVA_STYLE_ITERATORS
#define Q_DECLARE_SEQUENTIAL_ITERATOR(C)
#define Q_DECLARE_MUTABLE_SEQUENTIAL_ITERATOR(C)
#define Q_DECLARE_ASSOCIATIVE_ITERATOR(C)
#define Q_DECLARE_MUTABLE_ASSOCIATIVE_ITERATOR(C)
#define Q_DECLARE_ASSOCIATIVE_FORWARD_ITERATOR(C)
#define Q_DECLARE_MUTABLE_ASSOCIATIVE_FORWARD_ITERATOR(C)
#endif // QT_NO_JAVA_STYLE_ITERATORS

namespace QtPrivate {

template <typename Key, typename T, typename Iterator>
struct QDefaultKeyValues
{
    static Key key(const Iterator &it) { return it.key(); }
    static Key key(Iterator &it) { return it.key(); }
    static T value(const Iterator &it) { return it.value(); }
    static T value(Iterator &it) { return it.value(); }
};

} // namespace QtPrivate

template <typename Key, typename T, class Iterator,
          class Traits = QtPrivate::QDefaultKeyValues<Key, T, Iterator>>
class QKeyValueIterator
{
public:
    typedef typename Iterator::iterator_category iterator_category;
    typedef typename Iterator::difference_type difference_type;
    typedef std::pair<Key, T> value_type;
    typedef const value_type &reference;

    QKeyValueIterator() = default;
    constexpr explicit QKeyValueIterator(Iterator o) noexcept(std::is_nothrow_move_constructible<Iterator>::value)
        : i(std::move(o)) {}

    std::pair<Key, T> operator*() const {
        return std::pair<Key, T>(Traits::key(i), Traits::value(i));
    }

    using pointer = QtPrivate::ArrowProxy<value_type>;

    pointer operator->() const {
        return pointer{ std::pair<Key, T>(Traits::key(i), Traits::value(i)) };
    }

    friend bool operator==(QKeyValueIterator lhs, QKeyValueIterator rhs) noexcept { return lhs.i == rhs.i; }
    friend bool operator!=(QKeyValueIterator lhs, QKeyValueIterator rhs) noexcept { return lhs.i != rhs.i; }

    inline QKeyValueIterator &operator++() { ++i; return *this; }
    inline QKeyValueIterator operator++(int) { return QKeyValueIterator(i++);}
    inline QKeyValueIterator &operator--() { --i; return *this; }
    inline QKeyValueIterator operator--(int) { return QKeyValueIterator(i--); }
    Iterator base() const { return i; }

private:
    Iterator i;
};

namespace QtPrivate {

template <typename Map>
class QKeyValueRangeStorage
{
protected:
    Map m_map;
    Map &map() { return m_map; }
    const Map &map() const { return m_map; }
public:
    explicit QKeyValueRangeStorage(const Map &map) : m_map(map) {}
    explicit QKeyValueRangeStorage(Map &&map) : m_map(std::move(map)) {}
};

template <typename Map>
class QKeyValueRangeStorage<Map &>
#ifdef __cpp_lib_ranges
    : public std::ranges::view_base
#endif
{
protected:
    Map *m_map;
    Map &map() { return *m_map; }
    const Map &map() const { return *m_map; }
public:
    explicit QKeyValueRangeStorage(Map &map) : m_map(&map) {}
};

template <typename Map>
class QKeyValueRange : public QKeyValueRangeStorage<Map>
{
public:
    using QKeyValueRangeStorage<Map>::QKeyValueRangeStorage;
    auto begin() { return this->map().keyValueBegin(); }
    auto begin() const { return this->map().keyValueBegin(); }
    auto end() { return this->map().keyValueEnd(); }
    auto end() const { return this->map().keyValueEnd(); }
};

} // namespace QtPrivate


QT_END_NAMESPACE

#endif // QITERATOR_H
