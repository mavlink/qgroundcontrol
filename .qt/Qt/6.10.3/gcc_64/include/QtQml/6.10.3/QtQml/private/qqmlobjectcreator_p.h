// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLOBJECTCREATOR_P_H
#define QQMLOBJECTCREATOR_P_H

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

#include <private/qqmlimport_p.h>
#include <private/qqmltypenamecache_p.h>
#include <private/qv4compileddata_p.h>
#include <private/qrecursionwatcher_p.h>
#include <private/qqmlprofiler_p.h>
#include <private/qv4qmlcontext_p.h>
#include <private/qqmlguardedcontextdata_p.h>
#include <private/qqmlfinalizer_p.h>
#include <private/qqmlvmemetaobject_p.h>

#include <qpointer.h>
#include <deque>

QT_BEGIN_NAMESPACE

class QQmlAbstractBinding;
class QQmlInstantiationInterrupt;
class QQmlIncubatorPrivate;

struct AliasToRequiredInfo {
    QString propertyName;
    QUrl fileUrl;
};

/*!
\internal
This struct contains information solely used for displaying error messages
\variable aliasesToRequired allows us to give the user a way to know which (aliasing) properties
can be set to set the required property
\sa QQmlComponentPrivate::unsetRequiredPropertyToQQmlError
*/
struct RequiredPropertyInfo
{
    QString propertyName;
    QUrl fileUrl;
    QV4::CompiledData::Location location;
    QVector<AliasToRequiredInfo> aliasesToRequired;
};

struct RequiredPropertyKey
{
    RequiredPropertyKey() = default;
    RequiredPropertyKey(const QObject *object, const QQmlPropertyData *data)
        : object(object)
        , data(data)
    {}

    const QObject *object = nullptr;
    const QQmlPropertyData *data = nullptr;

private:
    friend size_t qHash(const RequiredPropertyKey &key, size_t seed = 0)
    {
        return qHashMulti(seed, key.object, key.data);
    }

    friend bool operator==(const RequiredPropertyKey &a, const RequiredPropertyKey &b)
    {
        return a.object == b.object && a.data == b.data;
    }
};

class RequiredProperties : public QHash<RequiredPropertyKey, RequiredPropertyInfo> {};

class RequiredPropertiesAndTarget : public RequiredProperties
{
public:
    RequiredPropertiesAndTarget(QObject *target) : target(target) {}
    RequiredPropertiesAndTarget(const RequiredPropertiesAndTarget &) = default;
    RequiredPropertiesAndTarget(RequiredPropertiesAndTarget &&) = default;
    RequiredPropertiesAndTarget &operator=(const RequiredPropertiesAndTarget &) = default;
    RequiredPropertiesAndTarget &operator=(RequiredPropertiesAndTarget &&) = default;
    QObject *target = nullptr;
};

struct DeferredQPropertyBinding {
    QObject *target = nullptr;
    int properyIndex = -1;
    QUntypedPropertyBinding binding;
};

class Q_AUTOTEST_EXPORT ObjectInCreationGCAnchorList {
public:
    // this is a non owning view, rule of zero applies
    ObjectInCreationGCAnchorList() = default;
    ObjectInCreationGCAnchorList(const QV4::Scope &scope)
    {
        allocationScope = &scope;
    }
    void trackObject(QV4::ExecutionEngine *engine, QObject *instance);
    bool canTrack() const { return allocationScope; }
private:
    // pointer to scope to reference JS wrappers during creation phase.
    const QV4::Scope *allocationScope = nullptr;
};

struct ParserStatus {
    size_t objectIndex = 0;
    int parserStatusCast = 0;
};

struct QQmlObjectCreatorSharedState final : QQmlRefCounted<QQmlObjectCreatorSharedState>
{
    QQmlRefPointer<QQmlContextData> rootContext;
    QQmlRefPointer<QQmlContextData> creationContext;
    std::vector<QQmlAbstractBinding::Ptr> allCreatedBindings;
    std::vector<ParserStatus> allParserStatusCallbacks;
    std::vector<QQmlGuard<QObject>> allCreatedObjects;
    ObjectInCreationGCAnchorList allJavaScriptObjects; // pointer to vector on JS stack to reference JS wrappers during creation phase.
    QQmlComponentAttached *componentAttached;
    QList<QQmlFinalizerHook *> finalizeHooks;
    QQmlVmeProfiler profiler;
    QRecursionNode recursionNode;
    RequiredProperties requiredProperties;
    QList<DeferredQPropertyBinding> allQPropertyBindings;
    bool hadTopLevelRequiredProperties;
};

class Q_QML_EXPORT QQmlObjectCreator
{
    Q_DECLARE_TR_FUNCTIONS(QQmlObjectCreator)
public:
    QQmlObjectCreator(const QQmlRefPointer<QQmlContextData> &parentContext,
                      const QQmlRefPointer<QV4::ExecutableCompilationUnit> &compilationUnit,
                      const QQmlRefPointer<QQmlContextData> &creationContext,
                      const QString &inlineComponentName,
                      QQmlIncubatorPrivate  *incubator = nullptr);
    ~QQmlObjectCreator();

    enum CreationFlags { NormalObject = 1, InlineComponent = 2 };
    QObject *create(int subComponentIndex = -1, QObject *parent = nullptr,
                    QQmlInstantiationInterrupt *interrupt = nullptr, int flags = NormalObject);

    bool populateDeferredProperties(QObject *instance, const QQmlData::DeferredData *deferredData);

    void beginPopulateDeferred(const QQmlRefPointer<QQmlContextData> &context);
    void populateDeferredBinding(const QQmlProperty &qmlProperty, int deferredIndex,
                                 const QV4::CompiledData::Binding *binding);
    void populateDeferredInstance(QObject *outerObject, int deferredIndex,
                                  int index, QObject *instance, QObject *bindingTarget,
                                  const QQmlPropertyData *valueTypeProperty,
                                  const QV4::CompiledData::Binding *binding = nullptr);
    void finalizePopulateDeferred();

    bool finalize(QQmlInstantiationInterrupt &interrupt);
    void clear();

    QQmlRefPointer<QQmlContextData> rootContext() const { return sharedState->rootContext; }
    QQmlComponentAttached **componentAttachment() { return &sharedState->componentAttached; }

    QList<QQmlError> errors;

    QQmlRefPointer<QQmlContextData> parentContextData() const
    {
        return parentContext.contextData();
    }
    std::vector<QQmlGuard<QObject> > &allCreatedObjects() { return sharedState->allCreatedObjects; }

    RequiredProperties *requiredProperties() {return &sharedState->requiredProperties;}
    bool componentHadTopLevelRequiredProperties() const {return sharedState->hadTopLevelRequiredProperties;}

    static QQmlComponent *createComponent(
            QQmlEngine *engine, QV4::ExecutableCompilationUnit *compilationUnit, int index,
            QObject *parent, const QQmlRefPointer<QQmlContextData> &context);

    void removePendingBinding(QObject *target, int propertyIndex) const
    {
        QList<DeferredQPropertyBinding> &pendingBindings = sharedState.data()->allQPropertyBindings;
        pendingBindings.removeIf([&](const DeferredQPropertyBinding &deferred) {
            return deferred.properyIndex == propertyIndex && deferred.target == target;
        });
    }

private:
    QQmlObjectCreator(
            const QQmlRefPointer<QQmlContextData> &contextData,
            const QQmlRefPointer<QV4::ExecutableCompilationUnit> &compilationUnit,
            const QString inlineComponentName,
            QQmlObjectCreatorSharedState *inheritedSharedState, bool isContextObject);

    void init(const QQmlRefPointer<QQmlContextData> &parentContext);

    QObject *createInstance(int index, QObject *parent = nullptr, bool isContextObject = false);

    bool populateInstance(int index, QObject *instance, QObject *bindingTarget,
                          const QQmlPropertyData *valueTypeProperty,
                          const QV4::CompiledData::Binding *binding = nullptr);

    // If qmlProperty and binding are null, populate all properties, otherwise only the given one.
    void populateDeferred(QObject *instance, int deferredIndex);
    void populateDeferred(QObject *instance, int deferredIndex,
                          const QQmlPropertyPrivate *qmlProperty,
                          const QV4::CompiledData::Binding *binding);

    enum BindingMode {
        ApplyNone      = 0x0,
        ApplyImmediate = 0x1,
        ApplyDeferred  = 0x2,
        ApplyAll       = ApplyImmediate | ApplyDeferred,
    };
    Q_DECLARE_FLAGS(BindingSetupFlags, BindingMode);

    void setupBindings(BindingSetupFlags mode = BindingMode::ApplyImmediate);
    bool setPropertyBinding(const QQmlPropertyData *property, const QV4::CompiledData::Binding *binding);
    void setPropertyValue(const QQmlPropertyData *property, const QV4::CompiledData::Binding *binding);
    void setupFunctions();

    QString stringAt(int idx) const { return compilationUnit->stringAt(idx); }
    void recordError(const QV4::CompiledData::Location &location, const QString &description);

    void registerObjectWithContextById(const QV4::CompiledData::Object *object, QObject *instance) const;

    inline QV4::QmlContext *currentQmlContext();
    QV4::ResolvedTypeReference *resolvedType(int id) const
    {
        return compilationUnit->resolvedType(id);
    }

    enum Phase {
        Startup,
        CreatingObjects,
        CreatingObjectsPhase2,
        ObjectsCreated,
        Finalizing,
        Done
    } phase;

    QQmlEngine *engine;
    QV4::ExecutionEngine *v4;
    QString m_inlineComponentName;
    QQmlRefPointer<QV4::ExecutableCompilationUnit> compilationUnit;
    const QV4::CompiledData::Unit *qmlUnit;
    QQmlGuardedContextData parentContext;
    QQmlRefPointer<QQmlContextData> context;
    const QQmlPropertyCacheVector *propertyCaches;
    QQmlRefPointer<QQmlObjectCreatorSharedState> sharedState;
    bool topLevelCreator;
    bool isContextObject;
    QQmlIncubatorPrivate *incubator;

    QObject *_qobject;
    QObject *_scopeObject;
    QObject *_bindingTarget;

    const QQmlPropertyData *_valueTypeProperty; // belongs to _qobjectForBindings's property cache
    int _compiledObjectIndex;
    const QV4::CompiledData::Object *_compiledObject;
    QQmlData *_ddata;
    QQmlPropertyCache::ConstPtr _propertyCache;
    QQmlVMEMetaObject *_vmeMetaObject;
    QQmlListProperty<void> _currentList;
    QV4::QmlContext *_qmlContext;

    friend struct QQmlObjectCreatorRecursionWatcher;

    typedef std::function<bool(QQmlObjectCreatorSharedState *sharedState)> PendingAliasBinding;
    std::vector<PendingAliasBinding> pendingAliasBindings;

    template<typename Functor>
    void doPopulateDeferred(QObject *instance, int deferredIndex, Functor f)
    {
        QQmlData *declarativeData = QQmlData::get(instance);

        // We're in the process of creating the object. We sure hope it's still alive.
        Q_ASSERT(declarativeData && declarativeData->propertyCache);

        QObject *bindingTarget = instance;

        QQmlPropertyCache::ConstPtr cache = declarativeData->propertyCache;
        QQmlVMEMetaObject *vmeMetaObject = QQmlVMEMetaObject::get(instance);

        QObject *scopeObject = instance;
        qt_ptr_swap(_scopeObject, scopeObject);

        QV4::Scope valueScope(v4);
        QScopedValueRollback<ObjectInCreationGCAnchorList> jsObjectGuard(
                sharedState->allJavaScriptObjects, ObjectInCreationGCAnchorList(valueScope));

        Q_ASSERT(topLevelCreator);
        QV4::QmlContext *qmlContext = static_cast<QV4::QmlContext *>(valueScope.alloc());

        qt_ptr_swap(_qmlContext, qmlContext);

        _propertyCache.swap(cache);
        qt_ptr_swap(_qobject, instance);

        int objectIndex = deferredIndex;
        std::swap(_compiledObjectIndex, objectIndex);

        const QV4::CompiledData::Object *obj = compilationUnit->objectAt(_compiledObjectIndex);
        qt_ptr_swap(_compiledObject, obj);
        qt_ptr_swap(_ddata, declarativeData);
        qt_ptr_swap(_bindingTarget, bindingTarget);
        qt_ptr_swap(_vmeMetaObject, vmeMetaObject);

        f();

        qt_ptr_swap(_vmeMetaObject, vmeMetaObject);
        qt_ptr_swap(_bindingTarget, bindingTarget);
        qt_ptr_swap(_ddata, declarativeData);
        qt_ptr_swap(_compiledObject, obj);
        std::swap(_compiledObjectIndex, objectIndex);
        qt_ptr_swap(_qobject, instance);
        _propertyCache.swap(cache);

        qt_ptr_swap(_qmlContext, qmlContext);
        qt_ptr_swap(_scopeObject, scopeObject);
    }
};

struct QQmlObjectCreatorRecursionWatcher
{
    QQmlObjectCreatorRecursionWatcher(QQmlObjectCreator *creator);

    bool hasRecursed() const { return watcher.hasRecursed(); }

private:
    QQmlRefPointer<QQmlObjectCreatorSharedState> sharedState;
    QRecursionWatcher<QQmlObjectCreatorSharedState, &QQmlObjectCreatorSharedState::recursionNode> watcher;
};

QV4::QmlContext *QQmlObjectCreator::currentQmlContext()
{
    if (!_qmlContext->isManaged())
        _qmlContext->setM(QV4::QmlContext::create(v4->rootContext(), context, _scopeObject));

    return _qmlContext;
}

QT_END_NAMESPACE

#endif // QQMLOBJECTCREATOR_P_H
