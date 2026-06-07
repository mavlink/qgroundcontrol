// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
// Qt-Security score:significant

#ifndef QDEFERREDPOINTER_P_H
#define QDEFERREDPOINTER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.

#include <qtqmlcompilerexports.h>

#include <QtCore/private/qglobal_p.h>
#include <QtCore/qsharedpointer.h>

QT_BEGIN_NAMESPACE

template<typename T>
class QDeferredSharedPointer;

template<typename T>
class QDeferredWeakPointer;

template<typename T>
class QDeferredFactory
{
public:
    bool isValid() const;

private:
    friend class QDeferredSharedPointer<const T>;
    friend class QDeferredWeakPointer<const T>;
    friend class QDeferredSharedPointer<T>;
    friend class QDeferredWeakPointer<T>;
    void populate(const QSharedPointer<T> &) const;
};

template<typename T>
class QDeferredSharedPointer
{
public:
    using Factory = QDeferredFactory<std::remove_const_t<T>>;

    Q_NODISCARD_CTOR QDeferredSharedPointer() = default;

    Q_NODISCARD_CTOR QDeferredSharedPointer(QSharedPointer<T> data)
        : m_data(std::move(data))
    {}

    Q_NODISCARD_CTOR QDeferredSharedPointer(QWeakPointer<T> data)
        : m_data(std::move(data))
    {}

    Q_NODISCARD_CTOR QDeferredSharedPointer(QSharedPointer<T> data, QSharedPointer<Factory> factory)
        : m_data(std::move(data)), m_factory(std::move(factory))
    {
        // You have to provide a valid pointer if you provide a factory. We cannot allocate the
        // pointer for you because then two copies of the same QDeferredSharedPointer will diverge
        // and lazy-load two separate data objects.
        Q_ASSERT(!m_data.isNull() || m_factory.isNull());
    }

    [[nodiscard]] operator QSharedPointer<T>() const
    {
        lazyLoad();
        return m_data;
    }

    operator QDeferredSharedPointer<const T>() const { return { m_data, m_factory }; }

    [[nodiscard]] T &operator*() const { lazyLoad(); return m_data.operator*(); }
    [[nodiscard]] T *operator->() const { lazyLoad(); return m_data.operator->(); }

    bool isNull() const
    {
        return m_data.isNull();
    }

    explicit operator bool() const noexcept { return !isNull(); }
    bool operator !() const noexcept { return isNull(); }

    [[nodiscard]] T *data() const { lazyLoad(); return m_data.data(); }
    [[nodiscard]] T *get() const { return data(); }

    friend size_t qHash(const QDeferredSharedPointer &ptr, size_t seed = 0)
    {
        // This is a hash of the pointer, not the data.
        return qHash(ptr.m_data, seed);
    }

    friend bool operator==(const QDeferredSharedPointer &a, const QDeferredSharedPointer &b)
    {
        // This is a comparison of the pointers, not their data. As we require the pointers to
        // be given in the ctor, we can do this.
        return a.m_data == b.m_data;
    }

    friend bool operator!=(const QDeferredSharedPointer &a, const QDeferredSharedPointer &b)
    {
        return !(a == b);
    }

    friend bool operator<(const QDeferredSharedPointer &a, const QDeferredSharedPointer &b)
    {
        return a.m_data < b.m_data;
    }

    friend bool operator<=(const QDeferredSharedPointer &a, const QDeferredSharedPointer &b)
    {
        return a.m_data <= b.m_data;
    }

    friend bool operator>(const QDeferredSharedPointer &a, const QDeferredSharedPointer &b)
    {
        return a.m_data > b.m_data;
    }

    friend bool operator>=(const QDeferredSharedPointer &a, const QDeferredSharedPointer &b)
    {
        return a.m_data >= b.m_data;
    }

    template <typename U>
    friend bool operator==(const QDeferredSharedPointer &a, const QSharedPointer<U> &b)
    {
        return a.m_data == b;
    }

    template <typename U>
    friend bool operator!=(const QDeferredSharedPointer &a, const QSharedPointer<U> &b)
    {
        return !(a == b);
    }

    template <typename U>
    friend bool operator==(const QSharedPointer<U> &a, const QDeferredSharedPointer &b)
    {
        return b == a;
    }

    template <typename U>
    friend bool operator!=(const QSharedPointer<U> &a, const QDeferredSharedPointer &b)
    {
        return b != a;
    }

    Factory *factory() const
    {
        return (m_factory && m_factory->isValid()) ? m_factory.data() : nullptr;
    }

    void resetFactory(const Factory& newFactory)
    {
        m_data.reset();
        *m_factory = newFactory;
    }

private:
    friend class QDeferredWeakPointer<T>;

    void lazyLoad() const
    {
        if (Factory *f = factory()) {
            Factory localFactory;
            std::swap(localFactory, *f); // Swap before executing, to avoid recursion
            localFactory.populate(m_data.template constCast<std::remove_const_t<T>>());
        }
    }

    QSharedPointer<T> m_data;
    QSharedPointer<Factory> m_factory;
};

template<typename T>
class QDeferredWeakPointer
{
public:
    using Factory = QDeferredFactory<std::remove_const_t<T>>;

    Q_NODISCARD_CTOR QDeferredWeakPointer() = default;

    Q_NODISCARD_CTOR QDeferredWeakPointer(const QDeferredSharedPointer<T> &strong)
        : m_data(strong.m_data), m_factory(strong.m_factory)
    {
    }

    Q_NODISCARD_CTOR QDeferredWeakPointer(QWeakPointer<T> data, QWeakPointer<Factory> factory)
        : m_data(data), m_factory(factory)
    {}

    [[nodiscard]] operator QWeakPointer<T>() const
    {
        lazyLoad();
        return m_data;
    }

    [[nodiscard]] operator QDeferredSharedPointer<T>() const
    {
        return QDeferredSharedPointer<T>(m_data.toStrongRef(), m_factory.toStrongRef());
    }

    operator QDeferredWeakPointer<const T>() const { return {m_data, m_factory}; }

    [[nodiscard]] QSharedPointer<T> toStrongRef() const
    {
        return QWeakPointer<T>(*this).toStrongRef();
    }

    bool isNull() const { return m_data.isNull(); }

    explicit operator bool() const noexcept { return !isNull(); }
    bool operator !() const noexcept { return isNull(); }

    friend bool operator==(const QDeferredWeakPointer &a, const QDeferredWeakPointer &b)
    {
        return a.m_data == b.m_data;
    }

    friend bool operator!=(const QDeferredWeakPointer &a, const QDeferredWeakPointer &b)
    {
        return !(a == b);
    }

private:
    void lazyLoad() const
    {
        if (m_factory) {
            auto factory = m_factory.toStrongRef();
            if (factory->isValid()) {
                Factory localFactory;
                std::swap(localFactory, *factory); // Swap before executing, to avoid recursion
                localFactory.populate(
                        m_data.toStrongRef().template constCast<std::remove_const_t<T>>());
            }
        }
    }

    QWeakPointer<T> m_data;
    QWeakPointer<Factory> m_factory;
};


QT_END_NAMESPACE

#endif // QDEFERREDPOINTER_P_H
