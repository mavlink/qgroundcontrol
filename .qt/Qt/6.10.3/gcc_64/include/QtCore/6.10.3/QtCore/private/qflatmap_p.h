// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QFLATMAP_P_H
#define QFLATMAP_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of a number of Qt sources files.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qcontainertools_impl.h>
#include "qlist.h"
#include <QtCore/qtclasshelpermacros.h>
#include "private/qglobal_p.h"

#include <algorithm>
#include <functional>
#include <initializer_list>
#include <iterator>
#include <numeric>
#include <type_traits>
#include <utility>
#include <vector>

QT_BEGIN_NAMESPACE

/*
  QFlatMap provides an associative container backed by sorted sequential
  containers. By default, QList is used.

  Keys and values are stored in two separate containers. This provides improved
  cache locality for key iteration and makes keys() and values() fast
  operations.

  One can customize the underlying container type by passing the KeyContainer
  and MappedContainer template arguments:
      QFlatMap<float, int, std::less<float>, std::vector<float>, std::vector<int>>
*/

namespace Qt {

QT_DEFINE_TAG(OrderedUniqueRange);

} // namespace Qt

template <class Key, class T, class Compare>
class QFlatMapValueCompare : protected Compare
{
public:
    QFlatMapValueCompare() = default;
    QFlatMapValueCompare(const Compare &key_compare)
        : Compare(key_compare)
    {
    }

    using value_type = std::pair<const Key, T>;
    static constexpr bool is_comparator_noexcept = noexcept(
        std::declval<Compare>()(std::declval<const Key &>(), std::declval<const Key &>()));

    bool operator()(const value_type &lhs, const value_type &rhs) const
        noexcept(is_comparator_noexcept)
    {
        return Compare::operator()(lhs.first, rhs.first);
    }
};

template<class Key, class T, class Compare = std::less<Key>, class KeyContainer = QList<Key>,
         class MappedContainer = QList<T>>
class QFlatMap : private QFlatMapValueCompare<Key, T, Compare>
{
    static_assert(std::is_nothrow_destructible_v<T>, "Types with throwing destructors are not supported in Qt containers.");
public:
    using key_type = Key;
    using mapped_type = T;
    using value_compare = QFlatMapValueCompare<Key, T, Compare>;
    using value_type = typename value_compare::value_type;
    using key_container_type = KeyContainer;
    using mapped_container_type = MappedContainer;
    using size_type = typename key_container_type::size_type;
    using key_compare = Compare;

    struct containers
    {
        key_container_type keys;
        mapped_container_type values;
    };

    class iterator
    {
    public:
        using difference_type = ptrdiff_t;
        using value_type = std::pair<const Key, T>;
        using reference = std::pair<const Key &, T &>;
        using pointer = QtPrivate::ArrowProxy<reference>;
        using iterator_category = std::random_access_iterator_tag;

        iterator() = default;

        iterator(containers *ac, size_type ai)
            : c(ac), i(ai)
        {
        }

        reference operator*() const
        {
            return { c->keys[i], c->values[i] };
        }

        pointer operator->() const
        {
            return { operator*() };
        }

        bool operator==(const iterator &o) const
        {
            return c == o.c && i == o.i;
        }

        bool operator!=(const iterator &o) const
        {
            return !operator==(o);
        }

        iterator &operator++()
        {
            ++i;
            return *this;
        }

        iterator operator++(int)
        {

            iterator r = *this;
            ++*this;
            return r;
        }

        iterator &operator--()
        {
            --i;
            return *this;
        }

        iterator operator--(int)
        {
            iterator r = *this;
            --*this;
            return r;
        }

        iterator &operator+=(size_type n)
        {
            i += n;
            return *this;
        }

        friend iterator operator+(size_type n, const iterator a)
        {
            iterator ret = a;
            return ret += n;
        }

        friend iterator operator+(const iterator a, size_type n)
        {
            return n + a;
        }

        iterator &operator-=(size_type n)
        {
            i -= n;
            return *this;
        }

        friend iterator operator-(const iterator a, size_type n)
        {
            iterator ret = a;
            return ret -= n;
        }

        friend difference_type operator-(const iterator b, const iterator a)
        {
            return b.i - a.i;
        }

        reference operator[](size_type n) const
        {
            size_type k = i + n;
            return { c->keys[k], c->values[k] };
        }

        bool operator<(const iterator &other) const
        {
            return i < other.i;
        }

        bool operator>(const iterator &other) const
        {
            return i > other.i;
        }

        bool operator<=(const iterator &other) const
        {
            return i <= other.i;
        }

        bool operator>=(const iterator &other) const
        {
            return i >= other.i;
        }

        const Key &key() const { return c->keys[i]; }
        T &value() const { return c->values[i]; }

    private:
        containers *c = nullptr;
        size_type i = 0;
        friend QFlatMap;
    };

    class const_iterator
    {
    public:
        using difference_type = ptrdiff_t;
        using value_type = std::pair<const Key, const T>;
        using reference = std::pair<const Key &, const T &>;
        using pointer = QtPrivate::ArrowProxy<reference>;
        using iterator_category = std::random_access_iterator_tag;

        const_iterator() = default;

        const_iterator(const containers *ac, size_type ai)
            : c(ac), i(ai)
        {
        }

        const_iterator(iterator o)
            : c(o.c), i(o.i)
        {
        }

        reference operator*() const
        {
            return { c->keys[i], c->values[i] };
        }

        pointer operator->() const
        {
            return { operator*() };
        }

        bool operator==(const const_iterator &o) const
        {
            return c == o.c && i == o.i;
        }

        bool operator!=(const const_iterator &o) const
        {
            return !operator==(o);
        }

        const_iterator &operator++()
        {
            ++i;
            return *this;
        }

        const_iterator operator++(int)
        {

            const_iterator r = *this;
            ++*this;
            return r;
        }

        const_iterator &operator--()
        {
            --i;
            return *this;
        }

        const_iterator operator--(int)
        {
            const_iterator r = *this;
            --*this;
            return r;
        }

        const_iterator &operator+=(size_type n)
        {
            i += n;
            return *this;
        }

        friend const_iterator operator+(size_type n, const const_iterator a)
        {
            const_iterator ret = a;
            return ret += n;
        }

        friend const_iterator operator+(const const_iterator a, size_type n)
        {
            return n + a;
        }

        const_iterator &operator-=(size_type n)
        {
            i -= n;
            return *this;
        }

        friend const_iterator operator-(const const_iterator a, size_type n)
        {
            const_iterator ret = a;
            return ret -= n;
        }

        friend difference_type operator-(const const_iterator b, const const_iterator a)
        {
            return b.i - a.i;
        }

        reference operator[](size_type n) const
        {
            size_type k = i + n;
            return { c->keys[k], c->values[k] };
        }

        bool operator<(const const_iterator &other) const
        {
            return i < other.i;
        }

        bool operator>(const const_iterator &other) const
        {
            return i > other.i;
        }

        bool operator<=(const const_iterator &other) const
        {
            return i <= other.i;
        }

        bool operator>=(const const_iterator &other) const
        {
            return i >= other.i;
        }

        const Key &key() const { return c->keys[i]; }
        const T &value() const { return c->values[i]; }

    private:
        const containers *c = nullptr;
        size_type i = 0;
        friend QFlatMap;
    };

private:
    template <class, class = void>
    struct is_marked_transparent_type : std::false_type { };

    template <class X>
    struct is_marked_transparent_type<X, std::void_t<typename X::is_transparent>> : std::true_type { };

    template <class X>
    using is_marked_transparent = typename std::enable_if<
        is_marked_transparent_type<X>::value>::type *;

    template <typename It>
    using is_compatible_iterator = typename std::enable_if<
        std::is_same<value_type, typename std::iterator_traits<It>::value_type>::value>::type *;

public:
    QFlatMap() = default;

    explicit QFlatMap(const key_container_type &keys, const mapped_container_type &values)
        : c{keys, values}
    {
        ensureOrderedUnique();
    }

    explicit QFlatMap(key_container_type &&keys, const mapped_container_type &values)
        : c{std::move(keys), values}
    {
        ensureOrderedUnique();
    }

    explicit QFlatMap(const key_container_type &keys, mapped_container_type &&values)
        : c{keys, std::move(values)}
    {
        ensureOrderedUnique();
    }

    explicit QFlatMap(key_container_type &&keys, mapped_container_type &&values)
        : c{std::move(keys), std::move(values)}
    {
        ensureOrderedUnique();
    }

    explicit QFlatMap(std::initializer_list<value_type> lst)
        : QFlatMap(lst.begin(), lst.end())
    {
    }

    template <class InputIt, is_compatible_iterator<InputIt> = nullptr>
    explicit QFlatMap(InputIt first, InputIt last)
    {
        initWithRange(first, last);
        ensureOrderedUnique();
    }

    explicit QFlatMap(Qt::OrderedUniqueRange_t, const key_container_type &keys,
                      const mapped_container_type &values)
        : c{keys, values}
    {
    }

    explicit QFlatMap(Qt::OrderedUniqueRange_t, key_container_type &&keys,
                      const mapped_container_type &values)
        : c{std::move(keys), values}
    {
    }

    explicit QFlatMap(Qt::OrderedUniqueRange_t, const key_container_type &keys,
                      mapped_container_type &&values)
        : c{keys, std::move(values)}
    {
    }

    explicit QFlatMap(Qt::OrderedUniqueRange_t, key_container_type &&keys,
                      mapped_container_type &&values)
        : c{std::move(keys), std::move(values)}
    {
    }

    explicit QFlatMap(Qt::OrderedUniqueRange_t, std::initializer_list<value_type> lst)
        : QFlatMap(Qt::OrderedUniqueRange, lst.begin(), lst.end())
    {
    }

    template <class InputIt, is_compatible_iterator<InputIt> = nullptr>
    explicit QFlatMap(Qt::OrderedUniqueRange_t, InputIt first, InputIt last)
    {
        initWithRange(first, last);
    }

    explicit QFlatMap(const Compare &compare)
        : value_compare(compare)
    {
    }

    explicit QFlatMap(const key_container_type &keys, const mapped_container_type &values,
                      const Compare &compare)
        : value_compare(compare), c{keys, values}
    {
        ensureOrderedUnique();
    }

    explicit QFlatMap(key_container_type &&keys, const mapped_container_type &values,
                      const Compare &compare)
        : value_compare(compare), c{std::move(keys), values}
    {
        ensureOrderedUnique();
    }

    explicit QFlatMap(const key_container_type &keys, mapped_container_type &&values,
                      const Compare &compare)
        : value_compare(compare), c{keys, std::move(values)}
    {
        ensureOrderedUnique();
    }

    explicit QFlatMap(key_container_type &&keys, mapped_container_type &&values,
                      const Compare &compare)
        : value_compare(compare), c{std::move(keys), std::move(values)}
    {
        ensureOrderedUnique();
    }

    explicit QFlatMap(std::initializer_list<value_type> lst, const Compare &compare)
        : QFlatMap(lst.begin(), lst.end(), compare)
    {
    }

    template <class InputIt, is_compatible_iterator<InputIt> = nullptr>
    explicit QFlatMap(InputIt first, InputIt last, const Compare &compare)
        : value_compare(compare)
    {
        initWithRange(first, last);
        ensureOrderedUnique();
    }

    explicit QFlatMap(Qt::OrderedUniqueRange_t, const key_container_type &keys,
                      const mapped_container_type &values, const Compare &compare)
        : value_compare(compare), c{keys, values}
    {
    }

    explicit QFlatMap(Qt::OrderedUniqueRange_t, key_container_type &&keys,
                      const mapped_container_type &values, const Compare &compare)
        : value_compare(compare), c{std::move(keys), values}
    {
    }

    explicit QFlatMap(Qt::OrderedUniqueRange_t, const key_container_type &keys,
                      mapped_container_type &&values, const Compare &compare)
        : value_compare(compare), c{keys, std::move(values)}
    {
    }

    explicit QFlatMap(Qt::OrderedUniqueRange_t, key_container_type &&keys,
                      mapped_container_type &&values, const Compare &compare)
        : value_compare(compare), c{std::move(keys), std::move(values)}
    {
    }

    explicit QFlatMap(Qt::OrderedUniqueRange_t, std::initializer_list<value_type> lst,
                      const Compare &compare)
        : QFlatMap(Qt::OrderedUniqueRange, lst.begin(), lst.end(), compare)
    {
    }

    template <class InputIt, is_compatible_iterator<InputIt> = nullptr>
    explicit QFlatMap(Qt::OrderedUniqueRange_t, InputIt first, InputIt last, const Compare &compare)
        : value_compare(compare)
    {
        initWithRange(first, last);
    }

    size_type count() const noexcept { return c.keys.size(); }
    size_type size() const noexcept { return c.keys.size(); }
    size_type capacity() const noexcept { return c.keys.capacity(); }
    bool isEmpty() const noexcept { return c.keys.empty(); }
    bool empty() const noexcept { return c.keys.empty(); }
    containers extract() && { return std::move(c); }
    const key_container_type &keys() const noexcept { return c.keys; }
    const mapped_container_type &values() const noexcept { return c.values; }

    void reserve(size_type s)
    {
        c.keys.reserve(s);
        c.values.reserve(s);
    }

    void clear()
    {
        c.keys.clear();
        c.values.clear();
    }

    bool remove(const Key &key)
    {
        return do_remove(find(key));
    }

    template <class X, class Y = Compare, is_marked_transparent<Y> = nullptr>
    bool remove(const X &key)
    {
        return do_remove(find(key));
    }

    iterator erase(iterator it)
    {
        c.values.erase(toValuesIterator(it));
        return fromKeysIterator(c.keys.erase(toKeysIterator(it)));
    }

    T take(const Key &key)
    {
        return do_take(find(key));
    }

    template <class X, class Y = Compare, is_marked_transparent<Y> = nullptr>
    T take(const X &key)
    {
        return do_take(find(key));
    }

    bool contains(const Key &key) const
    {
        return find(key) != end();
    }

    template <class X, class Y = Compare, is_marked_transparent<Y> = nullptr>
    bool contains(const X &key) const
    {
        return find(key) != end();
    }

    T value(const Key &key, const T &defaultValue) const
    {
        auto it = find(key);
        return it == end() ? defaultValue : it.value();
    }

    template <class X, class Y = Compare, is_marked_transparent<Y> = nullptr>
    T value(const X &key, const T &defaultValue) const
    {
        auto it = find(key);
        return it == end() ? defaultValue : it.value();
    }

    T value(const Key &key) const
    {
        auto it = find(key);
        if (it == end())
            return T();
        return it.value();
    }

    template <class X, class Y = Compare, is_marked_transparent<Y> = nullptr>
    T value(const X &key) const
    {
        auto it = find(key);
        if (it == end())
            return T();
        return it.value();
    }

    T &operator[](const Key &key)
    {
        return try_emplace(key).first.value();
    }

    T &operator[](Key &&key)
    {
        return try_emplace(std::move(key)).first.value();
    }

    T operator[](const Key &key) const
    {
        return value(key);
    }

    std::pair<iterator, bool> insert(const Key &key, const T &value)
    {
        return try_emplace(key, value);
    }

    std::pair<iterator, bool> insert(Key &&key, const T &value)
    {
        return try_emplace(std::move(key), value);
    }

    std::pair<iterator, bool> insert(const Key &key, T &&value)
    {
        return try_emplace(key, std::move(value));
    }

    std::pair<iterator, bool> insert(Key &&key, T &&value)
    {
        return try_emplace(std::move(key), std::move(value));
    }

    template <typename...Args>
    std::pair<iterator, bool> try_emplace(const Key &key, Args&&...args)
    {
        auto it = lower_bound(key);
        if (it == end() || key_compare::operator()(key, it.key())) {
            c.values.emplace(toValuesIterator(it), std::forward<Args>(args)...);
            return { fromKeysIterator(c.keys.insert(toKeysIterator(it), key)), true };
        } else {
            return {it, false};
        }
    }

    template <typename...Args>
    std::pair<iterator, bool> try_emplace(Key &&key, Args&&...args)
    {
        auto it = lower_bound(key);
        if (it == end() || key_compare::operator()(key, it.key())) {
            c.values.emplace(toValuesIterator(it), std::forward<Args>(args)...);
            return { fromKeysIterator(c.keys.insert(toKeysIterator(it), std::move(key))), true };
        } else {
            return {it, false};
        }
    }

    template <typename M>
    std::pair<iterator, bool> insert_or_assign(const Key &key, M &&obj)
    {
        auto r = try_emplace(key, std::forward<M>(obj));
        if (!r.second)
            *toValuesIterator(r.first) = std::forward<M>(obj);
        return r;
    }

    template <typename M>
    std::pair<iterator, bool> insert_or_assign(Key &&key, M &&obj)
    {
        auto r = try_emplace(std::move(key), std::forward<M>(obj));
        if (!r.second)
            *toValuesIterator(r.first) = std::forward<M>(obj);
        return r;
    }

    template <class InputIt, is_compatible_iterator<InputIt> = nullptr>
    void insert(InputIt first, InputIt last)
    {
        insertRange(first, last);
    }

    // ### Merge with the templated version above
    //     once we can use std::disjunction in is_compatible_iterator.
    void insert(const value_type *first, const value_type *last)
    {
        insertRange(first, last);
    }

    template <class InputIt, is_compatible_iterator<InputIt> = nullptr>
    void insert(Qt::OrderedUniqueRange_t, InputIt first, InputIt last)
    {
        insertOrderedUniqueRange(first, last);
    }

    // ### Merge with the templated version above
    //     once we can use std::disjunction in is_compatible_iterator.
    void insert(Qt::OrderedUniqueRange_t, const value_type *first, const value_type *last)
    {
        insertOrderedUniqueRange(first, last);
    }

    iterator begin() { return { &c, 0 }; }
    const_iterator begin() const { return { &c, 0 }; }
    const_iterator cbegin() const { return begin(); }
    const_iterator constBegin() const { return cbegin(); }
    iterator end() { return { &c, c.keys.size() }; }
    const_iterator end() const { return { &c, c.keys.size() }; }
    const_iterator cend() const { return end(); }
    const_iterator constEnd() const { return cend(); }
    std::reverse_iterator<iterator> rbegin() { return std::reverse_iterator<iterator>(end()); }
    std::reverse_iterator<const_iterator> rbegin() const
    {
        return std::reverse_iterator<const_iterator>(end());
    }
    std::reverse_iterator<const_iterator> crbegin() const { return rbegin(); }
    std::reverse_iterator<iterator> rend() {
        return std::reverse_iterator<iterator>(begin());
    }
    std::reverse_iterator<const_iterator> rend() const
    {
        return std::reverse_iterator<const_iterator>(begin());
    }
    std::reverse_iterator<const_iterator> crend() const { return rend(); }

    iterator lower_bound(const Key &key)
    {
        auto cit = std::as_const(*this).lower_bound(key);
        return { &c, cit.i };
    }

    template <class X, class Y = Compare, is_marked_transparent<Y> = nullptr>
    iterator lower_bound(const X &key)
    {
        auto cit = std::as_const(*this).lower_bound(key);
        return { &c, cit.i };
    }

    const_iterator lower_bound(const Key &key) const
    {
        return fromKeysIterator(std::lower_bound(c.keys.begin(), c.keys.end(), key, key_comp()));
    }

    template <class X, class Y = Compare, is_marked_transparent<Y> = nullptr>
    const_iterator lower_bound(const X &key) const
    {
        return fromKeysIterator(std::lower_bound(c.keys.begin(), c.keys.end(), key, key_comp()));
    }

    iterator find(const Key &key)
    {
        return { &c, std::as_const(*this).find(key).i };
    }

    template <class X, class Y = Compare, is_marked_transparent<Y> = nullptr>
    iterator find(const X &key)
    {
        return { &c, std::as_const(*this).find(key).i };
    }

    const_iterator find(const Key &key) const
    {
        auto it = lower_bound(key);
        if (it != end()) {
            if (!key_compare::operator()(key, it.key()))
                return it;
            it = end();
        }
        return it;
    }

    template <class X, class Y = Compare, is_marked_transparent<Y> = nullptr>
    const_iterator find(const X &key) const
    {
        auto it = lower_bound(key);
        if (it != end()) {
            if (!key_compare::operator()(key, it.key()))
                return it;
            it = end();
        }
        return it;
    }

    template <typename Predicate>
    size_type remove_if(Predicate pred)
    {
        const auto indirect_call_to_pred = [pred = std::move(pred)](iterator it) {
            using Pair = decltype(*it);
            using K = decltype(it.key());
            using V = decltype(it.value());
            using P = Predicate;
            if constexpr (std::is_invocable_v<P, K, V>) {
                return pred(it.key(), it.value());
            } else if constexpr (std::is_invocable_v<P, Pair> && !std::is_invocable_v<P, K>) {
                return pred(*it);
            } else if constexpr (std::is_invocable_v<P, K> && !std::is_invocable_v<P, Pair>) {
                return pred(it.key());
            } else {
                static_assert(QtPrivate::type_dependent_false<Predicate>(),
                    "Don't know how to call the predicate.\n"
                    "Options:\n"
                    "- pred(*it)\n"
                    "- pred(it.key(), it.value())\n"
                    "- pred(it.key())");
            }
        };

        auto first = begin();
        const auto last = end();

        // find_if prefix loop
        while (first != last && !indirect_call_to_pred(first))
            ++first;

        if (first == last)
            return 0; // nothing to do

        // we know that we need to remove *first

        auto kdest = toKeysIterator(first);
        auto vdest = toValuesIterator(first);

        ++first;

        auto k = std::next(kdest);
        auto v = std::next(vdest);

        // Main Loop
        // - first is used only for indirect_call_to_pred
        // - operations are done on k, v
        // Loop invariants:
        // - first, k, v are pointing to the same element
        // - [begin(), first[, [c.keys.begin(), k[, [c.values.begin(), v[: already processed
        // - [first, end()[,   [k, c.keys.end()[,   [v, c.values.end()[:   still to be processed
        // - [c.keys.begin(), kdest[ and [c.values.begin(), vdest[ are keepers
        // - [kdest, k[, [vdest, v[ are considered removed
        // - kdest is not c.keys.end()
        // - vdest is not v.values.end()
        while (first != last) {
            if (!indirect_call_to_pred(first)) {
                // keep *first, aka {*k, *v}
                *kdest = std::move(*k);
                *vdest = std::move(*v);
                ++kdest;
                ++vdest;
            }
            ++k;
            ++v;
            ++first;
        }

        const size_type r = std::distance(kdest, c.keys.end());
        c.keys.erase(kdest, c.keys.end());
        c.values.erase(vdest, c.values.end());
        return r;
    }

    key_compare key_comp() const noexcept
    {
        return static_cast<key_compare>(*this);
    }

    value_compare value_comp() const noexcept
    {
        return static_cast<value_compare>(*this);
    }

private:
    bool do_remove(iterator it)
    {
        if (it != end()) {
            erase(it);
            return true;
        }
        return false;
    }

    T do_take(iterator it)
    {
        if (it == end())
            return {};
        return [&] {
            T result = std::move(it.value());
            erase(it);
            return result;
        }();
    }

    template <class InputIt, is_compatible_iterator<InputIt> = nullptr>
    void initWithRange(InputIt first, InputIt last)
    {
        QtPrivate::reserveIfForwardIterator(this, first, last);
        while (first != last) {
            c.keys.push_back(first->first);
            c.values.push_back(first->second);
            ++first;
        }
    }

    iterator fromKeysIterator(typename key_container_type::iterator kit)
    {
        return { &c, static_cast<size_type>(std::distance(c.keys.begin(), kit)) };
    }

    const_iterator fromKeysIterator(typename key_container_type::const_iterator kit) const
    {
        return { &c, static_cast<size_type>(std::distance(c.keys.begin(), kit)) };
    }

    typename key_container_type::iterator toKeysIterator(iterator it)
    {
        return c.keys.begin() + it.i;
    }

    typename mapped_container_type::iterator toValuesIterator(iterator it)
    {
        return c.values.begin() + it.i;
    }

    template <class InputIt>
    void insertRange(InputIt first, InputIt last)
    {
        size_type i = c.keys.size();
        c.keys.resize(i + std::distance(first, last));
        c.values.resize(c.keys.size());
        for (; first != last; ++first, ++i) {
            c.keys[i] = first->first;
            c.values[i] = first->second;
        }
        ensureOrderedUnique();
    }

    class IndexedKeyComparator
    {
    public:
        IndexedKeyComparator(const QFlatMap *am)
            : m(am)
        {
        }

        bool operator()(size_type i, size_type k) const
        {
            return m->key_comp()(m->c.keys[i], m->c.keys[k]);
        }

    private:
        const QFlatMap *m;
    };

    template <class InputIt>
    void insertOrderedUniqueRange(InputIt first, InputIt last)
    {
        const size_type s = c.keys.size();
        c.keys.resize(s + std::distance(first, last));
        c.values.resize(c.keys.size());
        for (size_type i = s; first != last; ++first, ++i) {
            c.keys[i] = first->first;
            c.values[i] = first->second;
        }

        std::vector<size_type> p(size_t(c.keys.size()));
        std::iota(p.begin(), p.end(), 0);
        std::inplace_merge(p.begin(), p.begin() + s, p.end(), IndexedKeyComparator(this));
        applyPermutation(p);
        makeUnique();
    }

    void ensureOrderedUnique()
    {
        std::vector<size_type> p(size_t(c.keys.size()));
        std::iota(p.begin(), p.end(), 0);
        std::stable_sort(p.begin(), p.end(), IndexedKeyComparator(this));
        applyPermutation(p);
        makeUnique();
    }

    void applyPermutation(const std::vector<size_type> &p)
    {
        const size_type s = c.keys.size();
        std::vector<bool> done(s);
        for (size_type i = 0; i < s; ++i) {
            if (done[i])
                continue;
            done[i] = true;
            size_type j = i;
            size_type k = p[i];
            while (i != k) {
                qSwap(c.keys[j], c.keys[k]);
                qSwap(c.values[j], c.values[k]);
                done[k] = true;
                j = k;
                k = p[j];
            }
        }
    }

    void makeUnique()
    {
        // std::unique, but over two ranges
        auto equivalent = [this](const auto &lhs, const auto &rhs) {
            return !key_compare::operator()(lhs, rhs) && !key_compare::operator()(rhs, lhs);
        };
        const auto kb = c.keys.begin();
        const auto ke = c.keys.end();
        auto k = std::adjacent_find(kb, ke, equivalent);
        if (k == ke)
            return;

        // equivalent keys found, we need to do actual work:
        auto v = std::next(c.values.begin(), std::distance(kb, k));

        auto kdest = k;
        auto vdest = v;

        ++k;
        ++v;

        // Loop Invariants:
        //
        // - [keys.begin(), kdest] and [values.begin(), vdest] are unique
        // - k is not keys.end(), v is not values.end()
        // - [next(k), keys.end()[ and [next(v), values.end()[ still need to be checked
        while ((++v, ++k) != ke) {
            if (!equivalent(*kdest, *k)) {
                *++kdest = std::move(*k);
                *++vdest = std::move(*v);
            }
        }

        c.keys.erase(std::next(kdest), ke);
        c.values.erase(std::next(vdest), c.values.end());
    }

    containers c;
};

template <class Key, class T,
          qsizetype N = QVarLengthArrayDefaultPrealloc,
          class Compare = std::less<Key>>
using QVarLengthFlatMap = QFlatMap<Key, T, Compare, QVarLengthArray<Key, N>, QVarLengthArray<T, N>>;

QT_END_NAMESPACE

#endif // QFLATMAP_P_H
