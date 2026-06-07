// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
// Qt-Security score:significant

#ifndef QQMLJSLOGGER_H
#define QQMLJSLOGGER_H

#include <QtCore/qanystringview.h>
#include <QtQmlCompiler/qtqmlcompilerexports.h>

QT_BEGIN_NAMESPACE

class QQmlJSLoggerPrivate;
class QQmlJSScope;

namespace QQmlSA {
class SourceLocation;
}

namespace QQmlSA {

class Q_QMLCOMPILER_EXPORT LoggerWarningId
{
public:
    constexpr LoggerWarningId(QAnyStringView name) : m_name(name) { }

    QAnyStringView name() const { return m_name; }

private:
    friend bool operator==(const LoggerWarningId &a, const LoggerWarningId &b)
    {
        return a.m_name == b.m_name;
    }

    friend bool operator!=(const LoggerWarningId &a, const LoggerWarningId &b)
    {
        return a.m_name != b.m_name;
    }
    const QAnyStringView m_name;
};

} // namespace QQmlSA

extern const Q_QMLCOMPILER_EXPORT QQmlSA::LoggerWarningId qmlAccessSingleton;
extern const Q_QMLCOMPILER_EXPORT QQmlSA::LoggerWarningId qmlAliasCycle;
extern const Q_QMLCOMPILER_EXPORT QQmlSA::LoggerWarningId qmlAssignmentInCondition;
extern const Q_QMLCOMPILER_EXPORT QQmlSA::LoggerWarningId qmlAttachedPropertyReuse;
extern const Q_QMLCOMPILER_EXPORT QQmlSA::LoggerWarningId qmlComma;
extern const Q_QMLCOMPILER_EXPORT QQmlSA::LoggerWarningId qmlCompiler;
extern const Q_QMLCOMPILER_EXPORT QQmlSA::LoggerWarningId qmlComponentChildrenCount;
extern const Q_QMLCOMPILER_EXPORT QQmlSA::LoggerWarningId qmlConfusingExpressionStatement;
extern const Q_QMLCOMPILER_EXPORT QQmlSA::LoggerWarningId qmlConfusingMinuses;
extern const Q_QMLCOMPILER_EXPORT QQmlSA::LoggerWarningId qmlConfusingPluses;
extern const Q_QMLCOMPILER_EXPORT QQmlSA::LoggerWarningId qmlContextProperties;
extern const Q_QMLCOMPILER_EXPORT QQmlSA::LoggerWarningId qmlDeferredPropertyId;
extern const Q_QMLCOMPILER_EXPORT QQmlSA::LoggerWarningId qmlDeprecated;
extern const Q_QMLCOMPILER_EXPORT QQmlSA::LoggerWarningId qmlDuplicateEnumEntries;
extern const Q_QMLCOMPILER_EXPORT QQmlSA::LoggerWarningId qmlDuplicateImport;
extern const Q_QMLCOMPILER_EXPORT QQmlSA::LoggerWarningId qmlDuplicateInlineComponent;
extern const Q_QMLCOMPILER_EXPORT QQmlSA::LoggerWarningId qmlDuplicatePropertyBinding;
extern const Q_QMLCOMPILER_EXPORT QQmlSA::LoggerWarningId qmlDuplicatedName;
extern const Q_QMLCOMPILER_EXPORT QQmlSA::LoggerWarningId qmlEnumEntryMatchesEnum;
extern const Q_QMLCOMPILER_EXPORT QQmlSA::LoggerWarningId qmlEnumsAreNotTypes;
extern const Q_QMLCOMPILER_EXPORT QQmlSA::LoggerWarningId qmlEqualityTypeCoercion;
extern const Q_QMLCOMPILER_EXPORT QQmlSA::LoggerWarningId qmlEval;
extern const Q_QMLCOMPILER_EXPORT QQmlSA::LoggerWarningId qmlFunctionUsedBeforeDeclaration;
extern const Q_QMLCOMPILER_EXPORT QQmlSA::LoggerWarningId qmlImport;
extern const Q_QMLCOMPILER_EXPORT QQmlSA::LoggerWarningId qmlImportFileSelector;
extern const Q_QMLCOMPILER_EXPORT QQmlSA::LoggerWarningId qmlIncompatibleType;
extern const Q_QMLCOMPILER_EXPORT QQmlSA::LoggerWarningId qmlInheritanceCycle;
extern const Q_QMLCOMPILER_EXPORT QQmlSA::LoggerWarningId qmlInvalidLintDirective;
extern const Q_QMLCOMPILER_EXPORT QQmlSA::LoggerWarningId qmlLiteralConstructor;
extern const Q_QMLCOMPILER_EXPORT QQmlSA::LoggerWarningId qmlMissingEnumEntry;
extern const Q_QMLCOMPILER_EXPORT QQmlSA::LoggerWarningId qmlMissingProperty;
extern const Q_QMLCOMPILER_EXPORT QQmlSA::LoggerWarningId qmlMissingType;
extern const Q_QMLCOMPILER_EXPORT QQmlSA::LoggerWarningId qmlMultilineStrings;
extern const Q_QMLCOMPILER_EXPORT QQmlSA::LoggerWarningId qmlNonListProperty;
extern const Q_QMLCOMPILER_EXPORT QQmlSA::LoggerWarningId qmlNonRootEnums;
extern const Q_QMLCOMPILER_EXPORT QQmlSA::LoggerWarningId qmlPlugin;
extern const Q_QMLCOMPILER_EXPORT QQmlSA::LoggerWarningId qmlPreferNonVarProperties;
extern const Q_QMLCOMPILER_EXPORT QQmlSA::LoggerWarningId qmlPrefixedImportType;
extern const Q_QMLCOMPILER_EXPORT QQmlSA::LoggerWarningId qmlReadOnlyProperty;
extern const Q_QMLCOMPILER_EXPORT QQmlSA::LoggerWarningId qmlRecursionDepthErrors;
extern const Q_QMLCOMPILER_EXPORT QQmlSA::LoggerWarningId qmlRedundantOptionalChaining;
extern const Q_QMLCOMPILER_EXPORT QQmlSA::LoggerWarningId qmlRequired;
extern const Q_QMLCOMPILER_EXPORT QQmlSA::LoggerWarningId qmlRestrictedType;
extern const Q_QMLCOMPILER_EXPORT QQmlSA::LoggerWarningId qmlSignalParameters;
extern const Q_QMLCOMPILER_EXPORT QQmlSA::LoggerWarningId qmlStalePropertyRead;
extern const Q_QMLCOMPILER_EXPORT QQmlSA::LoggerWarningId qmlSyntax;
extern const Q_QMLCOMPILER_EXPORT QQmlSA::LoggerWarningId qmlSyntaxDuplicateIds;
extern const Q_QMLCOMPILER_EXPORT QQmlSA::LoggerWarningId qmlSyntaxIdQuotation;
extern const Q_QMLCOMPILER_EXPORT QQmlSA::LoggerWarningId qmlTopLevelComponent;
extern const Q_QMLCOMPILER_EXPORT QQmlSA::LoggerWarningId qmlTranslationFunctionMismatch;
extern const Q_QMLCOMPILER_EXPORT QQmlSA::LoggerWarningId qmlUncreatableType;
extern const Q_QMLCOMPILER_EXPORT QQmlSA::LoggerWarningId qmlUnintentionalEmptyBlock;
extern const Q_QMLCOMPILER_EXPORT QQmlSA::LoggerWarningId qmlUnqualified;
extern const Q_QMLCOMPILER_EXPORT QQmlSA::LoggerWarningId qmlUnreachableCode;
extern const Q_QMLCOMPILER_EXPORT QQmlSA::LoggerWarningId qmlUnresolvedAlias;
extern const Q_QMLCOMPILER_EXPORT QQmlSA::LoggerWarningId qmlUnresolvedType;
extern const Q_QMLCOMPILER_EXPORT QQmlSA::LoggerWarningId qmlUnterminatedCase;
extern const Q_QMLCOMPILER_EXPORT QQmlSA::LoggerWarningId qmlUnusedImports;
extern const Q_QMLCOMPILER_EXPORT QQmlSA::LoggerWarningId qmlUseProperFunction;
extern const Q_QMLCOMPILER_EXPORT QQmlSA::LoggerWarningId qmlVarUsedBeforeDeclaration;
extern const Q_QMLCOMPILER_EXPORT QQmlSA::LoggerWarningId qmlVoid;
extern const Q_QMLCOMPILER_EXPORT QQmlSA::LoggerWarningId qmlWith;

QT_END_NAMESPACE

#endif // QQMLJSLOGGER_H
