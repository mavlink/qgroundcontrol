// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQMLDOMREFORMATTER_P
#define QQMLDOMREFORMATTER_P

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

#include "qqmldomoutwriter_p.h"
#include "qqmldom_fwd_p.h"
#include "qqmldomcomments_p.h"
#include "qqmldomelements_p.h"

#include <QtQml/private/qqmljsast_p.h>

QT_BEGIN_NAMESPACE
namespace QQmlJS {
namespace Dom {

class ScriptFormatter final : protected AST::JSVisitor
{
public:
    // TODO QTBUG-121988
    ScriptFormatter(OutWriter &lw, const ScriptExpression *const script) : lw(lw), m_script(script)
    {
        if (m_script) {
            comments = m_script->astComments();
            accept(m_script->ast());
        }
    }

protected:
    inline void out(const char *str) { lw.write(QString::fromLatin1(str)); }
    inline void out(QStringView str) { lw.write(str); }
    inline void out(const SourceLocation &loc)
    {
        if (loc.length != 0)
            out(m_script->loc2Str(loc));
    }
    enum CommentOption { NoSpace, SpaceBeforePostComment, OnlyComments };
    void outWithComments(const SourceLocation &loc, AST::Node *node, CommentOption option = NoSpace)
    {
        if (!loc.isValid())
            return;
        const CommentedElement *c = comments->commentForNode(node, CommentAnchor::from(loc));
        if (c)
            c->writePre(lw);
        if (option != OnlyComments)
            out(loc);
        if (option == SpaceBeforePostComment)
            lw.ensureSpace();
        if (c)
            c->writePost(lw);
    }

    inline void newLine(quint32 count = 1) { lw.ensureNewline(count); }
    inline void accept(AST::Node *node) { AST::Node::accept(node, this); }
    void lnAcceptIndented(AST::Node *node);
    bool acceptBlockOrIndented(AST::Node *ast, bool finishWithSpaceOrNewline = false);

    bool preVisit(AST::Node *n) override;
    void postVisit(AST::Node *n) override;

    bool visit(AST::ThisExpression *ast) override;
    bool visit(AST::NullExpression *ast) override;
    bool visit(AST::TrueLiteral *ast) override;
    bool visit(AST::FalseLiteral *ast) override;
    bool visit(AST::IdentifierExpression *ast) override;
    bool visit(AST::StringLiteral *ast) override;
    bool visit(AST::NumericLiteral *ast) override;
    bool visit(AST::RegExpLiteral *ast) override;

    bool visit(AST::ArrayPattern *ast) override;

    bool visit(AST::ObjectPattern *ast) override;

    bool visit(AST::PatternElementList *ast) override;

    bool visit(AST::PatternPropertyList *ast) override;
    bool visit(AST::PatternProperty *property) override;

    bool visit(AST::NestedExpression *ast) override;
    bool visit(AST::IdentifierPropertyName *ast) override;
    bool visit(AST::StringLiteralPropertyName *ast) override;
    bool visit(AST::NumericLiteralPropertyName *ast) override;

    bool visit(AST::TemplateLiteral *ast) override;
    bool visit(AST::ArrayMemberExpression *ast) override;

    bool visit(AST::FieldMemberExpression *ast) override;

    bool visit(AST::NewMemberExpression *ast) override;

    bool visit(AST::NewExpression *ast) override;

    bool visit(AST::CallExpression *ast) override;

    bool visit(AST::PostIncrementExpression *ast) override;

    bool visit(AST::PostDecrementExpression *ast) override;
    bool visit(AST::PreIncrementExpression *ast) override;

    bool visit(AST::PreDecrementExpression *ast) override;

    bool visit(AST::DeleteExpression *ast) override;

    bool visit(AST::VoidExpression *ast) override;
    bool visit(AST::TypeOfExpression *ast) override;

    bool visit(AST::UnaryPlusExpression *ast) override;

    bool visit(AST::UnaryMinusExpression *ast) override;

    bool visit(AST::TildeExpression *ast) override;

    bool visit(AST::NotExpression *ast) override;

    bool visit(AST::BinaryExpression *ast) override;

    bool visit(AST::ConditionalExpression *ast) override;

    bool visit(AST::Block *ast) override;

    bool visit(AST::VariableStatement *ast) override;

    bool visit(AST::PatternElement *ast) override;
    bool visit(AST::TypeAnnotation *ast) override;
    bool visit(AST::Type *ast) override;
    bool visit(AST::UiQualifiedId *ast) override;

    bool visit(AST::EmptyStatement *ast) override;

    bool visit(AST::IfStatement *ast) override;
    bool visit(AST::DoWhileStatement *ast) override;

    bool visit(AST::WhileStatement *ast) override;

    bool visit(AST::ForStatement *ast) override;

    bool visit(AST::ForEachStatement *ast) override;

    bool visit(AST::ContinueStatement *ast) override;
    bool visit(AST::BreakStatement *ast) override;

    bool visit(AST::ReturnStatement *ast) override;
    bool visit(AST::YieldExpression *ast) override;
    bool visit(AST::ThrowStatement *ast) override;
    bool visit(AST::WithStatement *ast) override;

    bool visit(AST::SwitchStatement *ast) override;

    bool visit(AST::CaseBlock *ast) override;

    bool visit(AST::CaseClause *ast) override;

    bool visit(AST::DefaultClause *ast) override;

    bool visit(AST::LabelledStatement *ast) override;

    bool visit(AST::TryStatement *ast) override;

    bool visit(AST::Catch *ast) override;

    bool visit(AST::Finally *ast) override;

    bool visit(AST::FunctionDeclaration *ast) override;

    bool visit(AST::FunctionExpression *ast) override;

    bool visit(AST::Elision *ast) override;

    bool visit(AST::ArgumentList *ast) override;

    bool visit(AST::StatementList *ast) override;

    bool visit(AST::VariableDeclarationList *ast) override;

    bool visit(AST::CaseClauses *ast) override;

    bool visit(AST::FormalParameterList *ast) override;

    bool visit(AST::SuperLiteral *) override;
    bool visit(AST::ComputedPropertyName *) override;
    bool visit(AST::CommaExpression *el) override;
    bool visit(AST::ExpressionStatement *el) override;

    bool visit(AST::ClassDeclaration *ast) override;

    bool visit(AST::ImportDeclaration *ast) override;
    bool visit(AST::ImportSpecifier *ast) override;
    bool visit(AST::NameSpaceImport *ast) override;
    bool visit(AST::ImportsList *ast) override;
    bool visit(AST::NamedImports *ast) override;
    bool visit(AST::ImportClause *ast) override;

    bool visit(AST::ExportDeclaration *ast) override;
    bool visit(AST::ExportClause *ast) override;
    bool visit(AST::ExportSpecifier *ast) override;
    bool visit(AST::ExportsList *ast) override;

    bool visit(AST::FromClause *ast) override;

    void endVisit(AST::ComputedPropertyName *) override;

    void endVisit(AST::ExportDeclaration *ast) override;
    void endVisit(AST::ExportClause *ast) override;

    void endVisit(AST::ImportDeclaration *ast) override;
    void endVisit(AST::NamedImports *ast) override;

    void throwRecursionDepthError() override;

private:
    bool addSemicolons() const { return expressionDepth > 0; }
    bool canRemoveSemicolon(AST::Node *node);
    OutWriter &writeOutSemicolon(AST::Node *);
    OutWriter &lw;
    std::shared_ptr<AstComments> comments;
    const ScriptExpression *const m_script = nullptr; // outlives this
    QHash<AST::Node *, QList<std::function<void()>>> postOps;
    int expressionDepth = 0;
};

QMLDOM_EXPORT void reformatAst(OutWriter &lw, const QQmlJS::Dom::ScriptExpression *const script);

} // namespace Dom
} // namespace QQmlJS
QT_END_NAMESPACE

#endif // QQMLDOMREFORMATTER_P
