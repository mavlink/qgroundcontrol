// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:header-decls-only

#ifndef QJSONPARSEERROR_H
#define QJSONPARSEERROR_H

#include <QtCore/qtconfigmacros.h>
#include <QtCore/qtcoreexports.h>
#include <QtCore/qtypes.h>

QT_BEGIN_NAMESPACE

class QString;

struct Q_CORE_EXPORT QJsonParseError
{
    enum ParseError {
        NoError = 0,
        UnterminatedObject,
        MissingNameSeparator,
        UnterminatedArray,
        MissingValueSeparator,
        IllegalValue,
        TerminationByNumber,
        IllegalNumber,
        IllegalEscapeSequence,
        IllegalUTF8String,
        UnterminatedString,
        MissingObject,
        DeepNesting,
        DocumentTooLarge,
        GarbageAtEnd
    };

    QString errorString() const;

    std::conditional_t<QT_VERSION_MAJOR < 7, int, qint64>
    offset = -1;
    ParseError error = NoError;
};

QT_END_NAMESPACE

#endif // QJSONPARSEERROR_H
