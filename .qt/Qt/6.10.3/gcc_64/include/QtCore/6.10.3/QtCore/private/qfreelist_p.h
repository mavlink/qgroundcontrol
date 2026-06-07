// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QFREELIST_P_H
#define QFREELIST_P_H

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
#include <QtCore/qatomic.h>

QT_BEGIN_NAMESPACE

/*! \internal

    Element in a QFreeList. ConstReferenceType and ReferenceType are used as
    the return values for QFreeList::at() and QFreeList::operator[](). Contains
    the real data storage (_t) and the id of the next free element (next).

    Note: the t() functions should be used to access the data, not _t.
*/
template <typename T>
struct QFreeListElement
{
    typedef const T &ConstReferenceType;
    typedef T &ReferenceType;

    T _t;
    QAtomicInt next;

    inline ConstReferenceType t() const { return _t; }
    inline ReferenceType t() { return _t; }
};

/*! \internal

    Element in a QFreeList without a payload. ConstReferenceType and
    ReferenceType are void, the t() functions return void and are empty.
*/
template <>
struct QFreeListElement<void>
{
    typedef void ConstReferenceType;
    typedef void ReferenceType;

    QAtomicInt next;

    inline void t() const { }
    inline void t() { }
};

/*! \internal

    Defines default constants used by QFreeList:

    - The initial value returned by QFreeList::next() is zero.

    - QFreeList allows for up to 16777216 elements in QFreeList and uses the top
      8 bits to store a serial counter for ABA protection.

    - QFreeList will make a maximum of 4 allocations (blocks), with each
      successive block larger than the previous.

    - Sizes static int[] array to define the size of each block.

    It is possible to define your own constants struct/class and give this to
    QFreeList to customize/tune the behavior.
*/
struct Q_AUTOTEST_EXPORT QFreeListDefaultConstants
{
    // used by QFreeList, make sure to define all of when customizing
    enum {
        InitialNextValue = 0,
        IndexMask = 0x00ffffff,
        SerialMask = ~IndexMask & ~0x80000000,
        SerialCounter = IndexMask + 1,
        MaxIndex = IndexMask,
        BlockCount = 4
    };

    static const int Sizes[BlockCount];
};

/*! \internal

    This is a generic implementation of a lock-free free list. Use next() to
    get the next free entry in the list, and release(id) when done with the id.

    This version is templated and allows having a payload of type T which can
    be accessed using the id returned by next(). The payload is allocated and
    deallocated automatically by the free list, but *NOT* when calling
    next()/release(). Initialization should be done by code needing it after
    next() returns. Likewise, cleanup() should happen before calling release().
    It is possible to have use 'void' as the payload type, in which case the
    free list only contains indexes to the next free entry.

    The ConstantsType type defaults to QFreeListDefaultConstants above. You can
    define your custom ConstantsType, see above for details on what needs to be
    available.
*/
template <typename T, typename ConstantsType = QFreeListDefaultConstants>
class QFreeList
{
    typedef T ValueType;
    typedef QFreeListElement<T> ElementType;
    typedef typename ElementType::ConstReferenceType ConstReferenceType;
    typedef typename ElementType::ReferenceType ReferenceType;

    // return which block the index \a x falls in, and modify \a x to be the index into that block
    static inline int blockfor(int &x)
    {
        x = x & ConstantsType::IndexMask;
        for (int i = 0; i < ConstantsType::BlockCount; ++i) {
            int size = ConstantsType::Sizes[i];
            if (x < size)
                return i;
            x -= size;
        }
        Q_UNREACHABLE_RETURN(-1);
    }

    // allocate a block of the given \a size, initialized starting with the given \a offset
    static inline ElementType *allocate(int offset, int size)
    {
        // qDebug("QFreeList: allocating %d elements (%ld bytes) with offset %d", size, size * sizeof(ElementType), offset);
        ElementType *v = new ElementType[size];
        for (int i = 0; i < size; ++i)
            v[i].next.storeRelaxed(offset + i + 1);
        return v;
    }

    // take the current serial number from \a o, increment it, and store it in \a n
    static inline int incrementserial(int o, int n)
    {
        return int((uint(n) & ConstantsType::IndexMask) | ((uint(o) + ConstantsType::SerialCounter) & ConstantsType::SerialMask));
    }

    // the blocks
    QAtomicPointer<ElementType> _v[ConstantsType::BlockCount];
    // the next free id
    QAtomicInt _next;

    // QFreeList is not copyable
    Q_DISABLE_COPY_MOVE(QFreeList)

public:
    constexpr inline QFreeList();
    inline ~QFreeList();

    // returns the payload for the given index \a x
    inline ConstReferenceType at(int x) const;
    inline ReferenceType operator[](int x);

    /*
        Return the next free id. Use this id to access the payload (see above).
        Call release(id) when done using the id.
    */
    inline int next();
    inline void release(int id);
};

template <typename T, typename ConstantsType>
constexpr inline QFreeList<T, ConstantsType>::QFreeList()
    :
      _v{}, // uniform initialization required
      _next(ConstantsType::InitialNextValue)
{ }

template <typename T, typename ConstantsType>
inline QFreeList<T, ConstantsType>::~QFreeList()
{
    for (int i = 0; i < ConstantsType::BlockCount; ++i)
        delete [] _v[i].loadAcquire();
}

template <typename T, typename ConstantsType>
inline typename QFreeList<T, ConstantsType>::ConstReferenceType QFreeList<T, ConstantsType>::at(int x) const
{
    const int block = blockfor(x);
    return (_v[block].loadRelaxed())[x].t();
}

template <typename T, typename ConstantsType>
inline typename QFreeList<T, ConstantsType>::ReferenceType QFreeList<T, ConstantsType>::operator[](int x)
{
    const int block = blockfor(x);
    return (_v[block].loadRelaxed())[x].t();
}

template <typename T, typename ConstantsType>
inline int QFreeList<T, ConstantsType>::next()
{
    int newid;
    int id = _next.loadAcquire();
    do {
        int at = id & ConstantsType::IndexMask;
        const int block = blockfor(at);
        ElementType *v = _v[block].loadAcquire();

        if (!v) {
            ElementType* const alloced = allocate((id & ConstantsType::IndexMask) - at,
                                                  ConstantsType::Sizes[block]);
            if (_v[block].testAndSetOrdered(nullptr, alloced, v)) {
                v = alloced;
            } else {
                // race with another thread lost
                delete[] alloced;
                Q_ASSERT(v != nullptr);
            }
        }

        newid = v[at].next.loadRelaxed() | (id & ~ConstantsType::IndexMask);
    } while (!_next.testAndSetOrdered(id, newid, id));
    // qDebug("QFreeList::next(): returning %d (_next now %d, serial %d)",
    //        id & ConstantsType::IndexMask,
    //        newid & ConstantsType::IndexMask,
    //        (newid & ~ConstantsType::IndexMask) >> 24);
    return id;
}

template <typename T, typename ConstantsType>
inline void QFreeList<T, ConstantsType>::release(int id)
{
    int at = id & ConstantsType::IndexMask;
    const int block = blockfor(at);
    ElementType *v = _v[block].loadAcquire();

    int x, newid;
    do {
        x = _next.loadAcquire();
        v[at].next.storeRelaxed(x & ConstantsType::IndexMask);

        newid = incrementserial(x, id);
    } while (!_next.testAndSetRelease(x, newid));
    // qDebug("QFreeList::release(%d): _next now %d (was %d), serial %d",
    //        id & ConstantsType::IndexMask,
    //        newid & ConstantsType::IndexMask,
    //        x & ConstantsType::IndexMask,
    //        (newid & ~ConstantsType::IndexMask) >> 24);
}

QT_END_NAMESPACE

#endif // QFREELIST_P_H
