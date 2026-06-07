// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QSHAREDDATA_H
#define QSHAREDDATA_H

#include <QtCore/qglobal.h>
#include <QtCore/qatomic.h>
#include <QtCore/qcompare.h>
#include <QtCore/qhashfunctions.h>


QT_BEGIN_NAMESPACE


template <class T> class QSharedDataPointer;

class QSharedData
{
public:
    mutable QAtomicInt ref;

    QSharedData() noexcept : ref(0) { }
    QSharedData(const QSharedData &) noexcept : ref(0) { }

    // using the assignment operator would lead to corruption in the ref-counting
    QSharedData &operator=(const QSharedData &) = delete;
    ~QSharedData() = default;
};

struct QAdoptSharedDataTag { explicit constexpr QAdoptSharedDataTag() = default; };

template <typename T>
class QSharedDataPointer
{
public:
    typedef T Type;
    typedef T *pointer;

    void detach() { if (d && d->ref.loadRelaxed() != 1) detach_helper(); }
    T &operator*() { detach(); return *(d.get()); }
    const T &operator*() const { return *(d.get()); }
    T *operator->() { detach(); return d.get(); }
    const T *operator->() const noexcept { return d.get(); }
    operator T *() { detach(); return d.get(); }
    operator const T *() const noexcept { return d.get(); }
    T *data() { detach(); return d.get(); }
    T *get() { detach(); return d.get(); }
    const T *data() const noexcept { return d.get(); }
    const T *get() const noexcept { return d.get(); }
    const T *constData() const noexcept { return d.get(); }
    T *take() noexcept { return std::exchange(d, nullptr).get(); }

    Q_NODISCARD_CTOR
    QSharedDataPointer() noexcept : d(nullptr) { }
    ~QSharedDataPointer() { if (d && !d->ref.deref()) delete d.get(); }

    Q_NODISCARD_CTOR
    explicit QSharedDataPointer(T *data) noexcept : d(data)
    { if (d) d->ref.ref(); }
    Q_NODISCARD_CTOR
    QSharedDataPointer(T *data, QAdoptSharedDataTag) noexcept : d(data)
    {}
    Q_NODISCARD_CTOR
    QSharedDataPointer(const QSharedDataPointer &o) noexcept : d(o.d)
    { if (d) d->ref.ref(); }

    void reset(T *ptr = nullptr) noexcept
    {
        if (ptr != d.get()) {
            if (ptr)
                ptr->ref.ref();
            T *old = std::exchange(d, Qt::totally_ordered_wrapper(ptr)).get();
            if (old && !old->ref.deref())
                delete old;
        }
    }

    QSharedDataPointer &operator=(const QSharedDataPointer &o) noexcept
    {
        reset(o.d.get());
        return *this;
    }
    inline QSharedDataPointer &operator=(T *o) noexcept
    {
        reset(o);
        return *this;
    }
    Q_NODISCARD_CTOR
    QSharedDataPointer(QSharedDataPointer &&o) noexcept : d(std::exchange(o.d, nullptr)) {}
    QT_MOVE_ASSIGNMENT_OPERATOR_IMPL_VIA_MOVE_AND_SWAP(QSharedDataPointer)

    operator bool () const noexcept { return d != nullptr; }
    bool operator!() const noexcept { return d == nullptr; }

    void swap(QSharedDataPointer &other) noexcept
    { qt_ptr_swap(d, other.d); }

protected:
    T *clone();

private:
    friend bool comparesEqual(const QSharedDataPointer &lhs, const QSharedDataPointer &rhs) noexcept
    { return lhs.d == rhs.d; }
    friend Qt::strong_ordering
    compareThreeWay(const QSharedDataPointer &lhs, const QSharedDataPointer &rhs) noexcept
    { return Qt::compareThreeWay(lhs.d, rhs.d); }
    Q_DECLARE_STRONGLY_ORDERED(QSharedDataPointer)

    friend bool comparesEqual(const QSharedDataPointer &lhs, const T *rhs) noexcept
    { return lhs.d == rhs; }
    friend Qt::strong_ordering
    compareThreeWay(const QSharedDataPointer &lhs, const T *rhs) noexcept
    { return Qt::compareThreeWay(lhs.d, rhs); }
    Q_DECLARE_STRONGLY_ORDERED(QSharedDataPointer, T*)

    friend bool comparesEqual(const QSharedDataPointer &lhs, std::nullptr_t) noexcept
    { return lhs.d == nullptr; }
    friend Qt::strong_ordering
    compareThreeWay(const QSharedDataPointer &lhs, std::nullptr_t) noexcept
    { return Qt::compareThreeWay(lhs.d, nullptr); }
    Q_DECLARE_STRONGLY_ORDERED(QSharedDataPointer, std::nullptr_t)

    void detach_helper();

    Qt::totally_ordered_wrapper<T *> d;
};

template <typename T>
class QExplicitlySharedDataPointer
{
public:
    typedef T Type;
    typedef T *pointer;

    T &operator*() const { return *(d.get()); }
    T *operator->() noexcept { return d.get(); }
    T *operator->() const noexcept { return d.get(); }
    explicit operator T *() { return d.get(); }
    explicit operator const T *() const noexcept { return d.get(); }
    T *data() const noexcept { return d.get(); }
    T *get() const noexcept { return d.get(); }
    const T *constData() const noexcept { return d.get(); }
    T *take() noexcept { return std::exchange(d, nullptr).get(); }

    void detach() { if (d && d->ref.loadRelaxed() != 1) detach_helper(); }

    Q_NODISCARD_CTOR
    QExplicitlySharedDataPointer() noexcept : d(nullptr) { }
    ~QExplicitlySharedDataPointer() { if (d && !d->ref.deref()) delete d.get(); }

    Q_NODISCARD_CTOR
    explicit QExplicitlySharedDataPointer(T *data) noexcept : d(data)
    { if (d) d->ref.ref(); }
    Q_NODISCARD_CTOR
    QExplicitlySharedDataPointer(T *data, QAdoptSharedDataTag) noexcept : d(data)
    {}
    Q_NODISCARD_CTOR
    QExplicitlySharedDataPointer(const QExplicitlySharedDataPointer &o) noexcept : d(o.d)
    { if (d) d->ref.ref(); }

    template<typename X>
    Q_NODISCARD_CTOR
    QExplicitlySharedDataPointer(const QExplicitlySharedDataPointer<X> &o) noexcept
#ifdef QT_ENABLE_QEXPLICITLYSHAREDDATAPOINTER_STATICCAST
#error This macro has been removed in Qt 6.9.
#endif
        : d(o.data())
    { if (d) d->ref.ref(); }

    void reset(T *ptr = nullptr) noexcept
    {
        if (ptr != d) {
            if (ptr)
                ptr->ref.ref();
            T *old = std::exchange(d, Qt::totally_ordered_wrapper(ptr)).get();
            if (old && !old->ref.deref())
                delete old;
        }
    }

    QExplicitlySharedDataPointer &operator=(const QExplicitlySharedDataPointer &o) noexcept
    {
        reset(o.d.get());
        return *this;
    }
    QExplicitlySharedDataPointer &operator=(T *o) noexcept
    {
        reset(o);
        return *this;
    }
    Q_NODISCARD_CTOR
    QExplicitlySharedDataPointer(QExplicitlySharedDataPointer &&o) noexcept : d(std::exchange(o.d, nullptr)) {}
    QT_MOVE_ASSIGNMENT_OPERATOR_IMPL_VIA_MOVE_AND_SWAP(QExplicitlySharedDataPointer)

    operator bool () const noexcept { return d != nullptr; }
    bool operator!() const noexcept { return d == nullptr; }

    void swap(QExplicitlySharedDataPointer &other) noexcept
    { qt_ptr_swap(d, other.d); }

protected:
    T *clone();

private:
    friend bool comparesEqual(const QExplicitlySharedDataPointer &lhs,
                              const QExplicitlySharedDataPointer &rhs) noexcept
    { return lhs.d == rhs.d; }
    friend Qt::strong_ordering
    compareThreeWay(const QExplicitlySharedDataPointer &lhs,
                    const QExplicitlySharedDataPointer &rhs) noexcept
    { return Qt::compareThreeWay(lhs.d, rhs.d); }
    Q_DECLARE_STRONGLY_ORDERED(QExplicitlySharedDataPointer)

    friend bool comparesEqual(const QExplicitlySharedDataPointer &lhs, const T *rhs) noexcept
    { return lhs.d == rhs; }
    friend Qt::strong_ordering
    compareThreeWay(const QExplicitlySharedDataPointer &lhs, const T *rhs) noexcept
    { return Qt::compareThreeWay(lhs.d, rhs); }
    Q_DECLARE_STRONGLY_ORDERED(QExplicitlySharedDataPointer, const T*)

    friend bool comparesEqual(const QExplicitlySharedDataPointer &lhs, std::nullptr_t) noexcept
    { return lhs.d == nullptr; }
    friend Qt::strong_ordering
    compareThreeWay(const QExplicitlySharedDataPointer &lhs, std::nullptr_t) noexcept
    { return Qt::compareThreeWay(lhs.d, nullptr); }
    Q_DECLARE_STRONGLY_ORDERED(QExplicitlySharedDataPointer, std::nullptr_t)

    void detach_helper();

    Qt::totally_ordered_wrapper<T *> d;
};

// Declared here and as Q_OUTOFLINE_TEMPLATE to work-around MSVC bug causing missing symbols at link time.
template <typename T>
Q_INLINE_TEMPLATE T *QSharedDataPointer<T>::clone()
{
    return new T(*d);
}

template <typename T>
Q_OUTOFLINE_TEMPLATE void QSharedDataPointer<T>::detach_helper()
{
    T *x = clone();
    x->ref.ref();
    if (!d.get()->ref.deref())
        delete d.get();
    d.reset(x);
}

template <typename T>
Q_INLINE_TEMPLATE T *QExplicitlySharedDataPointer<T>::clone()
{
    return new T(*d.get());
}

template <typename T>
Q_OUTOFLINE_TEMPLATE void QExplicitlySharedDataPointer<T>::detach_helper()
{
    T *x = clone();
    x->ref.ref();
    if (!d->ref.deref())
        delete d.get();
    d.reset(x);
}

template <typename T>
void swap(QSharedDataPointer<T> &p1, QSharedDataPointer<T> &p2) noexcept
{ p1.swap(p2); }

template <typename T>
void swap(QExplicitlySharedDataPointer<T> &p1, QExplicitlySharedDataPointer<T> &p2) noexcept
{ p1.swap(p2); }

template <typename T>
size_t qHash(const QSharedDataPointer<T> &ptr, size_t seed = 0) noexcept
{
    return qHash(ptr.data(), seed);
}
template <typename T>
size_t qHash(const QExplicitlySharedDataPointer<T> &ptr, size_t seed = 0) noexcept
{
    return qHash(ptr.data(), seed);
}

template<typename T> Q_DECLARE_TYPEINFO_BODY(QSharedDataPointer<T>, Q_RELOCATABLE_TYPE);
template<typename T> Q_DECLARE_TYPEINFO_BODY(QExplicitlySharedDataPointer<T>, Q_RELOCATABLE_TYPE);

#define QT_DECLARE_QSDP_SPECIALIZATION_DTOR(Class) \
    template<> QSharedDataPointer<Class>::~QSharedDataPointer();

#define QT_DECLARE_QSDP_SPECIALIZATION_DTOR_WITH_EXPORT(Class, ExportMacro) \
    template<> ExportMacro QSharedDataPointer<Class>::~QSharedDataPointer();

#define QT_DEFINE_QSDP_SPECIALIZATION_DTOR(Class) \
    template<> QSharedDataPointer<Class>::~QSharedDataPointer() \
    { \
        if (d && !d->ref.deref()) \
            delete d.get(); \
    }

#define QT_DECLARE_QESDP_SPECIALIZATION_DTOR(Class) \
    template<> QExplicitlySharedDataPointer<Class>::~QExplicitlySharedDataPointer();

#define QT_DECLARE_QESDP_SPECIALIZATION_DTOR_WITH_EXPORT(Class, ExportMacro) \
    template<> ExportMacro QExplicitlySharedDataPointer<Class>::~QExplicitlySharedDataPointer();

#define QT_DEFINE_QESDP_SPECIALIZATION_DTOR(Class) \
    template<> QExplicitlySharedDataPointer<Class>::~QExplicitlySharedDataPointer() \
    { \
        if (d && !d->ref.deref()) \
            delete d.get(); \
    }

QT_END_NAMESPACE

#endif // QSHAREDDATA_H
