// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:header-decls-only

#ifndef QJSONPARSER_P_H
#define QJSONPARSER_P_H

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
#include <QtCore/private/qcborvalue_p.h>
#include <QtCore/qjsondocument.h>
#include <QtCore/qutf8stringview.h>

QT_BEGIN_NAMESPACE

namespace QJsonPrivate {

class Parser
{
public:
    explicit Parser(QUtf8StringView json);

    QCborValue parse(QJsonParseError *error);

private:
    inline void eatBOM();
    inline bool eatSpace();
    inline char nextToken();

    bool parseObject();
    bool parseArray();
    bool parseMember();
    bool parseString();
    bool parseValueIntoContainer();
    QCborValue parseValue();
    QCborValue parseNumber();
    const char *head;
    const char *json;
    const char *end;

    int nestingLevel;
    QJsonParseError::ParseError lastError;
    QExplicitlySharedDataPointer<QCborContainerPrivate> container;
};

}

QT_END_NAMESPACE

#endif
