// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QPOINTER_H
#define QPOINTER_H

#include <QtCore/qcompare.h>
#include <QtCore/qsharedpointer.h>
#include <QtCore/qtypeinfo.h>

#ifndef QT_NO_QOBJECT

QT_BEGIN_NAMESPACE

class QVariant;

template <class T>
class QPointer
{
    static_assert(!std::is_pointer<T>::value, "QPointer's template type must not be a pointer type");

    template <typename X>
    using if_convertible = std::enable_if_t<std::is_convertible_v<X*, T*>, bool>;
    template <typename X>
    friend class QPointer;

    using QObjectType =
        typename std::conditional<std::is_const<T>::value, const QObject, QObject>::type;
    QWeakPointer<QObjectType> wp;
public:
    Q_NODISCARD_CTOR
    QPointer() noexcept = default;
    Q_NODISCARD_CTOR
    constexpr QPointer(std::nullptr_t) noexcept : QPointer{} {}
    Q_WEAK_OVERLOAD
    Q_NODISCARD_CTOR
    inline QPointer(T *p) : wp(p, true) { }
    // compiler-generated copy/move ctor/assignment operators are fine!
    // compiler-generated dtor is fine!

    template <typename X, if_convertible<X> = true>
    Q_NODISCARD_CTOR
    QPointer(QPointer<X> &&other) noexcept
        : wp(std::exchange(other.wp, nullptr).internalData(), true) {}
    template <typename X, if_convertible<X> = true>
    Q_NODISCARD_CTOR
    QPointer(const QPointer<X> &other) noexcept
        : wp(other.wp.internalData(), true) {}

    template <typename X, if_convertible<X> = true>
    QPointer &operator=(const QPointer<X> &other) noexcept
    {
        QPointer(other).swap(*this);
        return *this;
    }

    template <typename X, if_convertible<X> = true>
    QPointer &operator=(QPointer<X> &&other) noexcept
    {
        QPointer(std::move(other)).swap(*this);
        return *this;
    }

#ifdef Q_QDOC
    // Stop qdoc from complaining about missing function
    ~QPointer();
#endif

    inline void swap(QPointer &other) noexcept { wp.swap(other.wp); }

    inline QPointer<T> &operator=(T* p)
    { wp.assign(static_cast<QObjectType*>(p)); return *this; }

    T* data() const noexcept
    { return static_cast<T*>(wp.internalData()); }
    T* get() const noexcept
    { return data(); }
    T* operator->() const noexcept
    { return data(); }
    T& operator*() const noexcept
    { return *data(); }
    operator T*() const noexcept
    { return data(); }

    bool isNull() const noexcept
    { return wp.isNull(); }
    explicit operator bool() const noexcept { return !isNull(); }

    void clear() noexcept
    { wp.clear(); }

    friend void swap(QPointer &lhs, QPointer &rhs) noexcept
    { lhs.swap(rhs); }

private:
    template <typename X>
    friend bool comparesEqual(const QPointer &lhs, const QPointer<X> &rhs) noexcept
    { return lhs.data() == rhs.data(); }
    QT_DECLARE_EQUALITY_OPERATORS_HELPER(QPointer, QPointer<X>, /* non-constexpr */,
                                         noexcept(true), template <typename X>)

    template <typename X>
    friend bool comparesEqual(const QPointer &lhs, X *rhs) noexcept
    { return lhs.data() == rhs; }
    Q_DECLARE_EQUALITY_COMPARABLE(QPointer, X*, template <typename X>)

    friend bool comparesEqual(const QPointer &lhs, std::nullptr_t) noexcept
    { return lhs.isNull(); }
    Q_DECLARE_EQUALITY_COMPARABLE(QPointer, std::nullptr_t)
};
template <class T> Q_DECLARE_TYPEINFO_BODY(QPointer<T>, Q_RELOCATABLE_TYPE);

template<typename T>
QPointer<T>
qPointerFromVariant(const QVariant &variant)
{
    const auto wp = QtSharedPointer::weakPointerFromVariant_internal(variant);
    return QPointer<T>{qobject_cast<T*>(QtPrivate::EnableInternalData::internalData(wp))};
}

QT_END_NAMESPACE

#endif // QT_NO_QOBJECT

#endif // QPOINTER_H
