// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QDATABUFFER_P_H
#define QDATABUFFER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of other Qt classes.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtGui/private/qtguiglobal_p.h>

#include "QtCore/qalloc.h"
#include "QtCore/qbytearray.h"
#include "QtCore/qtypeinfo.h"

#include <stdlib.h>

QT_BEGIN_NAMESPACE

template <typename Type> class QDataBuffer
{
    Q_DISABLE_COPY_MOVE(QDataBuffer)
public:
    explicit QDataBuffer(qsizetype res)
    {
        capacity = res;
        if (res) {
            QT_WARNING_PUSH
            QT_WARNING_DISABLE_GCC("-Walloc-size-larger-than=")
            buffer = (Type*) QtPrivate::fittedMalloc(0, &capacity, sizeof(Type));
            QT_WARNING_POP
            Q_CHECK_PTR(buffer);
        } else {
            buffer = nullptr;
        }
        siz = 0;
    }

    ~QDataBuffer()
    {
        static_assert(!QTypeInfo<Type>::isComplex);
        if (buffer)
            QtPrivate::sizedFree(buffer, capacity, sizeof(Type));
    }

    inline void reset() { siz = 0; }

    inline bool isEmpty() const { return siz==0; }

    qsizetype size() const { return siz; }
    inline Type *data() const { return buffer; }

    Type &at(qsizetype i) { Q_ASSERT(i >= 0 && i < siz); return buffer[i]; }
    const Type &at(qsizetype i) const { Q_ASSERT(i >= 0 && i < siz); return buffer[i]; }
    inline Type &last() { Q_ASSERT(!isEmpty()); return buffer[siz-1]; }
    inline const Type &last() const { Q_ASSERT(!isEmpty()); return buffer[siz-1]; }
    inline Type &first() { Q_ASSERT(!isEmpty()); return buffer[0]; }
    inline const Type &first() const { Q_ASSERT(!isEmpty()); return buffer[0]; }

    inline void add(const Type &t) {
        reserve(siz + 1);
        buffer[siz] = t;
        ++siz;
    }

    inline void pop_back() {
        Q_ASSERT(siz > 0);
        --siz;
    }

    void resize(qsizetype size) {
        reserve(size);
        siz = size;
    }

    void reserve(qsizetype size) {
        if (size > capacity) {
            if (capacity == 0)
                capacity = 1;
            while (capacity < size)
                capacity *= 2;
            auto ptr = QtPrivate::fittedRealloc(static_cast<void*>(buffer), 0, &capacity, sizeof(Type));
            Q_CHECK_PTR(ptr);
            buffer = static_cast<Type*>(ptr);
        }
    }

    void shrink(qsizetype size) {
        Q_ASSERT(capacity >= size);
        if (size) {
            capacity = size;
            const auto ptr = QtPrivate::fittedRealloc(static_cast<void*>(buffer), 0, &capacity, sizeof(Type));
            Q_CHECK_PTR(ptr);
            buffer = static_cast<Type*>(ptr);
            siz = std::min(siz, size);
        } else {
            QtPrivate::sizedFree(buffer, capacity, sizeof(Type));
            capacity = size;
            buffer = nullptr;
            siz = 0;
        }
    }

    inline void swap(QDataBuffer<Type> &other) {
        qSwap(capacity, other.capacity);
        qSwap(siz, other.siz);
        qSwap(buffer, other.buffer);
    }

    inline QDataBuffer &operator<<(const Type &t) { add(t); return *this; }

private:
    qsizetype capacity;
    qsizetype siz;
    Type *buffer;
};

QT_END_NAMESPACE

#endif // QDATABUFFER_P_H
