// Copyright (C) 2020 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Marc Mutz <marc.mutz@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QDUPLICATETRACKER_P_H
#define QDUPLICATETRACKER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <private/qglobal_p.h>

#ifdef __cpp_lib_memory_resource
#  include <unordered_set>
#  include <memory_resource>
#  include <qhash.h> // for the hashing helpers
#else
#  include <qset.h>
#endif

QT_BEGIN_NAMESPACE

template <typename T, size_t Prealloc = 32>
class QDuplicateTracker {
#ifdef __cpp_lib_memory_resource
    template <typename HT>
    struct QHasher {
        size_t storedSeed = QHashSeed::globalSeed();
        size_t operator()(const HT &t) const {
            return QHashPrivate::calculateHash(t, storedSeed);
        }
    };

    struct node_guesstimate { void *next; size_t hash; T value; };
    static constexpr size_t bufferSize(size_t N) {
        return N * sizeof(void*) // bucket list
                + N * sizeof(node_guesstimate); // nodes
    }

    char buffer[bufferSize(Prealloc)];
    std::pmr::monotonic_buffer_resource res{buffer, sizeof buffer};
    using Set = std::pmr::unordered_set<T, QHasher<T>>;
    Set set{Prealloc, &res};
#else
    class Set : public QSet<T> {
        qsizetype setSize = 0;
    public:
        explicit Set(qsizetype n) : QSet<T>{}
        { this->reserve(n); }

        auto insert(const T &e) {
            auto it = QSet<T>::insert(e);
            const auto n = this->size();
            return std::pair{it, std::exchange(setSize, n) != n};
        }

        auto insert(T &&e) {
            auto it = QSet<T>::insert(std::move(e));
            const auto n = this->size();
            return std::pair{it, std::exchange(setSize, n) != n};
        }
    };
    Set set{Prealloc};
#endif
    Q_DISABLE_COPY_MOVE(QDuplicateTracker)
public:
    static constexpr inline bool uses_pmr =
        #ifdef __cpp_lib_memory_resource
            true
        #else
            false
        #endif
            ;
    QDuplicateTracker() {} // don't `= default`, lest we value-initialize `buffer`
    explicit QDuplicateTracker(qsizetype n)
#ifdef __cpp_lib_memory_resource
        : set{size_t(n), &res}
#else
        : set{n}
#endif
    {}
    Q_DECL_DEPRECATED_X("Pass the capacity to reserve() to the ctor instead.")
    void reserve(qsizetype n) { set.reserve(n); }
    [[nodiscard]] bool hasSeen(const T &s)
    {
        return !set.insert(s).second;
    }
    [[nodiscard]] bool hasSeen(T &&s)
    {
        return !set.insert(std::move(s)).second;
    }
    // For when you want to know, but aren't *yet* sure you'll add it:
    [[nodiscard]] bool contains(const T &s) const
    {
        return set.find(s) != set.end(); // TODO C++20: can use set.contains()
    }

    template <typename C>
    void appendTo(C &c) const &
    {
        for (const auto &e : set)
            c.push_back(e);
    }

    template <typename C>
    void appendTo(C &c) &&
    {
        if constexpr (uses_pmr) {
            while (!set.empty())
                c.push_back(std::move(set.extract(set.begin()).value()));
        } else {
            return appendTo(c); // lvalue version
        }
    }

    void clear()
    {
#ifdef __cpp_lib_memory_resource
        // The birth defect of std::unordered_set is that both the nodes as
        // well as the bucket array are allocated from the same allocator, so
        // if we want to reclaim memory from the freed nodes, we also need to
        // reclaim the memory for the bucket array.

        set = Set();    // release all memory in `res` (clear() doesn't, and swap() is UB!)
        res.release();  // restore to initial state (buffer, sizeof buffer)
                        // m_b_r can't reuse buffers, anyway
        // now that `res` is reset to the initial state, also reset `set`:
        set = Set{Prealloc, &res};
#else
        set.clear();
#endif // __cpp_lib_memory_resource
    }

    using const_iterator = typename Set::const_iterator;
    const_iterator begin() const { return set.cbegin(); }
    const_iterator end() const { return set.cend(); }
    const_iterator cbegin() const { return begin(); }
    const_iterator cend() const { return end(); }
};

QT_END_NAMESPACE

#endif /* QDUPLICATETRACKER_P_H */
