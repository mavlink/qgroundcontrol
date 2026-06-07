// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
// Qt-Security score:significant

#ifndef QQMLJSCOMPILER_P_H
#define QQMLJSCOMPILER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.

#include <qtqmlcompilerexports.h>

#include <QtCore/qstring.h>
#include <QtCore/qlist.h>
#include <QtCore/qloggingcategory.h>

#include <private/qqmlirbuilder_p.h>
#include <private/qqmljscompilepass_p.h>
#include <private/qqmljscompilerstats_p.h>
#include <private/qqmljsdiagnosticmessage_p.h>
#include <private/qqmljsimporter_p.h>
#include <private/qqmljslogger_p.h>
#include <private/qqmljstyperesolver_p.h>
#include <private/qv4compileddata_p.h>

#include <functional>

QT_BEGIN_NAMESPACE

QT_DECLARE_EXPORTED_QT_LOGGING_CATEGORY(lcAotCompiler, Q_QMLCOMPILER_EXPORT);

struct Q_QMLCOMPILER_EXPORT QQmlJSCompileError
{
    QString message;
    void print();
    QQmlJSCompileError augment(const QString &contextErrorMessage) const;
    void appendDiagnostics(const QString &inputFileName,
                           const QList<QQmlJS::DiagnosticMessage> &diagnostics);
    void appendDiagnostic(const QString &inputFileName,
                          const QQmlJS::DiagnosticMessage &diagnostic);
};

struct Q_QMLCOMPILER_EXPORT QQmlJSAotFunction
{
    QStringList includes;
    QString code;
    QString signature;
    std::optional<QString> skipReason;
    int numArguments = 0;
};

class Q_QMLCOMPILER_EXPORT QQmlJSAotCompiler
{
public:
    enum Flag {
        NoFlags = 0x0,
        ValidateBasicBlocks = 0x1,
        IsLintCompiler = 0x2, // When we're linting and not compiling
    };
    Q_DECLARE_FLAGS(Flags, Flag)

    QQmlJSAotCompiler(QQmlJSImporter *importer, const QString &resourcePath,
                      const QStringList &qmldirFiles, QQmlJSLogger *logger);

    virtual ~QQmlJSAotCompiler() = default;

    virtual void setDocument(const QmlIR::JSCodeGen *codegen, const QmlIR::Document *document);
    virtual void setScope(const QmlIR::Object *object, const QmlIR::Object *scope);
    virtual std::variant<QQmlJSAotFunction, QList<QQmlJS::DiagnosticMessage>> compileBinding(
            const QV4::Compiler::Context *context, const QmlIR::Binding &irBinding,
            QQmlJS::AST::Node *astNode);
    virtual std::variant<QQmlJSAotFunction, QList<QQmlJS::DiagnosticMessage>> compileFunction(
            const QV4::Compiler::Context *context, const QString &name, QQmlJS::AST::Node *astNode);

    virtual QQmlJSAotFunction globalCode() const;

    bool isLintCompiler() const { return m_flags & IsLintCompiler; }

    Flags m_flags;

protected:
    std::optional<QList<QQmlJS::DiagnosticMessage>> finalizeBindingOrFunction();

    virtual QQmlJS::DiagnosticMessage diagnose(
            const QString &message, QtMsgType type, const QQmlJS::SourceLocation &location) const;

    QQmlJSTypeResolver m_typeResolver;

    const QString m_resourcePath;
    const QStringList m_qmldirFiles;

    const QmlIR::Document *m_document = nullptr;
    const QmlIR::Object *m_currentObject = nullptr;
    const QmlIR::Object *m_currentScope = nullptr;
    const QV4::Compiler::JSUnitGenerator *m_unitGenerator = nullptr;

    QQmlJSImporter *m_importer = nullptr;
    QQmlJSLogger *m_logger = nullptr;

private:
    QQmlJSAotFunction doCompile(
            const QV4::Compiler::Context *context, QQmlJSCompilePass::Function *function);
    QQmlJSAotFunction doCompileAndRecordAotStats(
            const QV4::Compiler::Context *context, QQmlJSCompilePass::Function *function,
            const QString &name, QQmlJS::SourceLocation location);
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QQmlJSAotCompiler::Flags);

using QQmlJSAotFunctionMap = QMap<int, QQmlJSAotFunction>;
using QQmlJSSaveFunction
    = std::function<bool(const QV4::CompiledData::SaveableUnitPointer &,
                         const QQmlJSAotFunctionMap &, QString *)>;

bool Q_QMLCOMPILER_EXPORT qCompileQmlFile(const QString &inputFileName,
                                          QQmlJSSaveFunction saveFunction,
                                          QQmlJSAotCompiler *aotCompiler, QQmlJSCompileError *error,
                                          bool storeSourceLocation = false,
                                          QV4::Compiler::CodegenWarningInterface *wInterface =
                                                  QV4::Compiler::defaultCodegenWarningInterface(),
                                          const QString *fileContents = nullptr);
bool Q_QMLCOMPILER_EXPORT qCompileQmlFile(QmlIR::Document &irDocument, const QString &inputFileName,
                                          QQmlJSSaveFunction saveFunction,
                                          QQmlJSAotCompiler *aotCompiler, QQmlJSCompileError *error,
                                          bool storeSourceLocation = false,
                                          QV4::Compiler::CodegenWarningInterface *wInterface =
                                          QV4::Compiler::defaultCodegenWarningInterface(),
                                          const QString *fileContents = nullptr);
bool Q_QMLCOMPILER_EXPORT qCompileJSFile(const QString &inputFileName, const QString &inputFileUrl,
                                         QQmlJSSaveFunction saveFunction,
                                         QQmlJSCompileError *error);

bool Q_QMLCOMPILER_EXPORT qSaveQmlJSUnitAsCpp(const QString &inputFileName,
                                              const QString &outputFileName,
                                              const QV4::CompiledData::SaveableUnitPointer &unit,
                                              const QQmlJSAotFunctionMap &aotFunctions,
                                              QString *errorString);

QT_END_NAMESPACE

#endif // QQMLJSCOMPILER_P_H
