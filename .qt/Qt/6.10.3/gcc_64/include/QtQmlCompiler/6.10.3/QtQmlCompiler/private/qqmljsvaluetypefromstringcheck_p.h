// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
// Qt-Security score:significant

#ifndef QQMLJSVALUETYPEFROMSTRINGCHECK_H
#define QQMLJSVALUETYPEFROMSTRINGCHECK_H

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
#include <QtCore/qstring.h>

#include <qtqmlcompilerexports.h>

QT_BEGIN_NAMESPACE

struct Q_QMLCOMPILER_EXPORT QQmlJSStructuredTypeError
{
    QString code;
    bool constructedFromInvalidString = false;

    bool isValid() const { return !code.isEmpty() || constructedFromInvalidString; }
    static QQmlJSStructuredTypeError withInvalidString() { return { QString(), true }; }
    static QQmlJSStructuredTypeError withValidString() { return { QString(), false }; }
    static QQmlJSStructuredTypeError fromSuggestedString(const QString &enhancedString)
    {
        return { enhancedString, false };
    }
};


class Q_QMLCOMPILER_EXPORT QQmlJSValueTypeFromStringCheck
{
public:
    static QQmlJSStructuredTypeError hasError(const QString &typeName, const QString &value);
};

QT_END_NAMESPACE

#endif // QQMLJSVALUETYPEFROMSTRINGCHECK_H
