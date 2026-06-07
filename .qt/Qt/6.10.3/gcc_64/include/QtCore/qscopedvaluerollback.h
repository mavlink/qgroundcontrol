// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QSCOPEDVALUEROLLBACK_H
#define QSCOPEDVALUEROLLBACK_H

#include <QtCore/qglobal.h>

#include <QtCore/q20utility.h>

QT_BEGIN_NAMESPACE

template <typename T>
class QScopedValueRollback
{
public:
    Q_NODISCARD_CTOR
    explicit constexpr QScopedValueRollback(T &var)
        : varRef(var), oldValue(var)
    {
    }

    Q_NODISCARD_CTOR
    explicit constexpr QScopedValueRollback(T &var, T value)
        : varRef(var), oldValue(q20::exchange(var, std::move(value)))
    {
    }

    Q_DECL_CONSTEXPR_DTOR
    ~QScopedValueRollback()
    {
        varRef = std::move(oldValue);
    }

    constexpr void commit()
    {
        oldValue = varRef;
    }

private:
    T &varRef;
    T oldValue;

    Q_DISABLE_COPY_MOVE(QScopedValueRollback)
};

QT_END_NAMESPACE

#endif // QSCOPEDVALUEROLLBACK_H
