// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQMLSEMANTICTOKENS_P_H
#define QQMLSEMANTICTOKENS_P_H

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

#include <QtLanguageServer/private/qlanguageserverspec_p.h>
#include <QtQmlDom/private/qqmldomitem_p.h>
#include <QtCore/qlist.h>
#include <QtCore/qmap.h>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(semanticTokens)

namespace QmlHighlighting {
Q_NAMESPACE

// Protocol agnostic highlighting kinds
// Use this enum while visiting dom tree to define the highlighting kinds for the semantic tokens
// Then map it to the protocol specific token types and modifiers
// This can be as much as detailed as needed
enum class QmlHighlightKind {
    QmlKeyword, // Qml keyword
    QmlType, // Qml type name
    QmlImportId, // Qml import module name
    QmlNamespace, // Qml module namespace, i.e import QtQuick as Namespace
    QmlLocalId, // Object id within the same file
    QmlExternalId, // Object id defined in another file. [UNUSED FOR NOW]
    QmlProperty, // Qml property. For now used for all kind of properties
    QmlScopeObjectProperty, // Qml property defined in the current scope
    QmlRootObjectProperty, // Qml property defined in the parent scopes
    QmlExternalObjectProperty, // Qml property defined in the root object of another file
    QmlMethod,
    QmlMethodParameter,
    QmlSignal,
    QmlSignalHandler,
    QmlEnumName, // Enum type name
    QmlEnumMember, // Enum field names
    QmlPragmaName, // Qml pragma name
    QmlPragmaValue, // Qml pragma value
    QmlTypeModifier, // list<QtObject>, list is the modifier, QtObject is the type
    JsImport, // Js imported name
    JsGlobalVar, // Js global variable or objects
    JsGlobalMethod, // Js global method
    JsScopeVar, // Js variable defined in the current scope
    JsLabel, // js label
    Number,
    String,
    Comment,
    Operator,
    Field, // Used for the field names in the property chains,
    Unknown, // Used for the unknown tokens
};

enum class QmlHighlightModifier {
    None = 0,
    QmlPropertyDefinition = 1 << 0,
    QmlDefaultProperty = 1 << 1,
    QmlFinalProperty = 1 << 2,
    QmlRequiredProperty = 1 << 3,
    QmlReadonlyProperty = 1 << 4,
};
Q_DECLARE_FLAGS(QmlHighlightModifiers, QmlHighlightModifier)
Q_DECLARE_OPERATORS_FOR_FLAGS(QmlHighlightModifiers)

enum class HighlightingMode { Default, QtCHighlighting };

// Protocol specific token types
// The values in this enum are converted to relevant strings and sent to the client as server
// capabilities The convention is that the first letter in the enum value is decapitalized and the
// rest is unchanged i.e Namespace -> "namespace" This is handled in enumToByteArray() helper
// function.
enum class SemanticTokenProtocolTypes {
    // Subset of the QLspSpefication::SemanticTokenTypes enum
    // We register only the token types used in the qml semantic highlighting
    Namespace,
    Type,
    Enum,
    Parameter,
    Variable,
    Property,
    EnumMember,
    Method,
    Keyword,
    Comment,
    String,
    Number,
    Regexp,
    Operator,
    Decorator,

    // Additional token types for the extended semantic highlighting
    QmlLocalId, // object id within the same file
    QmlExternalId, // object id defined in another file
    QmlRootObjectProperty, // qml property defined in the parent scopes
    QmlScopeObjectProperty, // qml property defined in the current scope
    QmlExternalObjectProperty, // qml property defined in the root object of another file
    JsScopeVar, // js variable defined in the current file
    JsImportVar, // js import name that is imported in the qml file
    JsGlobalVar, // js global variables
    QmlStateName, // name of a qml state
    Field, // used for the field names in the property chains
    Unknown, // used for the unknown contexts
};
Q_ENUM_NS(SemanticTokenProtocolTypes)

// Represents a semantic highlighting token
// startLine and startColumn are 0-based as in LSP spec.
struct HighlightToken
{
    HighlightToken() = default;
    HighlightToken(const QQmlJS::SourceLocation &loc, QmlHighlightKind,
                   QmlHighlightModifiers = QmlHighlightModifier::None);

    inline friend bool operator==(const HighlightToken &lhs, const HighlightToken &rhs)
    {
        return lhs.loc == rhs.loc  && lhs.kind == rhs.kind && lhs.modifiers == rhs.modifiers;
    }

    QQmlJS::SourceLocation loc;
    QmlHighlightKind kind;
    QmlHighlightModifiers modifiers;
};

using HighlightsContainer = QMap<int, HighlightToken>;
using QmlHighlightKindToLspKind = int (*)(QmlHighlightKind);

/*!
\internal
Offsets start from zero.
*/
struct HighlightsRange
{
    int startOffset;
    int endOffset;
};

namespace Utils
{
QList<int> encodeSemanticTokens(const HighlightsContainer &highlights, HighlightingMode mode = HighlightingMode::Default);
QList<QQmlJS::SourceLocation>
sourceLocationsFromMultiLineToken(QStringView code,
                                    const QQmlJS::SourceLocation &tokenLocation);
void addModifier(QLspSpecification::SemanticTokenModifiers modifier, int *baseModifier);
bool rangeOverlapsWithSourceLocation(const QQmlJS::SourceLocation &loc, const HighlightsRange &r);
QList<QLspSpecification::SemanticTokensEdit> computeDiff(const QList<int> &, const QList<int> &);
void updateResultID(QByteArray &resultID);
QList<int> collectTokens(const QQmlJS::Dom::DomItem &item,
                            const std::optional<HighlightsRange> &range,
                            HighlightingMode mode = HighlightingMode::Default);
HighlightsContainer visitTokens(const QQmlJS::Dom::DomItem &item,
                                const std::optional<HighlightsRange> &range);
void addHighlight(HighlightsContainer &out, const QQmlJS::SourceLocation &loc, QmlHighlightKind,
                    QmlHighlightModifiers = QmlHighlightModifier::None);
} // namespace Utils

class HighlightingVisitor
{
public:
    HighlightingVisitor(const QQmlJS::Dom::DomItem &item,
                        const std::optional<HighlightsRange> &range);
    const HighlightsContainer &hightights() const { return m_highlights; }
    HighlightsContainer &highlights() { return m_highlights; }

private:
    bool visitor(QQmlJS::Dom::Path, const QQmlJS::Dom::DomItem &item, bool);
    void highlightComment(const QQmlJS::Dom::DomItem &item);
    void highlightImport(const QQmlJS::Dom::DomItem &item);
    void highlightBinding(const QQmlJS::Dom::DomItem &item);
    void highlightPragma(const QQmlJS::Dom::DomItem &item);
    void highlightEnumItem(const QQmlJS::Dom::DomItem &item);
    void highlightEnumDecl(const QQmlJS::Dom::DomItem &item);
    void highlightQmlObject(const QQmlJS::Dom::DomItem &item);
    void highlightComponent(const QQmlJS::Dom::DomItem &item);
    void highlightPropertyDefinition(const QQmlJS::Dom::DomItem &item);
    void highlightMethod(const QQmlJS::Dom::DomItem &item);
    void highlightScriptLiteral(const QQmlJS::Dom::DomItem &item);
    void highlightIdentifier(const QQmlJS::Dom::DomItem &item);
    void highlightBySemanticAnalysis(const QQmlJS::Dom::DomItem &item, QQmlJS::SourceLocation loc);
    void highlightScriptExpressions(const QQmlJS::Dom::DomItem &item);
    void highlightCallExpression(const QQmlJS::Dom::DomItem &item);
    void highlightFieldMemberAccess(const QQmlJS::Dom::DomItem &item, QQmlJS::SourceLocation loc);
    void addHighlight(const QQmlJS::SourceLocation &loc, QmlHighlightKind,
                      QmlHighlightModifiers = QmlHighlightModifier::None);
private:
    HighlightsContainer m_highlights;
    std::optional<HighlightsRange> m_range;
};

} // namespace QmlHighlighting

QT_END_NAMESPACE

#endif // QQMLSEMANTICTOKENS_P_H
