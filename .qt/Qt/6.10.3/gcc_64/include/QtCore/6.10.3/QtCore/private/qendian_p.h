// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QENDIAN_P_H
#define QENDIAN_P_H

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

#include <QtCore/qendian.h>
#include <QtCore/private/qglobal_p.h>

QT_BEGIN_NAMESPACE

enum class QSpecialIntegerBitfieldInitializer {};
constexpr inline QSpecialIntegerBitfieldInitializer QSpecialIntegerBitfieldZero{};

template<class S>
class QSpecialIntegerStorage
{
public:
    using UnsignedStorageType = std::make_unsigned_t<typename S::StorageType>;

    constexpr QSpecialIntegerStorage() = default;
    constexpr QSpecialIntegerStorage(QSpecialIntegerBitfieldInitializer) : val(0) {}
    constexpr QSpecialIntegerStorage(UnsignedStorageType initial) : val(initial) {}

    UnsignedStorageType val;
};

template<class S, int pos, int width, class T = typename S::StorageType>
class QSpecialIntegerAccessor;

template<class S, int pos, int width, class T = typename S::StorageType>
class QSpecialIntegerConstAccessor
{
    Q_DISABLE_COPY_MOVE(QSpecialIntegerConstAccessor)
public:
    using Storage = const QSpecialIntegerStorage<S>;
    using Type = T;
    using UnsignedType = std::make_unsigned_t<T>;

    operator Type() const noexcept
    {
        if constexpr (std::is_signed_v<Type>) {
            UnsignedType i = S::fromSpecial(m_storage->val);
            i <<= (sizeof(Type) * 8) - width - pos;
            Type t = Type(i);
            t >>= (sizeof(Type) * 8) - width;
            return t;
        }
        return (S::fromSpecial(m_storage->val) & mask()) >> pos;
    }

    bool operator!() const noexcept { return !(m_storage->val & S::toSpecial(mask())); }

    static constexpr UnsignedType mask() noexcept
    {
        if constexpr (width == sizeof(UnsignedType) * 8) {
            static_assert(pos == 0);
            return ~UnsignedType(0);
        } else {
            return ((UnsignedType(1) << width) - 1) << pos;
        }
    }

private:
    template<class Storage, typename... Accessors>
    friend class QSpecialIntegerBitfieldUnion;
    friend class QSpecialIntegerAccessor<S, pos, width, T>;

    explicit QSpecialIntegerConstAccessor(Storage *storage) : m_storage(storage) {}

    friend bool operator==(const QSpecialIntegerConstAccessor<S, pos, width, T> &i,
                           const QSpecialIntegerConstAccessor<S, pos, width, T> &j) noexcept
    {
        return ((i.m_storage->val ^ j.m_storage->val) & S::toSpecial(mask())) == 0;
    }

    friend bool operator!=(const QSpecialIntegerConstAccessor<S, pos, width, T> &i,
                           const QSpecialIntegerConstAccessor<S, pos, width, T> &j) noexcept
    {
        return ((i.m_storage->val ^ j.m_storage->val) & S::toSpecial(mask())) != 0;
    }

    Storage *m_storage;
};

template<class S, int pos, int width, class T>
class QSpecialIntegerAccessor
{
    Q_DISABLE_COPY_MOVE(QSpecialIntegerAccessor)
public:
    using Const = QSpecialIntegerConstAccessor<S, pos, width, T>;
    using Storage = QSpecialIntegerStorage<S>;
    using Type = T;
    using UnsignedType = std::make_unsigned_t<T>;

    QSpecialIntegerAccessor &operator=(Type t)
    {
        UnsignedType i = S::fromSpecial(m_storage->val);
        i &= ~Const::mask();
        i |= (UnsignedType(t) << pos) & Const::mask();
        m_storage->val = S::toSpecial(i);
        return *this;
    }

    operator Const() { return Const(m_storage); }

private:
    template<class Storage, typename... Accessors>
    friend class QSpecialIntegerBitfieldUnion;

    explicit QSpecialIntegerAccessor(Storage *storage) : m_storage(storage) {}

    Storage *m_storage;
};

template<class S, typename... Accessors>
class QSpecialIntegerBitfieldUnion
{
public:
    constexpr QSpecialIntegerBitfieldUnion() = default;
    constexpr QSpecialIntegerBitfieldUnion(QSpecialIntegerBitfieldInitializer initial)
        : storage(initial)
    {}

    constexpr QSpecialIntegerBitfieldUnion(
            typename QSpecialIntegerStorage<S>::UnsignedStorageType initial)
        : storage(initial)
    {}

    template<typename A>
    void set(typename A::Type value)
    {
        member<A>() = value;
    }

    template<typename A>
    typename A::Type get() const
    {
        return member<A>();
    }

    typename QSpecialIntegerStorage<S>::UnsignedStorageType data() const
    {
        return storage.val;
    }

private:
    template<typename A>
    static constexpr bool isAccessor = std::disjunction_v<std::is_same<A, Accessors>...>;

    template<typename A>
    A member()
    {
        static_assert(isAccessor<A>);
        return A(&storage);
    }

    template<typename A>
    typename A::Const member() const
    {
        static_assert(isAccessor<A>);
        return typename A::Const(&storage);
    }

    QSpecialIntegerStorage<S> storage;
};

template<typename T, typename... Accessors>
using QLEIntegerBitfieldUnion
        = QSpecialIntegerBitfieldUnion<QLittleEndianStorageType<T>, Accessors...>;

template<typename T, typename... Accessors>
using QBEIntegerBitfieldUnion
        = QSpecialIntegerBitfieldUnion<QBigEndianStorageType<T>, Accessors...>;

template<typename... Accessors>
using qint32_le_bitfield_union = QLEIntegerBitfieldUnion<int, Accessors...>;
template<typename... Accessors>
using quint32_le_bitfield_union = QLEIntegerBitfieldUnion<uint, Accessors...>;
template<typename... Accessors>
using qint32_be_bitfield_union = QBEIntegerBitfieldUnion<int, Accessors...>;
template<typename... Accessors>
using quint32_be_bitfield_union = QBEIntegerBitfieldUnion<uint, Accessors...>;

template<int pos, int width, typename T = int>
using qint32_le_bitfield_member
        = QSpecialIntegerAccessor<QLittleEndianStorageType<int>, pos, width, T>;
template<int pos, int width, typename T = uint>
using quint32_le_bitfield_member
        = QSpecialIntegerAccessor<QLittleEndianStorageType<uint>, pos, width, T>;
template<int pos, int width, typename T = int>
using qint32_be_bitfield_member
        = QSpecialIntegerAccessor<QBigEndianStorageType<int>, pos, width, T>;
template<int pos, int width, typename T = uint>
using quint32_be_bitfield_member
        = QSpecialIntegerAccessor<QBigEndianStorageType<uint>, pos, width, T>;

QT_END_NAMESPACE

#endif // QENDIAN_P_H
