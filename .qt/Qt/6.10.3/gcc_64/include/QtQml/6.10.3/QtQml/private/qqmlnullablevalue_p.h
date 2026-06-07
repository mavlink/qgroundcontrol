// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLNULLABLEVALUE_P_H
#define QQMLNULLABLEVALUE_P_H

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

QT_BEGIN_NAMESPACE

template<typename T>
struct QQmlNullableValue
{
    QQmlNullableValue() = default;

    QQmlNullableValue(const QQmlNullableValue<T> &o)
        : m_value(o.m_value)
        , m_isNull(o.m_isNull)
    {}

    QQmlNullableValue(QQmlNullableValue<T> &&o) noexcept
        : m_value(std::move(o.m_value))
        , m_isNull(std::exchange(o.m_isNull, true))
    {}

    QQmlNullableValue(const T &t)
        : m_value(t)
        , m_isNull(false)
    {}

    QQmlNullableValue(T &&t) noexcept
        : m_value(std::move(t))
        , m_isNull(false)
    {}

    QQmlNullableValue<T> &operator=(const QQmlNullableValue<T> &o)
    {
        if (&o != this) {
            m_value = o.m_value;
            m_isNull = o.m_isNull;
        }
        return *this;
    }

    QQmlNullableValue<T> &operator=(QQmlNullableValue<T> &&o) noexcept
    {
        if (&o != this) {
            m_value = std::move(o.m_value);
            m_isNull = std::exchange(o.m_isNull, true);
        }
        return *this;
    }

    QQmlNullableValue<T> &operator=(const T &t)
    {
        m_value = t;
        m_isNull = false;
        return *this;
    }

    QQmlNullableValue<T> &operator=(T &&t) noexcept
    {
        m_value = std::move(t);
        m_isNull = false;
        return *this;
    }

    const T &value() const { return m_value; }
    operator T() const { return m_value; }

    void invalidate() { m_isNull = true; }
    bool isValid() const { return !m_isNull; }

private:
    T m_value = T();
    bool m_isNull = true;
};

QT_END_NAMESPACE

#endif // QQMLNULLABLEVALUE_P_H
