// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSYSTEMERROR_P_H
#define QSYSTEMERROR_P_H

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
#include <qstring.h>

QT_BEGIN_NAMESPACE

class QSystemError
{
public:
    enum ErrorScope
    {
        NoError,
        StandardLibraryError,
        NativeError
    };

    constexpr explicit QSystemError(int error, ErrorScope scope)
        : errorCode(error), errorScope(scope)
    {
    }
    constexpr QSystemError() = default;

    QString toString() const { return string(errorScope, errorCode); }
    constexpr ErrorScope scope() const { return errorScope; }
    constexpr int error() const { return errorCode; }

    constexpr bool ok() const noexcept { return errorScope == NoError; }
    static constexpr QSystemError stdError(int error)
    { return QSystemError(error, StandardLibraryError); }

    static Q_CORE_EXPORT QString string(ErrorScope errorScope, int errorCode);
    static Q_CORE_EXPORT QString stdString(int errorCode = -1);
#ifdef Q_OS_WIN
    static Q_CORE_EXPORT QString windowsString(int errorCode = -1);
    using HRESULT = long;
    static Q_CORE_EXPORT QString windowsComString(HRESULT hr);

    static constexpr QSystemError nativeError(int error)
    { return QSystemError(error, NativeError); }
#endif

    // data members
    int errorCode = 0;
    ErrorScope errorScope = NoError;
};

QT_END_NAMESPACE

#endif // QSYSTEMERROR_P_H
