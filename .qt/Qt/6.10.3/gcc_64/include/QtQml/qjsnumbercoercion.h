// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QJSNUMBERCOERCION_H
#define QJSNUMBERCOERCION_H

#include <QtCore/qglobal.h>
#include <cstring>

QT_BEGIN_NAMESPACE

class QJSNumberCoercion
{
public:

    static constexpr bool isInteger(double d)
    {
        // Comparing d with itself checks for NaN and comparing d with the min and max values
        // for int also covers infinities.
        if (!equals(d, d) || d < (std::numeric_limits<int>::min)()
            || d > (std::numeric_limits<int>::max)()) {
            return false;
        }

        return equals(static_cast<int>(d), d);
    }

    static constexpr bool isArrayIndex(double d)
    {
        return d >= 0
                && equals(d, d)
                && d <= (std::numeric_limits<uint>::max)()
                && equals(static_cast<uint>(d), d);
    }

    static constexpr bool isArrayIndex(qint64 i)
    {
        return i >= 0 && i <= (std::numeric_limits<uint>::max)();
    }

    static constexpr bool isArrayIndex(quint64 i)
    {
        return i <= (std::numeric_limits<uint>::max)();
    }

    static constexpr int toInteger(double d) {
        // Check for NaN
        if (!equals(d, d))
            return 0;

        if (d >= (std::numeric_limits<int>::min)() && d <= (std::numeric_limits<int>::max)()) {
            const int i = static_cast<int>(d);
            if (equals(i, d))
                return i;
        }

        return QJSNumberCoercion(d).toInteger();
    }

    static constexpr bool equals(double lhs, double rhs)
    {
        QT_WARNING_PUSH
        QT_WARNING_DISABLE_FLOAT_COMPARE
        return lhs == rhs;
        QT_WARNING_POP
    }

    static constexpr double roundTowards0(double d)
    {
        // Check for NaN
        if (!equals(d, d))
            return +0;

        if (equals(d, 0) || std::isinf(d))
            return d;

        return d >= 0 ? std::floor(d) : std::ceil(d);
    }

private:
    constexpr QJSNumberCoercion(double dbl)
    {
        // the dbl == 0 path is guaranteed constexpr. The other one may or may not be, depending
        // on whether and how the compiler inlines the memcpy.
        // In order to declare the ctor constexpr we need one guaranteed constexpr path.
        if (!equals(dbl, 0))
            memcpy(&d, &dbl, sizeof(double));
    }

    constexpr int sign() const
    {
        return (d >> 63) ? -1 : 1;
    }

    constexpr bool isDenormal() const
    {
        return static_cast<int>((d << 1) >> 53) == 0;
    }

    constexpr int exponent() const
    {
        return static_cast<int>((d << 1) >> 53) - 1023;
    }

    constexpr quint64 significant() const
    {
        quint64 m = (d << 12) >> 12;
        if (!isDenormal())
            m |= (static_cast<quint64>(1) << 52);
        return m;
    }

    constexpr int toInteger()
    {
        int e = exponent() - 52;
        if (e < 0) {
            if (e <= -53)
                return 0;
            return sign() * static_cast<int>(significant() >> -e);
        } else {
            if (e > 31)
                return 0;
            return sign() * (static_cast<int>(significant()) << e);
        }
    }

    quint64 d = 0;
};

QT_END_NAMESPACE

#endif // QJSNUMBERCOERCION_H
