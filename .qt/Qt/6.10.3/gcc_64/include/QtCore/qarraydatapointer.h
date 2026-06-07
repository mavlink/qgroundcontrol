// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QARRAYDATAPOINTER_H
#define QARRAYDATAPOINTER_H

#include <QtCore/qarraydataops.h>
#include <QtCore/qcontainertools_impl.h>

#include <QtCore/q20functional.h>
#include <QtCore/q20memory.h>

QT_BEGIN_NAMESPACE

template <class T>
struct QArrayDataPointer
{
private:
    typedef QTypedArrayData<T> Data;
    typedef QArrayDataOps<T> DataOps;

public:
    enum {
        pass_parameter_by_value =
                std::is_arithmetic<T>::value || std::is_pointer<T>::value || std::is_enum<T>::value
    };

    typedef typename std::conditional<pass_parameter_by_value, T, const T &>::type parameter_type;

    Q_NODISCARD_CTOR
    constexpr QArrayDataPointer() noexcept
        : d(nullptr), ptr(nullptr), size(0)
    {
    }

    Q_NODISCARD_CTOR
    QArrayDataPointer(const QArrayDataPointer &other) noexcept
        : d(other.d), ptr(other.ptr), size(other.size)
    {
        ref();
    }

    Q_NODISCARD_CTOR
    constexpr QArrayDataPointer(Data *header, T *adata, qsizetype n = 0) noexcept
        : d(header), ptr(adata), size(n)
    {
    }

    Q_NODISCARD_CTOR
    explicit QArrayDataPointer(std::pair<QTypedArrayData<T> *, T *> adata, qsizetype n = 0) noexcept
        : d(adata.first), ptr(adata.second), size(n)
    {
    }

    Q_NODISCARD_CTOR explicit
    QArrayDataPointer(qsizetype alloc, qsizetype n = 0,
                      QArrayData::AllocationOption option = QArrayData::KeepSize)
        : QArrayDataPointer(Data::allocate(alloc, option), n)
    {
    }

    Q_NODISCARD_CTOR
    static QArrayDataPointer fromRawData(const T *rawData, qsizetype length) noexcept
    {
        Q_ASSERT(rawData || !length);
        return { nullptr, const_cast<T *>(rawData), length };
    }

    QArrayDataPointer &operator=(const QArrayDataPointer &other) noexcept
    {
        QArrayDataPointer tmp(other);
        this->swap(tmp);
        return *this;
    }

    Q_NODISCARD_CTOR
    QArrayDataPointer(QArrayDataPointer &&other) noexcept
        : d(std::exchange(other.d, nullptr)),
          ptr(std::exchange(other.ptr, nullptr)),
          size(std::exchange(other.size, 0))
    {
    }

    QT_MOVE_ASSIGNMENT_OPERATOR_IMPL_VIA_MOVE_AND_SWAP(QArrayDataPointer)

    DataOps &operator*() noexcept
    {
        return *static_cast<DataOps *>(this);
    }

    DataOps *operator->() noexcept
    {
        return static_cast<DataOps *>(this);
    }

    const DataOps &operator*() const noexcept
    {
        return *static_cast<const DataOps *>(this);
    }

    const DataOps *operator->() const noexcept
    {
        return static_cast<const DataOps *>(this);
    }

    ~QArrayDataPointer()
    {
        if (!deref()) {
            (*this)->destroyAll();
            Data::deallocate(d);
        }
    }

    constexpr bool isNull() const noexcept
    {
        return !ptr;
    }

    T *data() noexcept { return ptr; }
    const T *data() const noexcept { return ptr; }

    T *begin() noexcept { return data(); }
    T *end() noexcept { return data() + size; }
    const T *begin() const noexcept { return data(); }
    const T *end() const noexcept { return data() + size; }
    const T *constBegin() const noexcept { return data(); }
    const T *constEnd() const noexcept { return data() + size; }

    void swap(QArrayDataPointer &other) noexcept
    {
        qt_ptr_swap(d, other.d);
        qt_ptr_swap(ptr, other.ptr);
        std::swap(size, other.size);
    }

    void clear() noexcept(std::is_nothrow_destructible<T>::value)
    {
        QArrayDataPointer tmp;
        swap(tmp);
    }

    void detach(QArrayDataPointer *old = nullptr)
    {
        if (needsDetach())
            reallocateAndGrow(QArrayData::GrowsAtEnd, 0, old);
    }

    /*! \internal

        Reinterprets the data of this QArrayDataPointer to type X. It's the
        caller's responsibility to ensure that the data contents are valid and
        properly aligned, particularly if T and X are not trivial types (i.e,
        don't do that). The current size is kept and the allocated capacity is
        updated to account for the difference in the element type's size.

        This is used in QString::fromLatin1 to perform in-place conversion of
        QString to QByteArray.
    */
    template <typename X> QArrayDataPointer<X> reinterpreted() &&
    {
        if (sizeof(T) != sizeof(X)) {
            Q_ASSERT(!d->isShared());
            d->alloc = d->alloc * sizeof(T) / sizeof(X);
        }
        auto od = reinterpret_cast<QTypedArrayData<X> *>(std::exchange(d, nullptr));
        auto optr = reinterpret_cast<X *>(std::exchange(ptr, nullptr));
        return { od, optr, std::exchange(size, 0) };
    }

    /*! \internal

        Detaches this (optionally) and grows to accommodate the free space for
        \a n elements at the required side. The side is determined from \a pos.

        \a data pointer can be provided when the caller knows that \a data
        points into range [this->begin(), this->end()). In case it is, *data
        would be updated so that it continues to point to the element it was
        pointing to before the data move. if \a data does not point into range,
        one can/should pass \c nullptr.

        Similarly to \a data, \a old, pointer to a default-constructed QADP, can
        be provided when the caller expects to e.g. copy the data from this to
        itself:
        \code
        QList<T> list(5);
        qsizetype pos = getArbitraryPos();
        list.insert(pos, list.begin(), list.end());
        \endcode

        The default rule would be: \a data and \a old must either both be valid
        pointers, or both equal to \c nullptr.
    */
    void detachAndGrow(QArrayData::GrowthPosition where, qsizetype n, const T **data,
                       QArrayDataPointer *old)
    {
        const bool detach = needsDetach();
        bool readjusted = false;
        if (!detach) {
            if (!n || (where == QArrayData::GrowsAtBeginning && freeSpaceAtBegin() >= n)
                || (where == QArrayData::GrowsAtEnd && freeSpaceAtEnd() >= n))
                return;
            readjusted = tryReadjustFreeSpace(where, n, data);
            Q_ASSERT(!readjusted
                     || (where == QArrayData::GrowsAtBeginning && freeSpaceAtBegin() >= n)
                     || (where == QArrayData::GrowsAtEnd && freeSpaceAtEnd() >= n));
        }

        if (!readjusted)
            reallocateAndGrow(where, n, old);
    }

    /*! \internal

        Reallocates to accommodate the free space for \a n elements at the
        required side. The side is determined from \a pos. Might also shrink
        when n < 0.
    */
    Q_NEVER_INLINE void reallocateAndGrow(QArrayData::GrowthPosition where, qsizetype n,
                                          QArrayDataPointer *old = nullptr)
    {
        if constexpr (QTypeInfo<T>::isRelocatable && alignof(T) <= alignof(std::max_align_t)) {
            if (where == QArrayData::GrowsAtEnd && !old && !needsDetach() && n > 0) {
                (*this)->reallocate(constAllocatedCapacity() - freeSpaceAtEnd() + n, QArrayData::Grow); // fast path
                return;
            }
        }

        QArrayDataPointer dp(allocateGrow(*this, n, where));
        if (n > 0)
            Q_CHECK_PTR(dp.data());
        if (where == QArrayData::GrowsAtBeginning) {
            Q_ASSERT(dp.freeSpaceAtBegin() >= n);
        } else {
            Q_ASSERT(dp.freeSpaceAtEnd() >= n);
        }
        if (size) {
            qsizetype toCopy = size;
            if (n < 0)
                toCopy += n;
            if (needsDetach() || old)
                dp->copyAppend(begin(), begin() + toCopy);
            else
                dp->moveAppend(begin(), begin() + toCopy);
            Q_ASSERT(dp.size == toCopy);
        }

        swap(dp);
        if (old)
            old->swap(dp);
    }

    /*! \internal

        Attempts to relocate [begin(), end()) to accommodate the free space for
        \a n elements at the required side. The side is determined from \a pos.

        Returns \c true if the internal data is moved. Returns \c false when
        there is no point in moving the data or the move is impossible. If \c
        false is returned, it is the responsibility of the caller to figure out
        how to accommodate the free space for \a n elements at \a pos.

        This function expects that certain preconditions are met, e.g. the
        detach is not needed, n > 0 and so on. This is intentional to reduce the
        number of if-statements when the caller knows that preconditions would
        be satisfied.

        \sa reallocateAndGrow
    */
    bool tryReadjustFreeSpace(QArrayData::GrowthPosition pos, qsizetype n, const T **data = nullptr)
    {
        Q_ASSERT(!this->needsDetach());
        Q_ASSERT(n > 0);
        Q_ASSERT((pos == QArrayData::GrowsAtEnd && this->freeSpaceAtEnd() < n)
                 || (pos == QArrayData::GrowsAtBeginning && this->freeSpaceAtBegin() < n));

        const qsizetype capacity = this->constAllocatedCapacity();
        const qsizetype freeAtBegin = this->freeSpaceAtBegin();
        const qsizetype freeAtEnd = this->freeSpaceAtEnd();

        qsizetype dataStartOffset = 0;
        // algorithm:
        //   a. GrowsAtEnd: relocate if space at begin AND size < (capacity * 2) / 3
        //      [all goes to free space at end]:
        //      new free space at begin = 0
        //
        //   b. GrowsAtBeginning: relocate if space at end AND size < capacity / 3
        //      [balance the free space]:
        //      new free space at begin = n + (total free space - n) / 2
        if (pos == QArrayData::GrowsAtEnd && freeAtBegin >= n
            && ((3 * this->size) < (2 * capacity))) {
            // dataStartOffset = 0; - done in declaration
        } else if (pos == QArrayData::GrowsAtBeginning && freeAtEnd >= n
                   && ((3 * this->size) < capacity)) {
            // total free space == capacity - size
            dataStartOffset = n + qMax(0, (capacity - this->size - n) / 2);
        } else {
            // nothing to do otherwise
            return false;
        }

        relocate(dataStartOffset - freeAtBegin, data);

        Q_ASSERT((pos == QArrayData::GrowsAtEnd && this->freeSpaceAtEnd() >= n)
                 || (pos == QArrayData::GrowsAtBeginning && this->freeSpaceAtBegin() >= n));
        return true;
    }

    /*! \internal

        Relocates [begin(), end()) by \a offset and updates \a data if it is not
        \c nullptr and points into [begin(), end()).
    */
    void relocate(qsizetype offset, const T **data = nullptr)
    {
        T *res = this->ptr + offset;
        QtPrivate::q_relocate_overlap_n(this->ptr, this->size, res);
        // first update data pointer, then this->ptr
        if (data && QtPrivate::q_points_into_range(*data, *this))
            *data += offset;
        this->ptr = res;
    }

    template <typename InputIterator, typename Projection = q20::identity>
    void assign(InputIterator first, InputIterator last, Projection proj = {})
    {
        // This function only provides the basic exception guarantee.
        constexpr bool IsFwdIt = std::is_convertible_v<
                typename std::iterator_traits<InputIterator>::iterator_category,
                std::forward_iterator_tag>;
        constexpr bool IsIdentity = std::is_same_v<Projection, q20::identity>;

        if constexpr (IsFwdIt) {
            const qsizetype n = std::distance(first, last);
            if (needsDetach() || n > constAllocatedCapacity()) {
                QArrayDataPointer allocated(detachCapacity(n));
                swap(allocated);
            }
        } else if (needsDetach()) {
            QArrayDataPointer allocated(allocatedCapacity());
            swap(allocated);
            // We don't want to copy data that we know we'll overwrite
        }

        auto offset = freeSpaceAtBegin();
        const auto capacityBegin = begin() - offset;
        const auto prependBufferEnd = begin();

        if constexpr (!std::is_nothrow_constructible_v<T, decltype(std::invoke(proj, *first))>
                      || !std::is_nothrow_invocable_v<Projection, decltype(*first)>)
        {
            // If construction can throw, and we have freeSpaceAtBegin(),
            // it's easiest to just clear the container and start fresh.
            // The alternative would be to keep track of two active, disjoint ranges.
            if (offset) {
                (*this)->truncate(0);
                setBegin(capacityBegin);
                offset = 0;
            }
        }

        auto dst = capacityBegin;
        const auto dend = end();
        if (offset) { // avoids dead stores
            setBegin(capacityBegin); // undo prepend optimization

            // By construction, the following loop is nothrow!
            // (otherwise, we can't reach here)
            // Assumes InputIterator operations don't throw.
            // (but we can't statically assert that, as these operations
            //  have preconditons, so typically aren't noexcept)
            while (true) {
                if (dst == prependBufferEnd) {  // ran out of prepend buffer space
                    size += offset;
                    // we now have a contiguous buffer, continue with the main loop:
                    break;
                }
                if (first == last) {            // ran out of elements to assign
                    std::destroy(prependBufferEnd, dend);
                    size = dst - begin();
                    return;
                }
                // construct element in prepend buffer
                q20::construct_at(dst, std::invoke(proj, *first));
                ++dst;
                ++first;
            }
        }

        while (true) {
            if (first == last) {    // ran out of elements to assign
                std::destroy(dst, dend);
                break;
            }
            if (dst == dend) {      // ran out of existing elements to overwrite
                if constexpr (IsFwdIt && IsIdentity) {
                    dst = std::uninitialized_copy(first, last, dst);
                    break;
                } else if constexpr (IsFwdIt && !IsIdentity
                           && std::is_nothrow_constructible_v<T, decltype(std::invoke(proj, *first))>
                           && std::is_nothrow_invocable_v<Projection, decltype(*first)>)
                {
                    for (; first != last; ++dst, ++first)   // uninitialized_copy with projection
                        q20::construct_at(dst, std::invoke(proj, *first));
                    break;
                } else {
                    do {
                        (*this)->emplace(size, std::invoke(proj, *first));
                    } while (++first != last);
                    return;         // size() is already correct (and dst invalidated)!
                }
            }
            *dst = std::invoke(proj, *first);    // overwrite existing element
            ++dst;
            ++first;
        }
        size = dst - begin();
    }

    QArrayDataPointer sliced(qsizetype pos, qsizetype n) const &
    {
        QArrayDataPointer result(n);
        std::uninitialized_copy_n(begin() + pos, n, result.begin());
        result.size = n;
        return result;
    }

    QArrayDataPointer sliced(qsizetype pos, qsizetype n) &&
    {
        if (needsDetach())
            return sliced(pos, n);
        T *newBeginning = begin() + pos;
        std::destroy(begin(), newBeginning);
        std::destroy(newBeginning + n, end());
        setBegin(newBeginning);
        size = n;
        return std::move(*this);
    }

    void appendInitialize(qsizetype newSize)
    {
        Q_ASSERT(this->isMutable());
        Q_ASSERT(!this->isShared());
        Q_ASSERT(newSize > this->size);
        Q_ASSERT(newSize - this->size <= this->freeSpaceAtEnd());

        T *const b = this->begin() + this->size;
        T *const e = this->begin() + newSize;
        q17::uninitialized_value_construct(b, e);
        this->size = newSize;
    }

    // forwards from QArrayData
    qsizetype allocatedCapacity() noexcept { return d ? d->allocatedCapacity() : 0; }
    qsizetype constAllocatedCapacity() const noexcept { return d ? d->constAllocatedCapacity() : 0; }
    void ref() noexcept { if (d) d->ref(); }
    bool deref() noexcept { return !d || d->deref(); }
    bool isMutable() const noexcept { return d; } // Returns false if this object is fromRawData()
    bool isShared() const noexcept { return !d || d->isShared(); }
    bool isSharedWith(const QArrayDataPointer &other) const noexcept { return d && d == other.d; }
    bool needsDetach() const noexcept { return !d || d->needsDetach(); }
    qsizetype detachCapacity(qsizetype newSize) const noexcept { return d ? d->detachCapacity(newSize) : newSize; }
    const typename Data::ArrayOptions flags() const noexcept { return d ? d->flags : Data::ArrayOptionDefault; }
    void setFlag(typename Data::ArrayOptions f) noexcept { Q_ASSERT(d); d->flags |= f; }
    void clearFlag(typename Data::ArrayOptions f) noexcept { if (d) d->flags &= ~f; }

    Data *d_ptr() noexcept { return d; }
    void setBegin(T *begin) noexcept { ptr = begin; }

    qsizetype freeSpaceAtBegin() const noexcept
    {
        if (d == nullptr)
            return 0;
        return this->ptr - Data::dataStart(d, alignof(typename Data::AlignmentDummy));
    }

    qsizetype freeSpaceAtEnd() const noexcept
    {
        if (d == nullptr)
            return 0;
        return d->constAllocatedCapacity() - freeSpaceAtBegin() - this->size;
    }

    // allocate and grow. Ensure that at the minimum requiredSpace is available at the requested end
    static QArrayDataPointer allocateGrow(const QArrayDataPointer &from, qsizetype n, QArrayData::GrowthPosition position)
    {
        // calculate new capacity. We keep the free capacity at the side that does not have to grow
        // to avoid quadratic behavior with mixed append/prepend cases

        // use qMax below, because constAllocatedCapacity() can be 0 when using fromRawData()
        qsizetype minimalCapacity = qMax(from.size, from.constAllocatedCapacity()) + n;
        // subtract the free space at the side we want to allocate. This ensures that the total size requested is
        // the existing allocation at the other side + size + n.
        minimalCapacity -= (position == QArrayData::GrowsAtEnd) ? from.freeSpaceAtEnd() : from.freeSpaceAtBegin();
        qsizetype capacity = from.detachCapacity(minimalCapacity);
        const bool grows = capacity > from.constAllocatedCapacity();
        auto [header, dataPtr] = Data::allocate(capacity, grows ? QArrayData::Grow : QArrayData::KeepSize);
        const bool valid = header != nullptr && dataPtr != nullptr;
        if (!valid)
            return QArrayDataPointer(header, dataPtr);

        // Idea: * when growing backwards, adjust pointer to prepare free space at the beginning
        //       * when growing forward, adjust by the previous data pointer offset
        dataPtr += (position == QArrayData::GrowsAtBeginning)
                ? n + qMax(0, (header->alloc - from.size - n) / 2)
                : from.freeSpaceAtBegin();
        header->flags = from.flags();
        return QArrayDataPointer(header, dataPtr);
    }

    friend bool operator==(const QArrayDataPointer &lhs, const QArrayDataPointer &rhs) noexcept
    {
        return lhs.data() == rhs.data() && lhs.size == rhs.size;
    }

    friend bool operator!=(const QArrayDataPointer &lhs, const QArrayDataPointer &rhs) noexcept
    {
        return lhs.data() != rhs.data() || lhs.size != rhs.size;
    }

    Data *d;
    T *ptr;
    qsizetype size;
};

template <class T>
inline void swap(QArrayDataPointer<T> &p1, QArrayDataPointer<T> &p2) noexcept
{
    p1.swap(p2);
}

////////////////////////////////////////////////////////////////////////////////
//  Q_ARRAY_LITERAL

// The idea here is to place a (read-only) copy of header and array data in an
// mmappable portion of the executable (typically, .rodata section).

// Hide array inside a lambda
#define Q_ARRAY_LITERAL(Type, ...) \
    ([]() -> QArrayDataPointer<Type> { \
        static Type const data[] = { __VA_ARGS__ }; \
        return QArrayDataPointer<Type>::fromRawData(const_cast<Type *>(data), std::size(data)); \
    }())
/**/

QT_END_NAMESPACE

#endif // include guard
