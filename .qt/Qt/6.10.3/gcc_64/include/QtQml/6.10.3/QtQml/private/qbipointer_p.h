// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QBIPOINTER_P_H
#define QBIPOINTER_P_H

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

#include <QtCore/qhashfunctions.h>

QT_BEGIN_NAMESPACE

namespace QtPrivate {
template <typename T> struct QFlagPointerAlignment
{
    enum : size_t { Value = Q_ALIGNOF(T) };
};
template <> struct QFlagPointerAlignment<void>
{
    enum : size_t { Value = ~size_t(0) };
};
}

/*!
    \internal
    \class template<typename T1, typename T2> QBiPointer<T1, T2>

    \short QBiPointer can be thought of as a space-optimized std::variant<T1*, T2*>
    with a nicer API to check the active pointer. Its other main feature is that
    it only requires sizeof(void *) space.

    \note It can also store one additional flag for a user defined purpose.
 */
template<typename T, typename T2>
class QBiPointer {
public:
    Q_NODISCARD_CTOR constexpr QBiPointer() noexcept = default;
    ~QBiPointer() noexcept = default;
    Q_NODISCARD_CTOR QBiPointer(const QBiPointer &o) noexcept = default;
    Q_NODISCARD_CTOR QBiPointer(QBiPointer &&o) noexcept = default;
    QBiPointer<T, T2> &operator=(const QBiPointer<T, T2> &o) noexcept = default;
    QBiPointer<T, T2> &operator=(QBiPointer<T, T2> &&o) noexcept = default;

    void swap(QBiPointer &other) noexcept { std::swap(ptr_value, other.ptr_value); }

    Q_NODISCARD_CTOR inline QBiPointer(T *);
    Q_NODISCARD_CTOR inline QBiPointer(T2 *);

    inline bool isNull() const;
    inline bool isT1() const;
    inline bool isT2() const;

    inline bool flag() const;
    inline void setFlag();
    inline void clearFlag();
    inline void setFlagValue(bool);

    inline QBiPointer<T, T2> &operator=(T *);
    inline QBiPointer<T, T2> &operator=(T2 *);

    friend inline bool operator==(QBiPointer<T, T2> ptr1, QBiPointer<T, T2> ptr2)
    {
        if (ptr1.isNull() && ptr2.isNull())
            return true;
        if (ptr1.isT1() && ptr2.isT1())
            return ptr1.asT1() == ptr2.asT1();
        if (ptr1.isT2() && ptr2.isT2())
            return ptr1.asT2() == ptr2.asT2();
        return false;
    }
    friend inline bool operator!=(QBiPointer<T, T2> ptr1, QBiPointer<T, T2> ptr2)
    {
        return !(ptr1 == ptr2);
    }

    friend void swap(QBiPointer &lhs, QBiPointer &rhs) noexcept { lhs.swap(rhs); }

    inline T *asT1() const;
    inline T2 *asT2() const;

    friend size_t qHash(const QBiPointer<T, T2> &ptr, size_t seed = 0)
    {
        return qHash(ptr.isNull() ? quintptr(0) : ptr.ptr_value, seed);
    }

private:
    quintptr ptr_value = 0;

    static const quintptr FlagBit = 0x1;
    static const quintptr Flag2Bit = 0x2;
    static const quintptr FlagsMask = FlagBit | Flag2Bit;
};

template <typename...Ts> // can't use commas in macros
Q_DECLARE_TYPEINFO_BODY(QBiPointer<Ts...>, Q_PRIMITIVE_TYPE);

template<typename T, typename T2>
QBiPointer<T, T2>::QBiPointer(T *v)
: ptr_value(quintptr(v))
{
    Q_STATIC_ASSERT_X(QtPrivate::QFlagPointerAlignment<T>::Value >= 4,
                      "Type T does not have sufficient alignment");
    Q_ASSERT((quintptr(v) & FlagsMask) == 0);
}

template<typename T, typename T2>
QBiPointer<T, T2>::QBiPointer(T2 *v)
: ptr_value(quintptr(v) | Flag2Bit)
{
    Q_STATIC_ASSERT_X(QtPrivate::QFlagPointerAlignment<T2>::Value >= 4,
                      "Type T2 does not have sufficient alignment");
    Q_ASSERT((quintptr(v) & FlagsMask) == 0);
}

template<typename T, typename T2>
bool QBiPointer<T, T2>::isNull() const
{
    return 0 == (ptr_value & (~FlagsMask));
}

template<typename T, typename T2>
bool QBiPointer<T, T2>::isT1() const
{
    return !(ptr_value & Flag2Bit);
}

template<typename T, typename T2>
bool QBiPointer<T, T2>::isT2() const
{
    return ptr_value & Flag2Bit;
}

template<typename T, typename T2>
bool QBiPointer<T, T2>::flag() const
{
    return ptr_value & FlagBit;
}

template<typename T, typename T2>
void QBiPointer<T, T2>::setFlag()
{
    ptr_value |= FlagBit;
}

template<typename T, typename T2>
void QBiPointer<T, T2>::clearFlag()
{
    ptr_value &= ~FlagBit;
}

template<typename T, typename T2>
void QBiPointer<T, T2>::setFlagValue(bool v)
{
    if (v) setFlag();
    else clearFlag();
}

template<typename T, typename T2>
QBiPointer<T, T2> &QBiPointer<T, T2>::operator=(T *o)
{
    Q_ASSERT((quintptr(o) & FlagsMask) == 0);

    ptr_value = quintptr(o) | (ptr_value & FlagBit);
    return *this;
}

template<typename T, typename T2>
QBiPointer<T, T2> &QBiPointer<T, T2>::operator=(T2 *o)
{
    Q_ASSERT((quintptr(o) & FlagsMask) == 0);

    ptr_value = quintptr(o) | (ptr_value & FlagBit) | Flag2Bit;
    return *this;
}

template<typename T, typename T2>
T *QBiPointer<T, T2>::asT1() const
{
    Q_ASSERT(isT1());
    return (T *)(ptr_value & ~FlagsMask);
}

template<typename T, typename T2>
T2 *QBiPointer<T, T2>::asT2() const
{
    Q_ASSERT(isT2());
    return (T2 *)(ptr_value & ~FlagsMask);
}

QT_END_NAMESPACE

#endif // QBIPOINTER_P_H
