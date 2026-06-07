// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QPODVECTOR_P_H
#define QPODVECTOR_P_H

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

#include <QtCore/private/qglobal_p.h>
#include <QtCore/qtclasshelpermacros.h>

#include <type_traits>

QT_BEGIN_NAMESPACE

template<class T, int Increment>
class QPODVector
{
    static_assert(std::is_trivially_constructible_v<T>);
    static_assert(std::is_trivially_move_constructible_v<T>);
    static_assert(std::is_trivially_move_assignable_v<T>);
    static_assert(std::is_trivially_destructible_v<T>);

public:
    QPODVector()
    : m_count(0), m_capacity(0), m_data(nullptr) {}
    ~QPODVector() { if (m_data) ::free(m_data); }

    const T &at(int idx) const {
        return m_data[idx];
    }

    T &operator[](int idx) {
        return m_data[idx];
    }

    void clear() {
        m_count = 0;
    }

    void prepend(const T &v) {
        insert(0, v);
    }

    void append(const T &v) {
        insert(m_count, v);
    }

    void insert(int idx, const T &v) {
        if (m_count == m_capacity) {
            m_capacity += Increment;
            m_data = (T *)realloc(static_cast<void *>(m_data), m_capacity * sizeof(T));
        }
        int moveCount = m_count - idx;
        if (moveCount)
            ::memmove(static_cast<void *>(m_data + idx + 1), static_cast<const void *>(m_data + idx), moveCount * sizeof(T));
        m_count++;
        m_data[idx] = v;
    }

    void reserve(int count) {
        if (count >= m_capacity) {
            m_capacity = (count + (Increment-1)) & (0xFFFFFFFF - Increment + 1);
            m_data = (T *)realloc(static_cast<void *>(m_data), m_capacity * sizeof(T));
        }
    }

    void insertBlank(int idx, int count) {
        int newSize = m_count + count;
        reserve(newSize);
        int moveCount = m_count - idx;
        if (moveCount)
            ::memmove(static_cast<void *>(m_data + idx + count),  static_cast<const void *>(m_data + idx),
                      moveCount * sizeof(T));
        m_count = newSize;
    }

    void remove(int idx, int count = 1) {
        int moveCount = m_count - (idx + count);
        if (moveCount)
            ::memmove(static_cast<void *>(m_data + idx), static_cast<const void *>(m_data + idx + count),
                      moveCount * sizeof(T));
        m_count -= count;
    }

    void removeOne(const T &v) {
        int idx = 0;
        while (idx < m_count) {
            if (m_data[idx] == v) {
                remove(idx);
                return;
            }
            ++idx;
        }
    }

    int find(const T &v) {
        for (int idx = 0; idx < m_count; ++idx)
            if (m_data[idx] == v)
                return idx;
        return -1;
    }

    bool contains(const T &v) {
        return find(v) != -1;
    }

    int count() const {
        return m_count;
    }

    void copyAndClear(QPODVector<T,Increment> &other) {
        if (other.m_data) ::free(other.m_data);
        other.m_count = m_count;
        other.m_capacity = m_capacity;
        other.m_data = m_data;
        m_count = 0;
        m_capacity = 0;
        m_data = nullptr;
    }

    QPODVector<T,Increment> &operator<<(const T &v) { append(v); return *this; }
private:
    Q_DISABLE_COPY(QPODVector)
    int m_count;
    int m_capacity;
    T *m_data;
};

QT_END_NAMESPACE

#endif
