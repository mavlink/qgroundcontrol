// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLTYPECOMPILER_P_H
#define QQMLTYPECOMPILER_P_H

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

#include <qglobal.h>
#include <qqmlerror.h>
#include <qhash.h>
#include <private/qqmltypeloader_p.h>
#include <private/qqmlirbuilder_p.h>
#include <private/qqmlpropertycachecreator_p.h>

QT_BEGIN_NAMESPACE

class QQmlEnginePrivate;
class QQmlError;
class QQmlTypeData;
class QQmlImports;

namespace QmlIR {
struct Document;
}

namespace QV4 {
namespace CompiledData {
struct QmlUnit;
struct Location;
}
}

struct QQmlTypeCompiler
{
    Q_DECLARE_TR_FUNCTIONS(QQmlTypeCompiler)
public:
    QQmlTypeCompiler(QQmlTypeLoader *typeLoader,
                     QQmlTypeData *typeData,
                     QmlIR::Document *document,
                     QV4::CompiledData::ResolvedTypeReferenceMap *resolvedTypeCache,
                     const QV4::CompiledData::DependentTypesHasher &dependencyHasher);

    // --- interface used by QQmlPropertyCacheCreator
    typedef QmlIR::Object CompiledObject;
    typedef QmlIR::Binding CompiledBinding;
    using ListPropertyAssignBehavior = QmlIR::Pragma::ListPropertyAssignBehaviorValue;

    // Deliberate choice of map over hash here to ensure stable generated output.
    using IdToObjectMap = QMap<int, int>;

    const QmlIR::Object *objectAt(int index) const { return document->objects.at(index); }
    QmlIR::Object *objectAt(int index) { return document->objects.at(index); }
    int objectCount() const { return document->objects.size(); }
    QString stringAt(int idx) const;
    QmlIR::PoolList<QmlIR::Function>::Iterator objectFunctionsBegin(const QmlIR::Object *object) const { return object->functionsBegin(); }
    QmlIR::PoolList<QmlIR::Function>::Iterator objectFunctionsEnd(const QmlIR::Object *object) const { return object->functionsEnd(); }
    QV4::CompiledData::ResolvedTypeReferenceMap *resolvedTypes = nullptr;
    ListPropertyAssignBehavior listPropertyAssignBehavior() const
    {
        for (const QmlIR::Pragma *pragma: document->pragmas) {
            if (pragma->type == QmlIR::Pragma::ListPropertyAssignBehavior)
                return pragma->listPropertyAssignBehavior;
        }
        return ListPropertyAssignBehavior::Append;
    }
    // ---

    QQmlRefPointer<QV4::CompiledData::CompilationUnit> compile();

    QList<QQmlError> compilationErrors() const { return errors; }
    void recordError(const QV4::CompiledData::Location &location, const QString &description);
    void recordError(const QQmlJS::DiagnosticMessage &message);
    void recordError(const QQmlError &e);

    int registerString(const QString &str);
    int registerConstant(QV4::ReturnedValue v);

    const QV4::CompiledData::Unit *qmlUnit() const;

    QUrl url() const { return typeData->finalUrl(); }
    QQmlTypeLoader *typeLoader() const { return loader; }
    const QQmlImports *imports() const;
    QVector<QmlIR::Object *> *qmlObjects() const;
    QQmlPropertyCacheVector *propertyCaches();
    const QQmlPropertyCacheVector *propertyCaches() const;
    QQmlJS::MemoryPool *memoryPool();
    QStringView newStringRef(const QString &string);
    const QV4::Compiler::StringTableGenerator *stringPool() const;

    const QHash<int, QQmlCustomParser*> &customParserCache() const { return customParsers; }

    QString bindingAsString(const QmlIR::Object *object, int scriptIndex) const;

    void addImport(const QString &module, const QString &qualifier, QTypeRevision version);

    QV4::ResolvedTypeReference *resolvedType(int id) const
    {
        return resolvedTypes->value(id);
    }

    QV4::ResolvedTypeReference *resolvedType(QMetaType type) const
    {
        for (QV4::ResolvedTypeReference *ref : std::as_const(*resolvedTypes)) {
            if (ref->type().typeId() == type)
                return ref;
        }
        return nullptr;
    }

    QQmlType qmlTypeForComponent(const QString &inlineComponentName = QString()) const;

private:
    QList<QQmlError> errors;
    QQmlTypeLoader *loader;
    const QV4::CompiledData::DependentTypesHasher &dependencyHasher;
    QmlIR::Document *document;
    // index is string index of type name (use obj->inheritedTypeNameIndex)
    QHash<int, QQmlCustomParser*> customParsers;

    // index in first hash is component index, vector inside contains object indices of objects with id property
    QQmlPropertyCacheVector m_propertyCaches;

    QQmlTypeData *typeData;
};

struct QQmlCompilePass
{
    QQmlCompilePass(QQmlTypeCompiler *typeCompiler);

    QString stringAt(int idx) const { return compiler->stringAt(idx); }
protected:
    void recordError(const QV4::CompiledData::Location &location, const QString &description) const
    { compiler->recordError(location, description); }

    QV4::ResolvedTypeReference *resolvedType(int id) const
    { return compiler->resolvedType(id); }

    QQmlTypeCompiler *compiler;
};

// Resolves signal handlers. Updates the QV4::CompiledData::Binding objects to
// set the property name to the final signal name (onTextChanged -> textChanged)
// and sets the IsSignalExpression flag.
struct SignalHandlerResolver : public QQmlCompilePass
{
    Q_DECLARE_TR_FUNCTIONS(SignalHandlerResolver)
public:
    SignalHandlerResolver(QQmlTypeCompiler *typeCompiler);

    bool resolveSignalHandlerExpressions();

private:
    bool resolveSignalHandlerExpressions(
            const QmlIR::Object *obj, const QString &typeName,
            const QQmlPropertyCache::ConstPtr &propertyCache,
            QQmlPropertyResolver::RevisionCheck revisionCheck
                    = QQmlPropertyResolver::CheckRevision);

    QQmlTypeLoader *typeLoader;
    const QVector<QmlIR::Object*> &qmlObjects;
    const QQmlImports *imports;
    const QHash<int, QQmlCustomParser*> &customParsers;
    const QQmlPropertyCacheVector * const propertyCaches;
};

// ### This will go away when the codegen resolves all enums to constant expressions
// and we replace the constant expression with a literal binding instead of using
// a script.
class QQmlEnumTypeResolver : public QQmlCompilePass
{
    Q_DECLARE_TR_FUNCTIONS(QQmlEnumTypeResolver)
public:
    QQmlEnumTypeResolver(QQmlTypeCompiler *typeCompiler);

    bool resolveEnumBindings();

private:
    bool assignEnumToBinding(QmlIR::Binding *binding, QStringView enumName, int enumValue, bool isQtObject);
    bool assignEnumToBinding(QmlIR::Binding *binding, const QString &enumName, int enumValue, bool isQtObject)
    {
        return assignEnumToBinding(binding, QStringView(enumName), enumValue, isQtObject);
    }
    bool tryQualifiedEnumAssignment(
            const QmlIR::Object *obj, const QQmlPropertyCache::ConstPtr &propertyCache,
            const QQmlPropertyData *prop, QmlIR::Binding *binding);
    int evaluateEnum(const QString &scope, QStringView enumName, QStringView enumValue, bool *ok) const;


    const QVector<QmlIR::Object*> &qmlObjects;
    const QQmlPropertyCacheVector * const propertyCaches;
    const QQmlImports *imports;
};

class QQmlCustomParserScriptIndexer: public QQmlCompilePass
{
public:
    QQmlCustomParserScriptIndexer(QQmlTypeCompiler *typeCompiler);

    void annotateBindingsWithScriptStrings();

private:
    void scanObjectRecursively(int objectIndex, bool annotateScriptBindings = false);

    const QVector<QmlIR::Object*> &qmlObjects;
    const QHash<int, QQmlCustomParser*> &customParsers;
};

// Annotate properties bound to aliases with a flag
class QQmlAliasAnnotator : public QQmlCompilePass
{
public:
    QQmlAliasAnnotator(QQmlTypeCompiler *typeCompiler);

    void annotateBindingsToAliases();
private:
    const QVector<QmlIR::Object*> &qmlObjects;
    const QQmlPropertyCacheVector * const propertyCaches;
};

class QQmlScriptStringScanner : public QQmlCompilePass
{
public:
    QQmlScriptStringScanner(QQmlTypeCompiler *typeCompiler);

    void scan();

private:
    const QVector<QmlIR::Object*> &qmlObjects;
    const QQmlPropertyCacheVector * const propertyCaches;
};

class QQmlDeferredAndCustomParserBindingScanner : public QQmlCompilePass
{
    Q_DECLARE_TR_FUNCTIONS(QQmlDeferredAndCustomParserBindingScanner)
public:
    QQmlDeferredAndCustomParserBindingScanner(QQmlTypeCompiler *typeCompiler);

    bool scanObject();

private:
    enum class ScopeDeferred { False, True };
    bool scanObject(int objectIndex, ScopeDeferred scopeDeferred);

    QVector<QmlIR::Object*> *qmlObjects;
    const QQmlPropertyCacheVector * const propertyCaches;
    const QHash<int, QQmlCustomParser*> &customParsers;

    bool _seenObjectWithId;
};

class QQmlDefaultPropertyMerger : public QQmlCompilePass
{
public:
    QQmlDefaultPropertyMerger(QQmlTypeCompiler *typeCompiler);

    void mergeDefaultProperties();

private:
    void mergeDefaultProperties(int objectIndex);

    const QVector<QmlIR::Object*> &qmlObjects;
    const QQmlPropertyCacheVector * const propertyCaches;
};

QT_END_NAMESPACE

#endif // QQMLTYPECOMPILER_P_H
