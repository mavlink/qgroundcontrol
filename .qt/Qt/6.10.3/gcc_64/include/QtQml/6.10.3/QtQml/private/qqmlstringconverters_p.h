// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:data-parser

#ifndef QQMLSTRINGCONVERTERS_P_H
#define QQMLSTRINGCONVERTERS_P_H

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

#include <QtCore/qglobal.h>
#include <QtCore/qvariant.h>

#include <private/qtqmlglobal_p.h>

QT_BEGIN_NAMESPACE

class QPointF;
class QSizeF;
class QRectF;
class QString;
class QByteArray;

namespace QQmlStringConverters
{
    Q_QML_EXPORT QVariant variantFromString(const QString &, QMetaType preferredType, bool *ok = nullptr);

    Q_QML_EXPORT QVariant colorFromString(const QString &, bool *ok = nullptr);
    Q_QML_EXPORT unsigned rgbaFromString(const QString &, bool *ok = nullptr);

#if QT_CONFIG(datestring)
    Q_QML_EXPORT QDate dateFromString(const QString &, bool *ok = nullptr);
    Q_QML_EXPORT QTime timeFromString(const QString &, bool *ok = nullptr);
    Q_QML_EXPORT QDateTime dateTimeFromString(const QString &, bool *ok = nullptr);
#endif
    Q_QML_EXPORT QPointF pointFFromString(const QString &, bool *ok = nullptr);
    Q_QML_EXPORT QSizeF sizeFFromString(const QString &, bool *ok = nullptr);
    Q_QML_EXPORT QRectF rectFFromString(const QString &, bool *ok = nullptr);

    // checks if the string contains a list of doubles separated by separators, like "double1
    // separators1 double2 separators2 ..." for example.
    template<int NumParams, char16_t... separators>
    bool isValidNumberString(const QString &s, std::array<double, NumParams> *numbers = nullptr)
    {
        Q_STATIC_ASSERT_X(
                NumParams == 2 || NumParams == 3 || NumParams == 4 || NumParams == 16,
                "Unsupported number of params; add an additional case below if necessary.");
        constexpr std::array<char16_t, NumParams - 1> separatorArray{ separators... };
        // complain about missing separators when first or last entry is initialized with 0
        Q_STATIC_ASSERT_X(separatorArray[0] != 0,
                          "Did not specify any separators for isValidNumberString.");
        Q_STATIC_ASSERT_X(separatorArray[NumParams - 2] != 0,
                          "Did not specify enough separators for isValidNumberString.");

        bool floatOk = true;
        QStringView view(s);
        for (qsizetype i = 0; i < NumParams - 1; ++i) {
            const qsizetype commaIndex = view.indexOf(separatorArray[i]);
            if (commaIndex == -1)
                return false;
            const auto current = view.first(commaIndex).toDouble(&floatOk);
            if (!floatOk)
                return false;
            if (numbers)
                (*numbers)[i] = current;

            view = view.sliced(commaIndex + 1);
        }
        const auto current = view.toDouble(&floatOk);
        if (!floatOk)
            return false;
        if (numbers)
            (*numbers)[NumParams - 1] = current;

        return true;
    }

    // Constructs a value type T from the given string that contains NumParams double values
    // separated by separators, like "double1 separators1 double2 separators2 ..." for example.
    template<typename T, int NumParams, char16_t... separators>
    T valueTypeFromNumberString(const QString &s, bool *ok = nullptr)
    {
        Q_STATIC_ASSERT_X(
                NumParams == 2 || NumParams == 3 || NumParams == 4 || NumParams == 16,
                "Unsupported number of params; add an additional case below if necessary.");

        std::array<double, NumParams> parameters;
        if (!isValidNumberString<NumParams, separators...>(s, &parameters)) {
            if (ok)
                *ok = false;
            return T{};
        }

        if (ok)
            *ok = true;

        if constexpr (NumParams == 2) {
            return T(parameters[0], parameters[1]);
        } else if constexpr (NumParams == 3) {
            return T(parameters[0], parameters[1], parameters[2]);
        } else if constexpr (NumParams == 4) {
            return T(parameters[0], parameters[1], parameters[2], parameters[3]);
        } else if constexpr (NumParams == 16) {
            return T(parameters[0], parameters[1], parameters[2], parameters[3], parameters[4],
                     parameters[5], parameters[6], parameters[7], parameters[8], parameters[9],
                     parameters[10], parameters[11], parameters[12], parameters[13], parameters[14],
                     parameters[15]);
        }

        Q_UNREACHABLE_RETURN(T{});
    }
}

QT_END_NAMESPACE

#endif // QQMLSTRINGCONVERTERS_P_H
