// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLREFCOUNT_P_H
#define QQMLREFCOUNT_P_H

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
#include <QtCore/qatomic.h>
#include <private/qv4global_p.h>

QT_BEGIN_NAMESPACE

template <typename T>
class QQmlRefCounted;

class QQmlRefCount
{
    Q_DISABLE_COPY_MOVE(QQmlRefCount)
public:
    inline QQmlRefCount();
    inline void addref() const;
    inline int count() const;

private:
    inline ~QQmlRefCount();
    template <typename T> friend class QQmlRefCounted;

private:
    mutable QAtomicInt refCount;
};

template <typename T>
class QQmlRefCounted : public QQmlRefCount
{
public:
    inline void release() const;
protected:
    inline ~QQmlRefCounted();
};

template<class T>
class QQmlRefPointer
{
public:
    enum Mode {
        AddRef,
        Adopt
    };
    Q_NODISCARD_CTOR inline QQmlRefPointer() noexcept;
    Q_NODISCARD_CTOR inline QQmlRefPointer(T *, Mode m = AddRef);
    Q_NODISCARD_CTOR inline QQmlRefPointer(const QQmlRefPointer &);
    Q_NODISCARD_CTOR inline QQmlRefPointer(QQmlRefPointer &&) noexcept;
    inline ~QQmlRefPointer();

    void swap(QQmlRefPointer &other) noexcept { qt_ptr_swap(o, other.o); }

    inline QQmlRefPointer<T> &operator=(const QQmlRefPointer<T> &o);
    inline QQmlRefPointer<T> &operator=(QQmlRefPointer<T> &&o) noexcept;

    inline bool isNull() const { return !o; }

    inline T* operator->() const { return o; }
    inline T& operator*() const { return *o; }
    explicit inline operator bool() const { return o != nullptr; }
    inline T* data() const { return o; }

    inline QQmlRefPointer<T> &adopt(T *);

    inline T* take() { T *res = o; o = nullptr; return res; }

    friend bool operator==(const QQmlRefPointer &a, const QQmlRefPointer &b) noexcept
    {
        return a.o == b.o;
    }

    friend bool operator!=(const QQmlRefPointer &a, const QQmlRefPointer &b) noexcept
    {
        return !(a == b);
    }

    friend size_t qHash(const QQmlRefPointer &v, size_t seed = 0) noexcept
    {
        return qHash(v.o, seed);
    }

    void reset(T *t = nullptr)
    {
        if (t == o)
            return;
        if (o)
            o->release();
        if (t)
            t->addref();
        o = t;
    }

private:
    T *o;
};

namespace QQml {
/*!
    \internal
    Creates a QQmlRefPointer which takes ownership of a newly constructed T.
    T must derive from QQmlRefCounted<T> (as we rely on an initial refcount of _1_).
    T will be constructed by forwarding \a args to its constructor.
 */
template <typename T, typename ...Args>
QQmlRefPointer<T> makeRefPointer(Args&&... args)
{
    static_assert(std::is_base_of_v<QQmlRefCount, T>);
    return QQmlRefPointer<T>(new T(std::forward<Args>(args)...), QQmlRefPointer<T>::Adopt);
}
}

template <typename T>
Q_DECLARE_TYPEINFO_BODY(QQmlRefPointer<T>, Q_RELOCATABLE_TYPE);

QQmlRefCount::QQmlRefCount()
: refCount(1)
{
}

QQmlRefCount::~QQmlRefCount()
{
    Q_ASSERT(refCount.loadRelaxed() == 0);
}

void QQmlRefCount::addref() const
{
    Q_ASSERT(refCount.loadRelaxed() > 0);
    refCount.ref();
}

template <typename T>
void QQmlRefCounted<T>::release() const
{
    static_assert(std::is_base_of_v<QQmlRefCounted, T>,
                  "QQmlRefCounted<T> must be a base of T (CRTP)");
    Q_ASSERT(refCount.loadRelaxed() > 0);
    if (!refCount.deref())
        delete static_cast<const T *>(this);
}

template <typename T>
QQmlRefCounted<T>::~QQmlRefCounted()
{
    static_assert(std::is_final_v<T> || std::has_virtual_destructor_v<T>,
                  "T must either be marked final or have a virtual dtor, "
                  "lest release() runs into UB.");
}

int QQmlRefCount::count() const
{
    return refCount.loadRelaxed();
}

template<class T>
QQmlRefPointer<T>::QQmlRefPointer() noexcept
: o(nullptr)
{
}

template<class T>
QQmlRefPointer<T>::QQmlRefPointer(T *o, Mode m)
: o(o)
{
    if (m == AddRef && o)
        o->addref();
}

template<class T>
QQmlRefPointer<T>::QQmlRefPointer(const QQmlRefPointer<T> &other)
: o(other.o)
{
    if (o) o->addref();
}

template <class T>
QQmlRefPointer<T>::QQmlRefPointer(QQmlRefPointer<T> &&other) noexcept
    : o(other.take())
{
}

template<class T>
QQmlRefPointer<T>::~QQmlRefPointer()
{
    if (o) o->release();
}

template<class T>
QQmlRefPointer<T> &QQmlRefPointer<T>::operator=(const QQmlRefPointer<T> &other)
{
    if (o == other.o)
        return *this;
    if (other.o)
        other.o->addref();
    if (o)
        o->release();
    o = other.o;
    return *this;
}

template <class T>
QQmlRefPointer<T> &QQmlRefPointer<T>::operator=(QQmlRefPointer<T> &&other) noexcept
{
    QQmlRefPointer<T> m(std::move(other));
    swap(m);
    return *this;
}

/*!
Takes ownership of \a other.  take() does *not* add a reference, as it assumes ownership
of the callers reference of other.
*/
template<class T>
QQmlRefPointer<T> &QQmlRefPointer<T>::adopt(T *other)
{
    if (o) o->release();
    o = other;
    return *this;
}

QT_END_NAMESPACE

#endif // QQMLREFCOUNT_P_H
