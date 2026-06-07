// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQMLLSCOMPLETION_H
#define QQMLLSCOMPLETION_H

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

#include "qqmlcompletioncontextstrings_p.h"
#include "qqmllsutils_p.h"
#include "qqmllsplugin_p.h"

#include <QtLanguageServer/private/qlanguageserverspectypes_p.h>
#include <QtQmlDom/private/qqmldomexternalitems_p.h>
#include <QtQmlDom/private/qqmldomtop_p.h>
#include <QtCore/private/qduplicatetracker_p.h>
#include <QtCore/private/qfactoryloader_p.h>
#include <QtCore/qpluginloader.h>
#include <QtCore/qxpfunctional.h>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(QQmlLSCompletionLog)


class QQmlLSCompletion
{
    using DomItem = QQmlJS::Dom::DomItem;
public:
    enum class ImportCompletionType { None, Module, Version };
    enum AppendOption { AppendSemicolon, AppendNothing };

    QQmlLSCompletion(const QFactoryLoader &pluginLoader);

    using CompletionItem = QLspSpecification::CompletionItem;
    using BackInsertIterator = std::back_insert_iterator<QList<CompletionItem>>;
    QList<CompletionItem> completions(const DomItem &currentItem,
                                      const CompletionContextStrings &ctx) const;

    static CompletionItem makeSnippet(QUtf8StringView qualifier, QUtf8StringView label,
                                      QUtf8StringView insertText);

    static CompletionItem makeSnippet(QUtf8StringView label, QUtf8StringView insertText);

private:
    struct QQmlLSCompletionPosition
    {
        DomItem itemAtPosition;
        CompletionContextStrings cursorPosition;
        qsizetype offset() const { return cursorPosition.offset(); }
    };

    void collectCompletions(const DomItem &currentItem, const CompletionContextStrings &ctx,
                            BackInsertIterator result) const;

    bool betweenLocations(QQmlJS::SourceLocation left, const QQmlLSCompletionPosition &positionInfo,
                          QQmlJS::SourceLocation right) const;
    bool afterLocation(QQmlJS::SourceLocation left,
                       const QQmlLSCompletionPosition &positionInfo) const;
    bool beforeLocation(const QQmlLSCompletionPosition &ctx, QQmlJS::SourceLocation right) const;
    bool ctxBeforeStatement(const QQmlLSCompletionPosition &positionInfo,
                            const DomItem &parentForContext,
                            QQmlJS::Dom::FileLocationRegion firstRegion) const;
    bool isCaseOrDefaultBeforeCtx(const DomItem &currentClause,
                                  const QQmlLSCompletionPosition &positionInfo,
                                  QQmlJS::Dom::FileLocationRegion keywordRegion) const;
    DomItem previousCaseOfCaseBlock(const DomItem &parentForContext,
                                    const QQmlLSCompletionPosition &positionInfo) const;

    void idsCompletions(const DomItem &component, BackInsertIterator it) const;

    void suggestReachableTypes(const DomItem &context,
                               QQmlJS::Dom::LocalSymbolsTypes typeCompletionType,
                               QLspSpecification::CompletionItemKind kind,
                               BackInsertIterator it) const;

    void suggestJSStatementCompletion(const DomItem &currentItem, BackInsertIterator it) const;
    void suggestCaseAndDefaultStatementCompletion(BackInsertIterator it) const;
    void suggestVariableDeclarationStatementCompletion(
            BackInsertIterator it, AppendOption option = AppendSemicolon) const;

    void suggestEnumerationsAndEnumerationValues(const QQmlJSScope::ConstPtr &scope,
                                                 const QString &enumName,
                                                 QDuplicateTracker<QString> &usedNames,
                                                 BackInsertIterator result) const;
    DomItem ownerOfQualifiedExpression(const DomItem &qualifiedExpression) const;
    void suggestJSExpressionCompletion(const DomItem &context, BackInsertIterator it) const;

    void suggestBindingCompletion(const DomItem &itemAtPosition, BackInsertIterator it) const;

    void insideImportCompletionHelper(const DomItem &file,
                                      const QQmlLSCompletionPosition &positionInfo,
                                      BackInsertIterator it) const;

    void jsIdentifierCompletion(const QQmlJSScope::ConstPtr &scope,
                                QDuplicateTracker<QString> *usedNames, BackInsertIterator it) const;

    void methodCompletion(const QQmlJSScope::ConstPtr &scope, QDuplicateTracker<QString> *usedNames,
                          BackInsertIterator it) const;
    void propertyCompletion(const QQmlJSScope::ConstPtr &scope,
                            QDuplicateTracker<QString> *usedNames, BackInsertIterator it) const;
    void enumerationCompletion(const QQmlJSScope::ConstPtr &scope,
                               QDuplicateTracker<QString> *usedNames, BackInsertIterator it) const;
    void enumerationValueCompletionHelper(const QStringList &enumeratorKeys,
                                          BackInsertIterator it) const;

    void enumerationValueCompletion(const QQmlJSScope::ConstPtr &scope,
                                    const QString &enumeratorName, BackInsertIterator it) const;

    static bool cursorInFrontOfItem(const DomItem &parentForContext,
                                    const QQmlLSCompletionPosition &positionInfo);
    static bool cursorAfterColon(const DomItem &currentItem,
                                 const QQmlLSCompletionPosition &positionInfo);
    void insidePragmaCompletion(QQmlJS::Dom::DomItem currentItem,
                                const QQmlLSCompletionPosition &positionInfo,
                                BackInsertIterator it) const;
    void insideQmlObjectCompletion(const DomItem &parentForContext,
                                   const QQmlLSCompletionPosition &positionInfo,
                                   BackInsertIterator it) const;
    void insidePropertyDefinitionCompletion(const DomItem &currentItem,
                                            const QQmlLSCompletionPosition &positionInfo,
                                            BackInsertIterator it) const;
    void insideBindingCompletion(const DomItem &currentItem,
                                 const QQmlLSCompletionPosition &positionInfo,
                                 BackInsertIterator it) const;
    void insideImportCompletion(const DomItem &currentItem,
                                const QQmlLSCompletionPosition &positionInfo,
                                BackInsertIterator it) const;
    void insideQmlFileCompletion(const DomItem &currentItem,
                                 const QQmlLSCompletionPosition &positionInfo,
                                 BackInsertIterator it) const;
    void suggestContinueAndBreakStatementIfNeeded(const DomItem &itemAtPosition,
                                                  BackInsertIterator it) const;
    void insideScriptLiteralCompletion(const DomItem &currentItem,
                                       const QQmlLSCompletionPosition &positionInfo,
                                       BackInsertIterator it) const;
    void insideCallExpression(const DomItem &currentItem,
                              const QQmlLSCompletionPosition &positionInfo,
                              BackInsertIterator it) const;
    void insideIfStatement(const DomItem &currentItem, const QQmlLSCompletionPosition &positionInfo,
                           BackInsertIterator it) const;
    void insideReturnStatement(const DomItem &currentItem,
                               const QQmlLSCompletionPosition &positionInfo,
                               BackInsertIterator it) const;
    void insideWhileStatement(const DomItem &currentItem,
                              const QQmlLSCompletionPosition &positionInfo,
                              BackInsertIterator it) const;
    void insideDoWhileStatement(const DomItem &parentForContext,
                                const QQmlLSCompletionPosition &positionInfo,
                                BackInsertIterator it) const;
    void insideForStatementCompletion(const DomItem &parentForContext,
                                      const QQmlLSCompletionPosition &positionInfo,
                                      BackInsertIterator it) const;
    void insideForEachStatement(const DomItem &parentForContext,
                                const QQmlLSCompletionPosition &positionInfo,
                                BackInsertIterator it) const;
    void insideSwitchStatement(const DomItem &parentForContext,
                               const QQmlLSCompletionPosition positionInfo,
                               BackInsertIterator it) const;
    void insideCaseClause(const DomItem &parentForContext,
                          const QQmlLSCompletionPosition &positionInfo,
                          BackInsertIterator it) const;
    void insideCaseBlock(const DomItem &parentForContext,
                         const QQmlLSCompletionPosition &positionInfo, BackInsertIterator it) const;
    void insideDefaultClause(const DomItem &parentForContext,
                             const QQmlLSCompletionPosition &positionInfo,
                             BackInsertIterator it) const;
    void insideBinaryExpressionCompletion(const DomItem &parentForContext,
                                          const QQmlLSCompletionPosition &positionInfo,
                                          BackInsertIterator it) const;
    void insideScriptPattern(const DomItem &parentForContext,
                             const QQmlLSCompletionPosition &positionInfo,
                             BackInsertIterator it) const;
    void insideVariableDeclarationEntry(const DomItem &parentForContext,
                                        const QQmlLSCompletionPosition &positionInfo,
                                        BackInsertIterator it) const;
    void insideThrowStatement(const DomItem &parentForContext,
                              const QQmlLSCompletionPosition &positionInfo,
                              BackInsertIterator it) const;
    void insideLabelledStatement(const DomItem &parentForContext,
                                 const QQmlLSCompletionPosition &positionInfo,
                                 BackInsertIterator it) const;
    void insideContinueStatement(const DomItem &parentForContext,
                                 const QQmlLSCompletionPosition &positionInfo,
                                 BackInsertIterator it) const;
    void insideBreakStatement(const DomItem &parentForContext,
                              const QQmlLSCompletionPosition &positionInfo,
                              BackInsertIterator it) const;
    void insideConditionalExpression(const DomItem &parentForContext,
                                     const QQmlLSCompletionPosition &positionInfo,
                                     BackInsertIterator it) const;
    void insideUnaryExpression(const DomItem &parentForContext,
                               const QQmlLSCompletionPosition &positionInfo,
                               BackInsertIterator it) const;
    void insidePostExpression(const DomItem &parentForContext,
                              const QQmlLSCompletionPosition &positionInfo,
                              BackInsertIterator it) const;
    void insideParenthesizedExpression(const DomItem &parentForContext,
                                       const QQmlLSCompletionPosition &positionInfo,
                                       BackInsertIterator it) const;
    void insideTemplateLiteral(const DomItem &parentForContext,
                               const QQmlLSCompletionPosition &positionInfo,
                               BackInsertIterator it) const;
    void insideNewExpression(const DomItem &parentForContext,
                             const QQmlLSCompletionPosition &positionInfo,
                             BackInsertIterator it) const;
    void insideNewMemberExpression(const DomItem &parentForContext,
                                   const QQmlLSCompletionPosition &positionInfo,
                                   BackInsertIterator it) const;
    void signalHandlerCompletion(const QQmlJSScope::ConstPtr &scope,
                                 QDuplicateTracker<QString> *usedNames,
                                 BackInsertIterator it) const;

    void suggestSnippetsForLeftHandSideOfBinding(const DomItem &items,
                                                 BackInsertIterator result) const;

    void suggestSnippetsForRightHandSideOfBinding(const DomItem &items,
                                                  BackInsertIterator result) const;

private:
    using CompletionFromPluginFunction = void(QQmlLSCompletionPlugin *plugin,
                                              BackInsertIterator result);
    void collectFromPlugins(const qxp::function_ref<CompletionFromPluginFunction> f,
                            BackInsertIterator result) const;

    QStringList m_loadPaths;

    std::vector<std::unique_ptr<QQmlLSCompletionPlugin>> m_plugins;
};

QT_END_NAMESPACE

#endif // QQMLLSCOMPLETION_H
