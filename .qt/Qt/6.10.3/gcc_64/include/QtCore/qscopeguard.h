// Copyright (C) 2018 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Sérgio Martins <sergio.martins@kdab.com>
// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QSCOPEGUARD_H
#define QSCOPEGUARD_H

#include <QtCore/qtclasshelpermacros.h>
#include <QtCore/qcompilerdetection.h>
#include <QtCore/qtconfigmacros.h>

#include <type_traits>
#include <utility>

QT_BEGIN_NAMESPACE

template <typename F>
class QScopeGuard
{
public:
    Q_NODISCARD_CTOR
    explicit QScopeGuard(F &&f) noexcept
        : m_func(std::move(f))
    {
    }

    Q_NODISCARD_CTOR
    explicit QScopeGuard(const F &f) noexcept
        : m_func(f)
    {
    }

    Q_NODISCARD_CTOR
    QScopeGuard(QScopeGuard &&other) noexcept
        : m_func(std::move(other.m_func))
        , m_invoke(std::exchange(other.m_invoke, false))
    {
    }

    ~QScopeGuard() noexcept
    {
        if (m_invoke)
            m_func();
    }

    void dismiss() noexcept
    {
        m_invoke = false;
    }

private:
    Q_DISABLE_COPY(QScopeGuard)

    F m_func;
    bool m_invoke = true;
};

template <typename F> QScopeGuard(F(&)()) -> QScopeGuard<F(*)()>;

//! [qScopeGuard]
template <typename F>
[[nodiscard]] QScopeGuard<typename std::decay<F>::type> qScopeGuard(F &&f)
{
    return QScopeGuard<typename std::decay<F>::type>(std::forward<F>(f));
}

QT_END_NAMESPACE

#endif // QSCOPEGUARD_H
