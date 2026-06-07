// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:data-parser

#ifndef QVARIANT_P_H
#define QVARIANT_P_H

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

#include "qvariant.h"

QT_BEGIN_NAMESPACE

inline auto customConstructSharedImpl(size_t size, size_t align)
{
    struct Deleter {
        void operator()(QVariant::PrivateShared *p) const
        { QVariant::PrivateShared::free(p); }
    };

    // this is exception-safe
    std::unique_ptr<QVariant::PrivateShared, Deleter> ptr;
    ptr.reset(QVariant::PrivateShared::create(size, align));
    return ptr;
}

template <typename F> static QVariant::PrivateShared *
customConstructShared(size_t size, size_t align, F &&construct)
{
    auto ptr = customConstructSharedImpl(size, align);
    construct(ptr->data());
    return ptr.release();
}

inline int QVariant::PrivateShared::computeOffset(PrivateShared *ps, size_t align)
{
    return int(((quintptr(ps) + sizeof(PrivateShared) + align - 1) & ~(align - 1)) - quintptr(ps));
}

inline size_t QVariant::PrivateShared::computeAllocationSize(size_t size, size_t align)
{
    size += sizeof(PrivateShared);
    if (align > sizeof(PrivateShared)) {
        // The alignment is larger than the alignment we can guarantee for the pointer
        // directly following PrivateShared, so we need to allocate some additional
        // memory to be able to fit the object into the available memory with suitable
        // alignment.
        size += align - sizeof(PrivateShared);
    }
    return size;
}

inline QVariant::PrivateShared *QVariant::PrivateShared::create(size_t size, size_t align)
{
    size = computeAllocationSize(size, align);
    void *data = operator new(size);
    auto *ps = new (data) QVariant::PrivateShared();
    ps->offset = computeOffset(ps, align);
    return ps;
}

inline void QVariant::PrivateShared::free(PrivateShared *p)
{
    p->~PrivateShared();
    operator delete(p);
}

inline QVariant::Private::Private(const QtPrivate::QMetaTypeInterface *iface) noexcept
    : is_shared(false), is_null(false), packedType(quintptr(iface) >> 2)
{
    Q_ASSERT((quintptr(iface) & 0x3) == 0);
}

template <typename T> inline
QVariant::Private::Private(std::piecewise_construct_t, const T &t)
    : is_shared(!CanUseInternalSpace<T>), is_null(std::is_same_v<T, std::nullptr_t>)
{
    // confirm noexceptness
    static constexpr bool isNothrowQVariantConstructible = noexcept(QVariant(t));
    static constexpr bool isNothrowCopyConstructible = std::is_nothrow_copy_constructible_v<T>;
    static constexpr bool isNothrowCopyAssignable = std::is_nothrow_copy_assignable_v<T>;

    const QtPrivate::QMetaTypeInterface *iface = QtPrivate::qMetaTypeInterfaceForType<T>();
    Q_ASSERT((quintptr(iface) & 0x3) == 0);
    packedType = quintptr(iface) >> 2;

    if constexpr (CanUseInternalSpace<T>) {
        static_assert(isNothrowQVariantConstructible == isNothrowCopyConstructible);
        static_assert(isNothrowQVariantConstructible == isNothrowCopyAssignable);
        new (data.data) T(t);
    } else {
        static_assert(!isNothrowQVariantConstructible); // we allocate memory, even if T doesn't
        data.shared = customConstructShared(sizeof(T), alignof(T), [&t](void *where) {
            new (where) T(t);
        });
    }
}

QT_END_NAMESPACE

#endif // QVARIANT_P_H
