// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLDOMCODEFORMATTER_P_H
#define QQMLDOMCODEFORMATTER_P_H

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
#include "qqmldomfunctionref_p.h"
#include "qqmldomscanner_p.h"
#include "qqmldomlinewriter_p.h"

#include <QtCore/QStack>
#include <QtCore/QList>
#include <QtCore/QSet>
#include <QtCore/QVector>
#include <QtCore/QMetaObject>

QT_BEGIN_NAMESPACE

namespace QQmlJS {
namespace Dom {

class QMLDOM_EXPORT FormatTextStatus
{
    Q_GADGET
public:
    enum class StateType : quint8 {
        Invalid = 0,

        TopmostIntro, // The first line in a "topmost" definition.

        TopQml, // root state for qml
        TopJs, // root for js
        ObjectdefinitionOrJs, // file starts with identifier

        MultilineCommentStart,
        MultilineCommentCont,

        ImportStart, // after 'import'
        ImportMaybeDotOrVersionOrAs, // after string or identifier
        ImportDot, // after .
        ImportMaybeAs, // after version
        ImportAs,

        PropertyStart, // after 'property'
        PropertyModifiers, // after 'default' or readonly
        RequiredProperty, // after required
        PropertyListOpen, // after 'list' as a type
        PropertyName, // after the type
        PropertyMaybeInitializer, // after the identifier
        ComponentStart, // after component
        ComponentName, // after component Name

        TypeAnnotation, // after a : starting a type annotation
        TypeParameter, // after a < in a type annotation (starting type parameters)

        EnumStart, // after 'enum'

        SignalStart, // after 'signal'
        SignalMaybeArglist, // after identifier
        SignalArglistOpen, // after '('

        FunctionStart, // after 'function'
        FunctionArglistOpen, // after '(' starting function argument list
        FunctionArglistClosed, // after ')' in argument list, expecting '{'

        BindingOrObjectdefinition, // after an identifier

        BindingAssignment, // after : in a binding
        ObjectdefinitionOpen, // after {

        Expression,
        ExpressionContinuation, // at the end of the line, when the next line definitely is a
                                // continuation
        ExpressionMaybeContinuation, // at the end of the line, when the next line may be an
                                     // expression
        ExpressionOrObjectdefinition, // after a binding starting with an identifier ("x: foo")
        ExpressionOrLabel, // when expecting a statement and getting an identifier

        ParenOpen, // opening ( in expression
        BracketOpen, // opening [ in expression
        ObjectliteralOpen, // opening { in expression

        ObjectliteralAssignment, // after : in object literal

        BracketElementStart, // after starting bracket_open or after ',' in bracket_open
        BracketElementMaybeObjectdefinition, // after an identifier in bracket_element_start

        TernaryOp, // The ? : operator
        TernaryOpAfterColon, // after the : in a ternary

        JsblockOpen,

        EmptyStatement, // for a ';', will be popped directly
        BreakcontinueStatement, // for continue/break, may be followed by identifier

        IfStatement, // After 'if'
        MaybeElse, // after the first substatement in an if
        ElseClause, // The else line of an if-else construct.

        ConditionOpen, // Start of a condition in 'if', 'while', entered after opening paren

        Substatement, // The first line after a conditional or loop construct.
        SubstatementOpen, // The brace that opens a substatement block.

        LabelledStatement, // after a label

        ReturnStatement, // After 'return'
        ThrowStatement, // After 'throw'

        StatementWithCondition, // After the 'for', 'while', ... token
        StatementWithConditionParenOpen, // While inside the (...)

        TryStatement, // after 'try'
        CatchStatement, // after 'catch', nested in try_statement
        FinallyStatement, // after 'finally', nested in try_statement
        MaybeCatchOrFinally, // after ther closing '}' of try_statement and catch_statement,
                             // nested in try_statement

        DoStatement, // after 'do'
        DoStatementWhileParenOpen, // after '(' in while clause

        SwitchStatement, // After 'switch' token
        CaseStart, // after a 'case' or 'default' token
        CaseCont // after the colon in a case/default
    };
    Q_ENUM(StateType)

    static QString stateToString(StateType type);

    class State
    {
    public:
        quint16 savedIndentDepth = 0;
        StateType type = StateType::Invalid;
        bool operator==(const State &other) const
        {
            return type == other.type && savedIndentDepth == other.savedIndentDepth;
        }
        QString typeStr() const { return FormatTextStatus::stateToString(type); }
    };

    static bool isBracelessState(StateType type)
    {
        return type == StateType::IfStatement || type == StateType::ElseClause
                || type == StateType::Substatement || type == StateType::BindingAssignment
                || type == StateType::BindingOrObjectdefinition;
    }

    static bool isExpressionEndState(StateType type)
    {
        return type == StateType::TopmostIntro || type == StateType::TopJs
                || type == StateType::ObjectdefinitionOpen || type == StateType::DoStatement
                || type == StateType::JsblockOpen || type == StateType::SubstatementOpen
                || type == StateType::BracketOpen || type == StateType::ParenOpen
                || type == StateType::CaseCont || type == StateType::ObjectliteralOpen;
    }

    static FormatTextStatus initialStatus(int baseIndent = 0)
    {
        return FormatTextStatus {
            Scanner::State {},
            QVector<State>({ State { quint16(baseIndent), StateType::TopmostIntro } }), baseIndent
        };
    }

    size_t size() const { return states.size(); }

    State state(int belowTop = 0) const;

    void pushState(StateType type, quint16 savedIndentDepth)
    {
        states.append(State { savedIndentDepth, type });
    }

    State popState()
    {
        if (states.isEmpty()) {
            Q_ASSERT(false);
            return State();
        }
        State res = states.last();
        states.removeLast();
        return res;
    }

    Scanner::State lexerState = {};
    QVector<State> states;
    int finalIndent = 0;
};

class QMLDOM_EXPORT FormatPartialStatus
{
    Q_GADGET
public:

    using OnEnterCallback =
            function_ref<void(FormatTextStatus::StateType newState, int *indentDepth,
                              int *savedIndentDepth, const FormatPartialStatus &fStatus)>;

    // to determine whether a line was joined, Tokenizer needs a
    // newline character at the end, lease ensure that line contains it
    FormatPartialStatus() = default;
    FormatPartialStatus(const FormatPartialStatus &o) = default;
    FormatPartialStatus &operator=(const FormatPartialStatus &o) = default;
    FormatPartialStatus(QStringView line, const FormatOptions &options,
                        const FormatTextStatus &initialStatus)
        : line(line),
          options(options),
          initialStatus(initialStatus),
          currentStatus(initialStatus),
          currentIndent(0),
          tokenIndex(0)
    {
        Scanner::State startState = initialStatus.lexerState;
        currentIndent = initialStatus.finalIndent;
        Scanner tokenize;
        lineTokens = tokenize(line, startState);
        currentStatus.lexerState = tokenize.state();
    }

    void enterState(FormatTextStatus::StateType newState);
    void leaveState(bool statementDone);
    void turnIntoState(FormatTextStatus::StateType newState);

    const Token &tokenAt(int idx) const;
    int tokenCount() const { return lineTokens.size(); }
    int column(int index) const;
    QStringView tokenText(const Token &token) const;
    void handleTokens();

    bool tryInsideExpression(bool alsoExpression);
    bool tryStatement();

    void defaultOnEnter(FormatTextStatus::StateType newState, int *indentDepth,
                        int *savedIndentDepth) const;

    int indentLine();
    int indentForNewLineAfter() const;
    void recalculateWithIndent(int indent);

    void dump() const;

    QStringView line;
    FormatOptions options;
    FormatTextStatus initialStatus;
    FormatTextStatus currentStatus;
    int indentOffset = 0;
    int currentIndent = 0;
    QList<Token> lineTokens;
    int tokenIndex = 0;
};

QMLDOM_EXPORT int indentForLineStartingWithToken(const FormatTextStatus &oldStatus,
                                                 const FormatOptions &options,
                                                 int token = QQmlJSGrammar::T_ERROR);

QMLDOM_EXPORT FormatPartialStatus formatCodeLine(QStringView line, const FormatOptions &options,
                                                 const FormatTextStatus &initialStatus);

} // namespace Dom
} // namespace QQmlJs
QT_END_NAMESPACE
#endif // QQMLDOMCODEFORMATTER_P_H
