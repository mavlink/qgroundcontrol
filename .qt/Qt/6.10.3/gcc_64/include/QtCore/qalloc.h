// Copyright (C) 2025 Aurélien Brooke <aurelien@bahiasoft.fr>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QALLOC_H
#define QALLOC_H

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

#include <QtCore/qtconfigmacros.h>
#include <QtCore/qtcoreexports.h>
#include <QtCore/qnumeric.h>
#include <QtCore/qtypeinfo.h>

#include <cstddef>
#include <cstdlib>

QT_BEGIN_NAMESPACE

namespace QtPrivate {

/**
 * \internal
 * \return the size that would be allocated for the given request.
 *
 * Computes the actual allocation size for \a allocSize and \a alignment,
 * as determined by the active allocator, without performing the allocation.
 *
 * In practice, it only returns nonzero when using jemalloc.
 */
Q_CORE_EXPORT Q_DECL_PURE_FUNCTION
size_t expectedAllocSize(size_t allocSize, size_t alignment) noexcept;

/**
 * \internal
 * \brief Computes the best allocation size for the requested minimum capacity, and updates capacity.
 *
 * Computes the allocation size starting from \a headerSize and a requested minimum capacity in \a capacity,
 * multiplied by the \a elementSize and adjusted by the \a unusedCapacity.
 * The final capacity is written back into \a capacity.
 * The \a headerSize and \a unusedCapacity values are not included in the final reported capacity.
 */
inline size_t fittedAllocSize(size_t headerSize, size_t *capacity,
                              size_t elementSize, size_t unusedCapacity, size_t alignment) noexcept
{
    size_t totalCapacity = 0; // = capacity + unusedCapacity
    if (Q_UNLIKELY(qAddOverflow(*capacity, unusedCapacity, &totalCapacity)))
        return 0; // or handle error

    size_t payloadSize = 0; // = totalCapacity * elementSize
    if (Q_UNLIKELY(qMulOverflow(totalCapacity, elementSize, &payloadSize)))
        return 0;

    size_t allocSize = 0; // = headerSize + payloadSize
    if (Q_UNLIKELY(qAddOverflow(headerSize, payloadSize, &allocSize)))
        return 0;

    if (size_t fittedSize = expectedAllocSize(allocSize, alignment); fittedSize != 0) {
        // no need to overflow/underflow check from fittedSize,
        // since allocSize <= fittedSize <= SIZE_T_MAX
        *capacity = (fittedSize - headerSize) / elementSize - unusedCapacity;
        size_t newTotalCapacity = *capacity + unusedCapacity;
        size_t newPayloadSize = newTotalCapacity * elementSize;
        return headerSize + newPayloadSize;
    }

    return allocSize;
}

#ifdef Q_CC_GNU
__attribute__((malloc))
#endif
inline void *fittedMalloc(size_t headerSize, size_t *capacity,
                          size_t elementSize, size_t unusedCapacity) noexcept
{
    size_t allocSize = fittedAllocSize(headerSize, capacity,
                                       elementSize, unusedCapacity, alignof(std::max_align_t));
    if (Q_LIKELY(allocSize != 0))
        return malloc(allocSize);
    else
        return nullptr;
}
inline void *fittedMalloc(size_t headerSize, qsizetype *capacity,
                          size_t elementSize, size_t unusedCapacity = 0) noexcept
{
    size_t uCapacity = size_t(*capacity);
    void *ptr = fittedMalloc(headerSize, &uCapacity, elementSize, unusedCapacity);
    *capacity = qsizetype(uCapacity);
    return ptr;
}

inline void *fittedRealloc(void *ptr, size_t headerSize, size_t *capacity,
                           size_t elementSize, size_t unusedCapacity) noexcept
{
    size_t newCapacity = *capacity;
    size_t allocSize = fittedAllocSize(headerSize, &newCapacity,
                                       elementSize, unusedCapacity, alignof(std::max_align_t));
    if (Q_LIKELY(allocSize != 0)) {
        void *newPtr = realloc(ptr, allocSize);
        if (newPtr)
            *capacity = newCapacity;
        return newPtr;
    } else {
        return nullptr;
    }
}
inline void *fittedRealloc(void *ptr, size_t headerSize, qsizetype *capacity,
                           size_t elementSize, size_t unusedCapacity = 0) noexcept
{
    size_t uCapacity = size_t(*capacity);
    ptr = fittedRealloc(ptr, headerSize, &uCapacity, elementSize, unusedCapacity);
    *capacity = qsizetype(uCapacity);
    return ptr;
}

Q_CORE_EXPORT void sizedFree(void *ptr, size_t allocSize) noexcept;
inline void sizedFree(void *ptr, size_t capacity, size_t elementSize) noexcept
{
    sizedFree(ptr, capacity * elementSize);
}

} // namespace QtPrivate

QT_END_NAMESPACE

#endif // QALLOC_H
