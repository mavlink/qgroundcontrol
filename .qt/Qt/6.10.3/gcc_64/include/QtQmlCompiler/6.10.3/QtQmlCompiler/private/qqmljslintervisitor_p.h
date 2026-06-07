// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
// Qt-Security score:significant

#ifndef QQMLJSLINTERVISITOR_P_H
#define QQMLJSLINTERVISITOR_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.

#include <private/qqmljsimportvisitor_p.h>

#include <private/qqmljsengine_p.h>

QT_BEGIN_NAMESPACE

namespace QQmlJS {

/*!
   \internal
    Extends QQmlJSImportVisitor with extra warnings that are required for linting but unrelated to
   QQmlJSImportVisitor actual task that is constructing QQmlJSScopes. One example of such warnings
   are purely syntactic checks, or warnings that don't affect compilation.
 */
class LinterVisitor final : public QQmlJSImportVisitor
{
public:
    LinterVisitor(const QQmlJSScope::Ptr &target, QQmlJSImporter *importer, QQmlJSLogger *logger,
                  const QString &implicitImportDirectory,
                  const QStringList &qmldirFiles = QStringList(), QQmlJS::Engine *engine = nullptr);

protected:
    using QQmlJSImportVisitor::endVisit;
    using QQmlJSImportVisitor::visit;

    bool preVisit(QQmlJS::AST::Node *) override;
    void postVisit(QQmlJS::AST::Node *) override;
    QQmlJS::AST::Node *astParentOfVisitedNode() const;

    void leaveEnvironment() override;

    bool visit(QQmlJS::AST::StringLiteral *) override;
    bool visit(AST::CommaExpression *) override;
    bool visit(QQmlJS::AST::NewMemberExpression *) override;
    bool visit(QQmlJS::AST::VoidExpression *ast) override;
    bool visit(QQmlJS::AST::BinaryExpression *) override;
    bool visit(QQmlJS::AST::UiImport *import) override;
    bool visit(QQmlJS::AST::UiEnumDeclaration *uied) override;
    bool visit(QQmlJS::AST::CaseBlock *) override;
    bool visit(QQmlJS::AST::ExpressionStatement *ast) override;

private:
    struct SeenImport
    {
        QStringView filename;
        QString uri;
        QTypeRevision version;
        QStringView id;
        QQmlJS::AST::UiImport *uiImport;

        SeenImport(QQmlJS::AST::UiImport *i) : filename(i->fileName), id(i->importId), uiImport(i)
        {
            if (i->importUri)
                uri = i->importUri->toString();
            if (i->version)
                version = i->version->version;
        }
        friend bool comparesEqual(const SeenImport &lhs, const SeenImport &rhs) noexcept
        {
            return lhs.filename == rhs.filename && lhs.uri == rhs.uri
                    && lhs.version == rhs.version && lhs.id == rhs.id;
        }
        Q_DECLARE_EQUALITY_COMPARABLE(SeenImport)

        friend size_t qHash(const SeenImport &i, size_t seed = 0)
        {
            return qHashMulti(seed, i.filename, i.uri, i.version, i.id);
        }
    };
    QQmlJS::Engine *m_engine = nullptr;
    QSet<SeenImport> m_seenImports;
    std::vector<QQmlJS::AST::Node *> m_ancestryIncludingCurrentNode;

    void handleDuplicateEnums(QQmlJS::AST::UiEnumMemberList *members, QStringView key,
                              const QQmlJS::SourceLocation &location);
    void warnCaseNoFlowControl(QQmlJS::SourceLocation caseToken) const;
    void checkCaseFallthrough(QQmlJS::AST::StatementList *statements, SourceLocation errorLoc,
                              SourceLocation nextLoc);
    BindingExpressionParseResult parseBindingExpression(
            const QString &name, const QQmlJS::AST::Statement *statement,
            const QQmlJS::AST::UiPublicMember *associatedPropertyDefinition = nullptr) override;
    void handleLiteralBinding(const QQmlJSMetaPropertyBinding &binding,
                              const AST::UiPublicMember *associatedPropertyDefinition) override;
};

} // namespace QQmlJS

QT_END_NAMESPACE

#endif // QQMLJSLINTERVISITOR_P_H
