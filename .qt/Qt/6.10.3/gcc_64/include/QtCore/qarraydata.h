// Copyright (C) 2020 The Qt Company Ltd.
// Copyright (C) 2019 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QARRAYDATA_H
#define QARRAYDATA_H

#include <QtCore/qpair.h>
#include <QtCore/qatomic.h>
#include <QtCore/qflags.h>
#include <QtCore/qcontainerfwd.h>
#include <string.h>

QT_BEGIN_NAMESPACE

#if __has_cpp_attribute(gnu::malloc)
#  define Q_DECL_MALLOCLIKE [[nodiscard, gnu::malloc]]
#else
#  define Q_DECL_MALLOCLIKE [[nodiscard]]
#endif

template <class T> struct QTypedArrayData;

struct QArrayData
{
    enum AllocationOption {
        Grow,
        KeepSize
    };

    enum GrowthPosition {
        GrowsAtEnd,
        GrowsAtBeginning
    };

   enum ArrayOption {
        ArrayOptionDefault = 0,
        CapacityReserved     = 0x1  //!< the capacity was reserved by the user, try to keep it
    };
    Q_DECLARE_FLAGS(ArrayOptions, ArrayOption)

    QBasicAtomicInt ref_;
    ArrayOptions flags;
    qsizetype alloc;

    qsizetype allocatedCapacity() noexcept
    {
        return alloc;
    }

    qsizetype constAllocatedCapacity() const noexcept
    {
        return alloc;
    }

    /// Returns true if sharing took place
    bool ref() noexcept
    {
        ref_.ref();
        return true;
    }

    /// Returns false if deallocation is necessary
    bool deref() noexcept
    {
        return ref_.deref();
    }

    bool isShared() const noexcept
    {
        return ref_.loadRelaxed() != 1;
    }

    // Returns true if a detach is necessary before modifying the data
    // This method is intentionally not const: if you want to know whether
    // detaching is necessary, you should be in a non-const function already
    bool needsDetach() noexcept
    {
        return ref_.loadRelaxed() > 1;
    }

    qsizetype detachCapacity(qsizetype newSize) const noexcept
    {
        if (flags & CapacityReserved && newSize < constAllocatedCapacity())
            return constAllocatedCapacity();
        return newSize;
    }

    Q_DECL_MALLOCLIKE
    static Q_CORE_EXPORT void *allocate(QArrayData **pdata, qsizetype objectSize, qsizetype alignment,
            qsizetype capacity, AllocationOption option = QArrayData::KeepSize) noexcept;
    Q_DECL_MALLOCLIKE
    static Q_CORE_EXPORT void *allocate1(QArrayData **pdata, qsizetype capacity,
                                         AllocationOption option = QArrayData::KeepSize) noexcept;
    Q_DECL_MALLOCLIKE
    static Q_CORE_EXPORT void *allocate2(QArrayData **pdata, qsizetype capacity,
                                         AllocationOption option = QArrayData::KeepSize) noexcept;

    [[nodiscard]] static Q_CORE_EXPORT std::pair<QArrayData *, void *> reallocateUnaligned(QArrayData *data, void *dataPointer,
            qsizetype objectSize, qsizetype newCapacity, AllocationOption option) noexcept;
    static Q_CORE_EXPORT void deallocate(QArrayData *data, qsizetype objectSize,
            qsizetype alignment) noexcept;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QArrayData::ArrayOptions)

namespace QtPrivate {
// QArrayData with strictest alignment requirements supported by malloc()
#if defined(Q_PROCESSOR_X86_32) && defined(Q_CC_GNU)
// GCC's definition is incorrect since GCC 8 (commit r240248 in SVN; commit
// 63012d9a57edc950c5f30242d1e19318b5708060 in Git). This is applied to all
// GCC-like compilers in case they decide to follow GCC's lead in being wrong.
constexpr size_t MaxPrimitiveAlignment = 2 * sizeof(void *);
#else
constexpr size_t MaxPrimitiveAlignment = alignof(std::max_align_t);
#endif

struct alignas(MaxPrimitiveAlignment) AlignedQArrayData : QArrayData
{
};
}

template <class T>
struct QTypedArrayData
    : QArrayData
{
    struct AlignmentDummy { QtPrivate::AlignedQArrayData header; T data; };

    [[nodiscard]] static std::pair<QTypedArrayData *, T *> allocate(qsizetype capacity, AllocationOption option = QArrayData::KeepSize)
    {
        static_assert(sizeof(QTypedArrayData) == sizeof(QArrayData));
        QArrayData *d;
        void *result;
        if constexpr (sizeof(T) == 1) {
            // necessarily, alignof(T) == 1
            result = allocate1(&d, capacity, option);
        } else if constexpr (sizeof(T) == 2) {
            // alignof(T) may be 1, but that makes no difference
            result = allocate2(&d, capacity, option);
        } else {
            result = QArrayData::allocate(&d, sizeof(T), alignof(AlignmentDummy), capacity, option);
        }
#if __has_builtin(__builtin_assume_aligned)
        // and yet we do offer results that have stricter alignment
        result = __builtin_assume_aligned(result, Q_ALIGNOF(AlignmentDummy));
#endif
        return {static_cast<QTypedArrayData *>(d), static_cast<T *>(result)};
    }

    static std::pair<QTypedArrayData *, T *>
    reallocateUnaligned(QTypedArrayData *data, T *dataPointer, qsizetype capacity, AllocationOption option)
    {
        static_assert(sizeof(QTypedArrayData) == sizeof(QArrayData));
        std::pair<QArrayData *, void *> pair =
                QArrayData::reallocateUnaligned(data, dataPointer, sizeof(T), capacity, option);
        return {static_cast<QTypedArrayData *>(pair.first), static_cast<T *>(pair.second)};
    }

    static void deallocate(QArrayData *data) noexcept
    {
        static_assert(sizeof(QTypedArrayData) == sizeof(QArrayData));
        QArrayData::deallocate(data, sizeof(T), alignof(AlignmentDummy));
    }

    static T *dataStart(QArrayData *data, qsizetype alignment) noexcept
    {
        // Alignment is a power of two
        Q_ASSERT(alignment >= qsizetype(alignof(QArrayData)) && !(alignment & (alignment - 1)));
        void *start =  reinterpret_cast<void *>(
            (quintptr(data) + sizeof(QArrayData) + alignment - 1) & ~(alignment - 1));
        return static_cast<T *>(start);
    }

    constexpr static qsizetype maxSize() noexcept
    {
        // -1 to deal with the pointer one-past-the-end
        return (QtPrivate::MaxAllocSize - sizeof(QtPrivate::AlignedQArrayData) - 1) / sizeof(T);
    }
    constexpr static qsizetype max_size() noexcept
    {
        return maxSize();
    }
};

namespace QtPrivate {
struct Q_CORE_EXPORT QContainerImplHelper
{
    enum CutResult { Null, Empty, Full, Subset };
    static constexpr CutResult mid(qsizetype originalLength, qsizetype *_position, qsizetype *_length)
    {
        qsizetype &position = *_position;
        qsizetype &length = *_length;
        if (position > originalLength) {
            position = 0;
            length = 0;
            return Null;
        }

        if (position < 0) {
            if (length < 0 || length + position >= originalLength) {
                position = 0;
                length = originalLength;
                return Full;
            }
            if (length + position <= 0) {
                position = length = 0;
                return Null;
            }
            length += position;
            position = 0;
        } else if (size_t(length) > size_t(originalLength - position)) {
            length = originalLength - position;
        }

        if (position == 0 && length == originalLength)
            return Full;

        return length > 0 ? Subset : Empty;
    }
};
}

#undef Q_DECL_MALLOCLIKE

QT_END_NAMESPACE

#endif // include guard
