// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
// Qt-Security score:significant

#ifndef QQMLJSTYPEDESCRIPTIONREADER_P_H
#define QQMLJSTYPEDESCRIPTIONREADER_P_H

#include <qtqmlcompilerexports.h>

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.

#include "qqmljsscope_p.h"

#include <QtQml/private/qqmljsastfwd_p.h>

// for Q_DECLARE_TR_FUNCTIONS
#include <QtCore/qcoreapplication.h>

QT_BEGIN_NAMESPACE

class Q_QMLCOMPILER_EXPORT QQmlJSTypeDescriptionReader
{
    Q_DECLARE_TR_FUNCTIONS(QQmlJSTypeDescriptionReader)
public:
    QQmlJSTypeDescriptionReader() = default;
    explicit QQmlJSTypeDescriptionReader(QString fileName, QString data)
        : m_fileName(std::move(fileName)), m_source(std::move(data)) {}

    bool operator()(QList<QQmlJSExportedScope> *objects, QStringList *dependencies);

    QString errorMessage() const { return m_errorMessage; }
    QString warningMessage() const { return m_warningMessage; }

private:
    void readDocument(QQmlJS::AST::UiProgram *ast);
    void readModule(QQmlJS::AST::UiObjectDefinition *ast);
    void readDependencies(QQmlJS::AST::UiScriptBinding *ast);
    void readComponent(QQmlJS::AST::UiObjectDefinition *ast);
    void readSignalOrMethod(QQmlJS::AST::UiObjectDefinition *ast, bool isMethod,
                            const QQmlJSScope::Ptr &scope);
    void readProperty(QQmlJS::AST::UiObjectDefinition *ast, const QQmlJSScope::Ptr &scope);
    void readEnum(QQmlJS::AST::UiObjectDefinition *ast, const QQmlJSScope::Ptr &scope);
    void readParameter(QQmlJS::AST::UiObjectDefinition *ast, QQmlJSMetaMethod *metaMethod);

    QString readStringBinding(QQmlJS::AST::UiScriptBinding *ast);
    bool readBoolBinding(QQmlJS::AST::UiScriptBinding *ast);
    double readNumericBinding(QQmlJS::AST::UiScriptBinding *ast);
    QTypeRevision readNumericVersionBinding(QQmlJS::AST::UiScriptBinding *ast);
    int readIntBinding(QQmlJS::AST::UiScriptBinding *ast);
    QList<QQmlJSScope::Export> readExports(QQmlJS::AST::UiScriptBinding *ast);
    void readAliases(QQmlJS::AST::UiScriptBinding *ast, const QQmlJSScope::Ptr &scope);
    void readInterfaces(QQmlJS::AST::UiScriptBinding *ast, const QQmlJSScope::Ptr &scope);
    void checkMetaObjectRevisions(
            QQmlJS::AST::UiScriptBinding *ast, QList<QQmlJSScope::Export> *exports);

    QStringList readStringList(QQmlJS::AST::UiScriptBinding *ast);
    void readDeferredNames(QQmlJS::AST::UiScriptBinding *ast, const QQmlJSScope::Ptr &scope);
    void readImmediateNames(QQmlJS::AST::UiScriptBinding *ast, const QQmlJSScope::Ptr &scope);
    void readEnumValues(QQmlJS::AST::UiScriptBinding *ast, QQmlJSMetaEnum *metaEnum);

    void addError(const QQmlJS::SourceLocation &loc, const QString &message);
    void addWarning(const QQmlJS::SourceLocation &loc, const QString &message);

    QQmlJS::AST::ArrayPattern *getArray(QQmlJS::AST::UiScriptBinding *ast);

    QString m_fileName;
    QString m_source;
    QString m_errorMessage;
    QString m_warningMessage;
    QList<QQmlJSExportedScope> *m_objects = nullptr;
    QStringList *m_dependencies = nullptr;
    int m_currentCtorIndex = 0;
    int m_currentMethodIndex = 0;
};

QT_END_NAMESPACE

#endif // QQMLJSTYPEDESCRIPTIONREADER_P_H
