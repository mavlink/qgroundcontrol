// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QV4EXECUTABLECOMPILATIONUNIT_P_H
#define QV4EXECUTABLECOMPILATIONUNIT_P_H

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

#include <private/qintrusivelist_p.h>
#include <private/qqmlmetatype_p.h>
#include <private/qqmlnullablevalue_p.h>
#include <private/qqmlpropertycachevector_p.h>
#include <private/qqmlrefcount_p.h>
#include <private/qqmltype_p.h>
#include <private/qqmltypenamecache_p.h>
#include <private/qv4compileddata_p.h>
#include <private/qv4identifierhash_p.h>

#include <memory>

QT_BEGIN_NAMESPACE

class QQmlScriptData;
class QQmlEnginePrivate;

namespace QV4 {

class CompilationUnitMapper;

struct CompilationUnitRuntimeData
{
    Heap::String **runtimeStrings = nullptr; // Array

    // pointers either to data->constants() or little-endian memory copy.
    // We keep this member twice so that the JIT can access it via standard layout.
    const StaticValue *constants = nullptr;

    QV4::StaticValue *runtimeRegularExpressions = nullptr;
    Heap::InternalClass **runtimeClasses = nullptr;
    const StaticValue **imports = nullptr;

    QV4::Lookup *runtimeLookups = nullptr;
    QVector<QV4::Function *> runtimeFunctions;
    QVector<QV4::Heap::InternalClass *> runtimeBlocks;
    mutable QVector<QV4::Heap::Object *> templateObjects;
};

static_assert(std::is_standard_layout_v<CompilationUnitRuntimeData>);
static_assert(offsetof(CompilationUnitRuntimeData, runtimeStrings) == 0);
static_assert(offsetof(CompilationUnitRuntimeData, constants) == sizeof(QV4::Heap::String **));
static_assert(offsetof(CompilationUnitRuntimeData, runtimeRegularExpressions) == offsetof(CompilationUnitRuntimeData, constants) + sizeof(const StaticValue *));
static_assert(offsetof(CompilationUnitRuntimeData, runtimeClasses) == offsetof(CompilationUnitRuntimeData, runtimeRegularExpressions) + sizeof(const StaticValue *));
static_assert(offsetof(CompilationUnitRuntimeData, imports) == offsetof(CompilationUnitRuntimeData, runtimeClasses) + sizeof(const StaticValue *));

class Q_QML_EXPORT ExecutableCompilationUnit final
    : public CompilationUnitRuntimeData,
      public QQmlRefCounted<ExecutableCompilationUnit>
{
    Q_DISABLE_COPY_MOVE(ExecutableCompilationUnit)
public:
    friend class QQmlRefCounted<ExecutableCompilationUnit>;
    friend class QQmlRefPointer<ExecutableCompilationUnit>;
    friend struct ExecutionEngine;

    ExecutionEngine *engine = nullptr;

    QString finalUrlString() const { return m_compilationUnit->finalUrlString(); }
    QString fileName() const { return m_compilationUnit->fileName(); }

    QUrl url() const { return m_compilationUnit->url(); }
    QUrl finalUrl() const { return m_compilationUnit->finalUrl(); }

    QQmlRefPointer<QQmlTypeNameCache> typeNameCache() const
    {
        return m_compilationUnit->typeNameCache;
    }

    QQmlPropertyCacheVector *propertyCachesPtr()
    {
        return &m_compilationUnit->propertyCaches;
    }

    QQmlPropertyCache::ConstPtr rootPropertyCache() const
    {
        return m_compilationUnit->rootPropertyCache();
    }

    // mapping from component object index (CompiledData::Unit object index that points to component) to identifier hash of named objects
    // this is initialized on-demand by QQmlContextData
    QHash<int, IdentifierHash> namedObjectsPerComponentCache;
    inline IdentifierHash namedObjectsPerComponent(int componentObjectIndex);

    ResolvedTypeReference *resolvedType(int id) const
    {
        return m_compilationUnit->resolvedType(id);
    }

    QQmlType qmlTypeForComponent(const QString &inlineComponentName = QString()) const
    {
        return m_compilationUnit->qmlTypeForComponent(inlineComponentName);
    }

    QMetaType metaType() const { return m_compilationUnit->qmlType.typeId(); }

    int inlineComponentId(const QString &inlineComponentName) const
    {
        return m_compilationUnit->inlineComponentId(inlineComponentName);
    }

    // --- interface for QQmlPropertyCacheCreator
    using CompiledObject = CompiledData::CompilationUnit::CompiledObject;
    using CompiledFunction = CompiledData::CompilationUnit::CompiledFunction;
    using CompiledBinding = CompiledData::CompilationUnit::CompiledBinding;
    using IdToObjectMap = CompiledData::CompilationUnit::IdToObjectMap;

    bool nativeMethodsAcceptThisObjects() const
    {
        return m_compilationUnit->nativeMethodsAcceptThisObjects();
    }

    bool ignoresFunctionSignature() const { return m_compilationUnit->ignoresFunctionSignature(); }
    bool valueTypesAreCopied() const { return m_compilationUnit->valueTypesAreCopied(); }
    bool valueTypesAreAddressable() const { return m_compilationUnit->valueTypesAreAddressable(); }
    bool valueTypesAreAssertable() const { return m_compilationUnit->valueTypesAreAssertable(); }
    bool componentsAreBound() const { return m_compilationUnit->componentsAreBound(); }
    bool isESModule() const { return m_compilationUnit->isESModule(); }

    int objectCount() const { return m_compilationUnit->objectCount(); }
    const CompiledObject *objectAt(int index) const
    {
        return m_compilationUnit->objectAt(index);
    }

    Heap::Object *templateObjectAt(int index) const;

    Heap::Module *instantiate();
    const Value *resolveExport(QV4::String *exportName)
    {
        QVector<ResolveSetEntry> resolveSet;
        return resolveExportRecursively(exportName, &resolveSet);
    }

    QStringList exportedNames() const
    {
        QStringList names;
        QVector<const ExecutableCompilationUnit*> exportNameSet;
        getExportedNamesRecursively(&names, &exportNameSet);
        names.sort();
        auto last = std::unique(names.begin(), names.end());
        names.erase(last, names.end());
        return names;
    }

    void evaluate();
    void evaluateModuleRequests();

    void mark(MarkStack *markStack) const { markObjects(markStack); }
    void markObjects(MarkStack *markStack) const;

    QString bindingValueAsString(const CompiledData::Binding *binding) const;
    double bindingValueAsNumber(const CompiledData::Binding *binding) const
    {
        return m_compilationUnit->bindingValueAsNumber(binding);
    }
    QString bindingValueAsScriptString(const CompiledData::Binding *binding) const
    {
        return m_compilationUnit->bindingValueAsScriptString(binding);
    }

    struct TranslationDataIndex
    {
        uint index;
        bool byId;
    };

    QString translateFrom(TranslationDataIndex index) const;

    Heap::Module *module() const;
    void setModule(Heap::Module *module);

    ReturnedValue value() const { return m_valueOrModule.asReturnedValue(); }

    // Non-ES script values are held in the context's importedScripts array.
    // That one uses the generic write barrier we have for JavaScript arrays.
    // We don't need to markCustom() here.
    void setValue(const QV4::Value &value) { m_valueOrModule = value; }

    const CompiledData::Unit *unitData() const { return m_compilationUnit->data; }

    QString stringAt(uint index) const { return m_compilationUnit->stringAt(index); }

    const QVector<QQmlRefPointer<QQmlScriptData>> *dependentScriptsPtr() const
    {
        return &m_compilationUnit->dependentScripts;
    }

    const CompiledData::BindingPropertyData *bindingPropertyDataPerObjectAt(
            qsizetype objectIndex) const
    {
        return &m_compilationUnit->bindingPropertyDataPerObject.at(objectIndex);
    }

    const QQmlRefPointer<QV4::CompiledData::CompilationUnit> &baseCompilationUnit() const
    {
        return m_compilationUnit;
    }

    QV4::Function *rootFunction()
    {
        if (!runtimeStrings)
            populate();

        const auto *data = unitData();
        return data->indexOfRootFunction != -1
                ? runtimeFunctions[data->indexOfRootFunction]
                : nullptr;
    }

    void populate();
    void clear();

protected:
    quint32 totalStringCount() const
    { return unitData()->stringTableSize; }

private:
    friend struct ExecutionEngine;

    QQmlRefPointer<CompiledData::CompilationUnit> m_compilationUnit;
    Value m_valueOrModule = QV4::Value::emptyValue();

    struct ResolveSetEntry
    {
        ResolveSetEntry() {}
        ResolveSetEntry(ExecutableCompilationUnit *module, QV4::String *exportName)
            : module(module), exportName(exportName) {}
        ExecutableCompilationUnit *module = nullptr;
        QV4::String *exportName = nullptr;
    };

    ExecutableCompilationUnit();
    ExecutableCompilationUnit(QQmlRefPointer<CompiledData::CompilationUnit> &&compilationUnit);
    ~ExecutableCompilationUnit();

    static QQmlRefPointer<ExecutableCompilationUnit> create(
            QQmlRefPointer<CompiledData::CompilationUnit> &&compilationUnit,
            ExecutionEngine *engine);

    const Value *resolveExportRecursively(QV4::String *exportName,
                                          QVector<ResolveSetEntry> *resolveSet);

    QUrl urlAt(int index) const { return QUrl(stringAt(index)); }

    Q_NEVER_INLINE IdentifierHash createNamedObjectsPerComponent(int componentObjectIndex);
    const CompiledData::ExportEntry *lookupNameInExportTable(
            const CompiledData::ExportEntry *firstExportEntry, int tableSize,
            QV4::String *name) const;

    void getExportedNamesRecursively(
            QStringList *names, QVector<const ExecutableCompilationUnit *> *exportNameSet,
            bool includeDefaultExport = true) const;
};

IdentifierHash ExecutableCompilationUnit::namedObjectsPerComponent(int componentObjectIndex)
{
    auto it = namedObjectsPerComponentCache.constFind(componentObjectIndex);
    if (Q_UNLIKELY(it == namedObjectsPerComponentCache.cend()))
        return createNamedObjectsPerComponent(componentObjectIndex);
    Q_ASSERT(!it->isEmpty());
    return *it;
}

} // namespace QV4

QT_END_NAMESPACE

#endif // QV4EXECUTABLECOMPILATIONUNIT_P_H
