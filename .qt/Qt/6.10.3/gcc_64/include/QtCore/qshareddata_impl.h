// Copyright (C) 2020 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Giuseppe D'Angelo <giuseppe.dangelo@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#if 0
#pragma qt_sync_skip_header_check
#pragma qt_sync_stop_processing
#endif

#ifndef QSHAREDDATA_IMPL_H
#define QSHAREDDATA_IMPL_H

#include <QtCore/qcompare.h>
#include <QtCore/qglobal.h>
#include <QtCore/qshareddata.h>

QT_BEGIN_NAMESPACE

namespace QtPrivate {

template <typename T>
class QExplicitlySharedDataPointerV2
{
    Qt::totally_ordered_wrapper<T *> d;

public:
    constexpr QExplicitlySharedDataPointerV2() noexcept : d(nullptr) {}

    explicit QExplicitlySharedDataPointerV2(T *t) noexcept
        : d(t)
    {
        if (d)
            d->ref.ref();
    }

    QExplicitlySharedDataPointerV2(T *t, QAdoptSharedDataTag) noexcept
        : d(t)
    {
    }

    QExplicitlySharedDataPointerV2(const QExplicitlySharedDataPointerV2 &other) noexcept
        : d(other.d)
    {
        if (d)
            d->ref.ref();
    }

    QExplicitlySharedDataPointerV2 &operator=(const QExplicitlySharedDataPointerV2 &other) noexcept
    {
        QExplicitlySharedDataPointerV2 copy(other);
        swap(copy);
        return *this;
    }

    QExplicitlySharedDataPointerV2(QExplicitlySharedDataPointerV2 &&other) noexcept
        : d(std::exchange(other.d, nullptr))
    {
    }

    QExplicitlySharedDataPointerV2 &operator=(QExplicitlySharedDataPointerV2 &&other) noexcept
    {
        QExplicitlySharedDataPointerV2 moved(std::move(other));
        swap(moved);
        return *this;
    }

    ~QExplicitlySharedDataPointerV2()
    {
        if (d && !d->ref.deref())
            delete d.get();
    }

    void detach()
    {
        if (!d) {
            // should this codepath be here on in all user's detach()?
            d.reset(new T);
            d->ref.ref();
        } else if (d->ref.loadRelaxed() != 1) {
            // TODO: qAtomicDetach here...?
            QExplicitlySharedDataPointerV2 copy(new T(*d));
            swap(copy);
        }
    }

    void reset(T *t = nullptr) noexcept
    {
        if (d && !d->ref.deref())
            delete d.get();
        d.reset(t);
        if (d)
            d->ref.ref();
    }

    constexpr T *take() noexcept
    {
        return std::exchange(d, nullptr).get();
    }

    bool isShared() const noexcept
    {
        return d && d->ref.loadRelaxed() != 1;
    }

    constexpr void swap(QExplicitlySharedDataPointerV2 &other) noexcept
    {
        qt_ptr_swap(d, other.d);
    }

    // important change from QExplicitlySharedDataPointer: deep const
    constexpr T &operator*() { return *(d.get()); }
    constexpr T *operator->() { return d.get(); }
    constexpr const T &operator*() const { return *(d.get()); }
    constexpr const T *operator->() const { return d.get(); }

    constexpr T *data() noexcept { return d.get(); }
    constexpr const T *data() const noexcept { return d.get(); }

    constexpr explicit operator bool() const noexcept { return d.get(); }

private:
    constexpr friend bool comparesEqual(const QExplicitlySharedDataPointerV2 &lhs,
                                        const QExplicitlySharedDataPointerV2 &rhs) noexcept
    { return lhs.d == rhs.d; }
    constexpr friend Qt::strong_ordering
    compareThreeWay(const QExplicitlySharedDataPointerV2 &lhs,
                    const QExplicitlySharedDataPointerV2 &rhs) noexcept
    { return Qt::compareThreeWay(lhs.d, rhs.d); }
    Q_DECLARE_STRONGLY_ORDERED_LITERAL_TYPE(QExplicitlySharedDataPointerV2)

    constexpr friend bool
    comparesEqual(const QExplicitlySharedDataPointerV2 &lhs, std::nullptr_t) noexcept
    { return lhs.d == nullptr; }
    constexpr friend Qt::strong_ordering
    compareThreeWay(const QExplicitlySharedDataPointerV2 &lhs, std::nullptr_t) noexcept
    { return Qt::compareThreeWay(lhs.d, nullptr); }
    Q_DECLARE_STRONGLY_ORDERED_LITERAL_TYPE(QExplicitlySharedDataPointerV2, std::nullptr_t)
};

template <typename T>
constexpr void swap(QExplicitlySharedDataPointerV2<T> &lhs, QExplicitlySharedDataPointerV2<T> &rhs) noexcept
{
    lhs.swap(rhs);
}

} // namespace QtPrivate

QT_END_NAMESPACE

#endif // QSHAREDDATA_IMPL_H
