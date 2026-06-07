// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQMLDOMSCANNER_P_H
#define QQMLDOMSCANNER_P_H

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

#include "qqmldom_global.h"
#include "qqmldomstringdumper_p.h"

#include <QStringList>
#include <QStringView>
#include <QtQml/private/qqmljslexer_p.h>
#include <QtQml/private/qqmljsgrammar_p.h>

QT_BEGIN_NAMESPACE

namespace QQmlJS {
namespace Dom {

class QMLDOM_EXPORT Token
{
    Q_GADGET
public:
    static bool lexKindIsDelimiter(int kind);
    static bool lexKindIsJSKeyword(int kind);
    static bool lexKindIsIdentifier(int kind);
    static bool lexKindIsStringType(int kind);
    static bool lexKindIsInvalid(int kind);
    static bool lexKindIsQmlReserved(int kind);
    static bool lexKindIsComment(int kind);

    inline Token() = default;
    inline Token(int o, int l, int lexKind) : offset(o), length(l), lexKind(lexKind) { }
    inline int begin() const { return offset; }
    inline int end() const { return offset + length; }
    void dump(const Sink &s, QStringView line = QStringView()) const;
    QString toString(QStringView line = QStringView()) const
    {
        return dumperToString([line, this](const Sink &s) { this->dump(s, line); });
    }

    static int compare(const Token &t1, const Token &t2)
    {
        if (int c = t1.offset - t2.offset)
            return c;
        if (int c = t1.length - t2.length)
            return c;
        return int(t1.lexKind) - int(t2.lexKind);
    }

    int offset = 0;
    int length = 0;
    int lexKind = QQmlJSGrammar::T_NONE;
};

inline int operator==(const Token &t1, const Token &t2)
{
    return Token::compare(t1, t2) == 0;
}
inline int operator!=(const Token &t1, const Token &t2)
{
    return Token::compare(t1, t2) != 0;
}

class QMLDOM_EXPORT Scanner
{
public:
    struct QMLDOM_EXPORT State
    {
        Lexer::State state {};
        bool regexpMightFollow = true;
        bool isMultiline() const;
        bool isMultilineComment() const;
    };

    QList<Token> operator()(QStringView text, const State &startState);
    State state() const;

private:
    bool _qmlMode = true;
    State _state;
};

} // namespace Dom
} // namespace QQmlJS
QT_END_NAMESPACE
#endif
