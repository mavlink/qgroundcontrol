// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QRECYCLEPOOL_P_H
#define QRECYCLEPOOL_P_H

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

#include <QtCore/q20memory.h>

QT_BEGIN_NAMESPACE

#define QRECYCLEPOOLCOOKIE 0x33218ADF

template<typename T, int Step>
class QRecyclePoolPrivate
{
public:
    QRecyclePoolPrivate()
    : recyclePoolHold(true), outstandingItems(0), cookie(QRECYCLEPOOLCOOKIE),
      currentPage(nullptr), nextAllocated(nullptr)
    {
    }

    bool recyclePoolHold;
    int outstandingItems;
    quint32 cookie;

    struct PoolType : public T {
        union {
            QRecyclePoolPrivate<T, Step> *pool;
            PoolType *nextAllocated;
        };
    };

    struct Page {
        Page *nextPage;
        unsigned int free;
        union {
            char array[Step * sizeof(PoolType)];
            qint64 q_for_alignment_1;
            double q_for_alignment_2;
        };
    };

    Page *currentPage;
    PoolType *nextAllocated;

    inline T *allocate();
    static inline void dispose(T *);
    inline void releaseIfPossible();
};

template<typename T, int Step = 1024>
class QRecyclePool
{
public:
    inline QRecyclePool();
    inline ~QRecyclePool();

    template<typename...Args>
    [[nodiscard]] inline T *New(Args&&...args);

    static inline void Delete(T *);

private:
    QRecyclePoolPrivate<T, Step> *d;
};

template<typename T, int Step>
QRecyclePool<T, Step>::QRecyclePool()
: d(new QRecyclePoolPrivate<T, Step>())
{
}

template<typename T, int Step>
QRecyclePool<T, Step>::~QRecyclePool()
{
    d->recyclePoolHold = false;
    d->releaseIfPossible();
}

template<typename T, int Step>
template<typename...Args>
T *QRecyclePool<T, Step>::New(Args&&...args)
{
    return q20::construct_at(d->allocate(), std::forward<Args>(args)...);
}

template<typename T, int Step>
void QRecyclePool<T, Step>::Delete(T *t)
{
    t->~T();
    QRecyclePoolPrivate<T, Step>::dispose(t);
}

template<typename T, int Step>
void QRecyclePoolPrivate<T, Step>::releaseIfPossible()
{
    if (recyclePoolHold || outstandingItems)
        return;

    Page *p = currentPage;
    while (p) {
        Page *n = p->nextPage;
        free(p);
        p = n;
    }

    delete this;
}

template<typename T, int Step>
T *QRecyclePoolPrivate<T, Step>::allocate()
{
    PoolType *rv = nullptr;
    if (nextAllocated) {
        rv = nextAllocated;
        nextAllocated = rv->nextAllocated;
    } else if (currentPage && currentPage->free) {
        rv = (PoolType *)(currentPage->array + (Step - currentPage->free) * sizeof(PoolType));
        currentPage->free--;
    } else {
        Page *p = (Page *)malloc(sizeof(Page));
        p->nextPage = currentPage;
        p->free = Step;
        currentPage = p;

        rv = (PoolType *)currentPage->array;
        currentPage->free--;
    }

    rv->pool = this;
    ++outstandingItems;
    return rv;
}

template<typename T, int Step>
void QRecyclePoolPrivate<T, Step>::dispose(T *t)
{
    PoolType *pt = static_cast<PoolType *>(t);
    Q_ASSERT(pt->pool && pt->pool->cookie == QRECYCLEPOOLCOOKIE);

    QRecyclePoolPrivate<T, Step> *This = pt->pool;
    pt->nextAllocated = This->nextAllocated;
    This->nextAllocated = pt;
    --This->outstandingItems;
    This->releaseIfPossible();
}

QT_END_NAMESPACE

#endif // QRECYCLEPOOL_P_H
