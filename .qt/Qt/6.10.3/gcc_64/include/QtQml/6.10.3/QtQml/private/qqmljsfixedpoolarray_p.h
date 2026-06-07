// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLJSFIXEDPOOLARRAY_P_H
#define QQMLJSFIXEDPOOLARRAY_P_H

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

#include <QtCore/qglobal.h>
#include <private/qqmljsmemorypool_p.h>

QT_BEGIN_NAMESPACE

namespace QQmlJS {

template <typename T>
class FixedPoolArray
{
    T *data;
    int count = 0;

public:
    FixedPoolArray()
        : data(nullptr)
    {}

    FixedPoolArray(MemoryPool *pool, int size)
    { allocate(pool, size); }

    void allocate(MemoryPool *pool, int size)
    {
        count = size;
        data = reinterpret_cast<T*>(pool->allocate(count * sizeof(T)));
    }

    void allocate(MemoryPool *pool, const QVector<T> &vector)
    {
        count = vector.size();
        data = reinterpret_cast<T*>(pool->allocate(count * sizeof(T)));

        if (QTypeInfo<T>::isComplex) {
            for (int i = 0; i < count; ++i)
                new (data + i) T(vector.at(i));
        } else if (count) {
            memcpy(data, static_cast<const void*>(vector.constData()), count * sizeof(T));
        }
    }

    template <typename Container>
    void allocate(MemoryPool *pool, const Container &container)
    {
        count = container.size();
        data = reinterpret_cast<T*>(pool->allocate(count * sizeof(T)));
        typename Container::ConstIterator it = container.constBegin();
        for (int i = 0; i < count; ++i)
            new (data + i) T(*it++);
    }

    int size() const
    { return count; }

    const T &at(int index) const {
        Q_ASSERT(index >= 0 && index < count);
        return data[index];
    }

    T &at(int index) {
        Q_ASSERT(index >= 0 && index < count);
        return data[index];
    }

    T &operator[](int index) {
        return at(index);
    }


    int indexOf(const T &value) const {
        for (int i = 0; i < count; ++i)
            if (data[i] == value)
                return i;
        return -1;
    }

    const T *begin() const { return data; }
    const T *end() const { return data + count; }

    T *begin() { return data; }
    T *end() { return data + count; }
};

} // namespace QQmlJS

QT_END_NAMESPACE

#endif // QQMLJSFIXEDPOOLARRAY_P_H
