// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLJSASTVISITOR_P_H
#define QQMLJSASTVISITOR_P_H

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

#include "qqmljsastfwd_p.h"
#include "qqmljsglobal_p.h"

QT_BEGIN_NAMESPACE

// as done in https://en.wikipedia.org/wiki/X_Macro

#define QQmlJSASTUiClassListToVisit \
    X(UiProgram)                    \
    X(UiHeaderItemList)             \
    X(UiPragmaValueList)            \
    X(UiPragma)                     \
    X(UiImport)                     \
    X(UiPublicMember)               \
    X(UiSourceElement)              \
    X(UiObjectDefinition)           \
    X(UiObjectInitializer)          \
    X(UiObjectBinding)              \
    X(UiScriptBinding)              \
    X(UiArrayBinding)               \
    X(UiParameterList)              \
    X(UiObjectMemberList)           \
    X(UiArrayMemberList)            \
    X(UiQualifiedId)                \
    X(UiEnumDeclaration)            \
    X(UiEnumMemberList)             \
    X(UiVersionSpecifier)           \
    X(UiInlineComponent)            \
    X(UiAnnotation)                 \
    X(UiAnnotationList)             \
    X(UiRequired)

#define QQmlJSASTQQmlJSClassListToVisit \
    X(TypeExpression)                   \
    X(ThisExpression)                   \
    X(IdentifierExpression)             \
    X(NullExpression)                   \
    X(TrueLiteral)                      \
    X(FalseLiteral)                     \
    X(SuperLiteral)                     \
    X(StringLiteral)                    \
    X(TemplateLiteral)                  \
    X(NumericLiteral)                   \
    X(RegExpLiteral)                    \
    X(ArrayPattern)                     \
    X(ObjectPattern)                    \
    X(PatternElementList)               \
    X(PatternPropertyList)              \
    X(PatternElement)                   \
    X(PatternProperty)                  \
    X(Elision)                          \
    X(NestedExpression)                 \
    X(IdentifierPropertyName)           \
    X(StringLiteralPropertyName)        \
    X(NumericLiteralPropertyName)       \
    X(ComputedPropertyName)             \
    X(ArrayMemberExpression)            \
    X(FieldMemberExpression)            \
    X(TaggedTemplate)                   \
    X(NewMemberExpression)              \
    X(NewExpression)                    \
    X(CallExpression)                   \
    X(ArgumentList)                     \
    X(PostIncrementExpression)          \
    X(PostDecrementExpression)          \
    X(DeleteExpression)                 \
    X(VoidExpression)                   \
    X(TypeOfExpression)                 \
    X(PreIncrementExpression)           \
    X(PreDecrementExpression)           \
    X(UnaryPlusExpression)              \
    X(UnaryMinusExpression)             \
    X(TildeExpression)                  \
    X(NotExpression)                    \
    X(BinaryExpression)                 \
    X(ConditionalExpression)            \
    X(CommaExpression)                  \
    X(Block)                            \
    X(StatementList)                    \
    X(VariableStatement)                \
    X(VariableDeclarationList)          \
    X(EmptyStatement)                   \
    X(ExpressionStatement)              \
    X(IfStatement)                      \
    X(DoWhileStatement)                 \
    X(WhileStatement)                   \
    X(ForStatement)                     \
    X(ForEachStatement)                 \
    X(ContinueStatement)                \
    X(BreakStatement)                   \
    X(ReturnStatement)                  \
    X(YieldExpression)                  \
    X(WithStatement)                    \
    X(SwitchStatement)                  \
    X(CaseBlock)                        \
    X(CaseClauses)                      \
    X(CaseClause)                       \
    X(DefaultClause)                    \
    X(LabelledStatement)                \
    X(ThrowStatement)                   \
    X(TryStatement)                     \
    X(Catch)                            \
    X(Finally)                          \
    X(FunctionDeclaration)              \
    X(FunctionExpression)               \
    X(FormalParameterList)              \
    X(ClassExpression)                  \
    X(ClassDeclaration)                 \
    X(ClassElementList)                 \
    X(Program)                          \
    X(NameSpaceImport)                  \
    X(ImportSpecifier)                  \
    X(ImportsList)                      \
    X(NamedImports)                     \
    X(FromClause)                       \
    X(ImportClause)                     \
    X(ImportDeclaration)                \
    X(ExportSpecifier)                  \
    X(ExportsList)                      \
    X(ExportClause)                     \
    X(ExportDeclaration)                \
    X(ESModule)                         \
    X(DebuggerStatement)                \
    X(Type)                             \
    X(TypeAnnotation)

#define QQmlJSASTClassListToVisit QQmlJSASTUiClassListToVisit QQmlJSASTQQmlJSClassListToVisit

namespace QQmlJS { namespace AST {

class QML_PARSER_EXPORT BaseVisitor
{
public:
    class RecursionDepthCheck
    {
        Q_DISABLE_COPY(RecursionDepthCheck)
    public:
        RecursionDepthCheck(RecursionDepthCheck &&) = delete;
        RecursionDepthCheck &operator=(RecursionDepthCheck &&) = delete;

        RecursionDepthCheck(BaseVisitor *visitor) : m_visitor(visitor)
        {
            ++(m_visitor->m_recursionDepth);
        }

        ~RecursionDepthCheck()
        {
            --(m_visitor->m_recursionDepth);
        }

        bool operator()() const {
            return m_visitor->m_recursionDepth < s_recursionLimit;
        }

    private:
        static const quint16 s_recursionLimit = 4096;
        BaseVisitor *m_visitor;
    };

    BaseVisitor(quint16 parentRecursionDepth = 0);
    virtual ~BaseVisitor();

    virtual bool preVisit(Node *) = 0;
    virtual void postVisit(Node *) = 0;

#define X(name)                     \
    virtual bool visit(name *) = 0; \
    virtual void endVisit(name *) = 0;
    QQmlJSASTClassListToVisit
#undef X

    virtual void throwRecursionDepthError() = 0;

    quint16 recursionDepth() const { return m_recursionDepth; }

protected:
    quint16 m_recursionDepth = 0;
    friend class RecursionDepthCheck;
};

class QML_PARSER_EXPORT Visitor: public BaseVisitor
{
public:
    Visitor(quint16 parentRecursionDepth = 0);

    bool preVisit(Node *) override { return true; }
    void postVisit(Node *) override {}

#define X(name)                                  \
    bool visit(name *) override { return true; } \
    void endVisit(name *) override { }
    QQmlJSASTClassListToVisit
#undef X
};

class QML_PARSER_EXPORT JSVisitor : public BaseVisitor
{
public:
    JSVisitor() = default;

    bool preVisit(Node *) override { return true; }
    void postVisit(Node *) override { }

#define X(name)                                  \
    bool visit(name *) override { return true; } \
    void endVisit(name *) override { }
    QQmlJSASTQQmlJSClassListToVisit
#undef X

#define X(name)                 \
    bool visit(name *) override \
    {                           \
        Q_ASSERT(false);        \
        return false;           \
    }                           \
    void endVisit(name *) override { }
            QQmlJSASTUiClassListToVisit
#undef X
}; // namespace AST
} } // namespace AST

QT_END_NAMESPACE

#endif // QQMLJSASTVISITOR_P_H
