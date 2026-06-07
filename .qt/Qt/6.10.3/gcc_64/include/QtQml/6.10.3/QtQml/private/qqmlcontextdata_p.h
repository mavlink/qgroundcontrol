// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLCONTEXTDATA_P_H
#define QQMLCONTEXTDATA_P_H

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

#include <QtQml/private/qtqmlglobal_p.h>
#include <QtQml/private/qqmlcontext_p.h>
#include <QtQml/private/qqmlguard_p.h>
#include <QtQml/private/qqmltypenamecache_p.h>
#include <QtQml/private/qqmlnotifier_p.h>
#include <QtQml/private/qv4identifierhash_p.h>
#include <QtQml/private/qv4executablecompilationunit_p.h>

QT_BEGIN_NAMESPACE

class QQmlComponentAttached;
class QQmlGuardedContextData;
class QQmlJavaScriptExpression;
class QQmlIncubatorPrivate;

class Q_QML_EXPORT QQmlContextData
{
public:
    static QQmlRefPointer<QQmlContextData> createRefCounted(
            const QQmlRefPointer<QQmlContextData> &parent)
    {
        return QQmlRefPointer<QQmlContextData>(new QQmlContextData(RefCounted, nullptr, parent),
                                               QQmlRefPointer<QQmlContextData>::Adopt);
    }

    // Owned by the parent. When the parent is reset to nullptr, it will be deref'd.
    static QQmlRefPointer<QQmlContextData> createChild(
            const QQmlRefPointer<QQmlContextData> &parent)
    {
        Q_ASSERT(!parent.isNull());
        return QQmlRefPointer<QQmlContextData>(new QQmlContextData(OwnedByParent, nullptr, parent));
    }

    void addref() const { ++m_refCount; }
    void release() const { if (--m_refCount == 0) delete this; }
    int count() const { return m_refCount; }
    int refCount() const { return m_refCount; }

    QQmlRefPointer<QV4::ExecutableCompilationUnit> typeCompilationUnit() const
    {
        return m_typeCompilationUnit;
    }
    void initFromTypeCompilationUnit(const QQmlRefPointer<QV4::ExecutableCompilationUnit> &unit,
                                     int subComponentIndex);

    static QQmlRefPointer<QQmlContextData> get(QQmlContext *context) {
        return QQmlContextPrivate::get(context)->m_data;
    }

    void emitDestruction();
    void clearContext();
    void clearContextRecursively();
    void invalidate();

    bool isValid() const
    {
        return m_engine && (!m_isInternal || !m_contextObject
                            || !QObjectPrivate::get(m_contextObject)->wasDeleted);
    }

    bool isInternal() const { return m_isInternal; }
    void setInternal(bool isInternal) { m_isInternal = isInternal; }

    bool isJSContext() const { return m_isJSContext; }
    void setJSContext(bool isJSContext) { m_isJSContext = isJSContext; }

    bool isPragmaLibraryContext() const { return m_isPragmaLibraryContext; }
    void setPragmaLibraryContext(bool library) { m_isPragmaLibraryContext = library; }

    QQmlRefPointer<QQmlContextData> parent() const { return m_parent; }
    void clearParent()
    {
        if (!m_parent)
            return;

        m_parent = nullptr;
        if (m_ownedByParent) {
            m_ownedByParent = false;
            release();
        }
    }

    void refreshExpressions();

    void addOwnedObject(QQmlData *ownedObject);
    QQmlData *ownedObjects() const { return m_ownedObjects; }
    void setOwnedObjects(QQmlData *ownedObjects) { m_ownedObjects = ownedObjects; }

    enum QmlObjectKind {
        OrdinaryObject,
        DocumentRoot,
    };
    void installContext(QQmlData *ddata, QmlObjectKind kind);

    QUrl resolvedUrl(const QUrl &) const;

    // My containing QQmlContext.  If isInternal is true this owns publicContext.
    // If internal is false publicContext owns this.
    QQmlContext *asQQmlContext()
    {
        if (!m_publicContext)
            m_publicContext = new QQmlContext(*new QQmlContextPrivate(this));
        return m_publicContext;
    }

    QQmlContextPrivate *asQQmlContextPrivate()
    {
        return QQmlContextPrivate::get(asQQmlContext());
    }

    QObject *contextObject() const { return m_contextObject; }
    void setContextObject(QObject *contextObject) { m_contextObject = contextObject; }

    template<typename HandleSelf, typename HandleLinked>
    void deepClearContextObject(
            QObject *contextObject, HandleSelf &&handleSelf, HandleLinked &&handleLinked) {
        for (QQmlContextData *lc = m_linkedContext.data(); lc; lc = lc->m_linkedContext.data()) {
            handleLinked(lc);
            if (lc->m_contextObject == contextObject)
                lc->m_contextObject = nullptr;
        }

        handleSelf(this);
        if (m_contextObject == contextObject)
            m_contextObject = nullptr;
    }

    void deepClearContextObject(QObject *contextObject)
    {
        deepClearContextObject(
                contextObject,
                [](QQmlContextData *self) { self->emitDestruction(); },
                [](QQmlContextData *){});
    }

    QQmlEngine *engine() const { return m_engine; }
    void setEngine(QQmlEngine *engine) { m_engine = engine; }

    QQmlContext *publicContext() const { return m_publicContext; }
    void clearPublicContext()
    {
        if (!m_publicContext)
            return;

        m_publicContext = nullptr;
        if (m_ownedByPublicContext) {
            m_ownedByPublicContext = false;
            release();
        }
    }

    int propertyIndex(const QString &name) const
    {
        ensurePropertyNames();
        return m_propertyNameCache.value(name);
    }

    int propertyIndex(QV4::String *name) const
    {
        ensurePropertyNames();
        return m_propertyNameCache.value(name);
    }

    QString propertyName(int index) const
    {
        ensurePropertyNames();
        return m_propertyNameCache.findId(index);
    }

    void addPropertyNameAndIndex(const QString &name, int index)
    {
        Q_ASSERT(!m_propertyNameCache.isEmpty());
        m_propertyNameCache.add(name, index);
    }

    void setExpressions(QQmlJavaScriptExpression *expressions) { m_expressions = expressions; }
    QQmlJavaScriptExpression *takeExpressions()
    {
        QQmlJavaScriptExpression *expressions = m_expressions;
        m_expressions = nullptr;
        return expressions;
    }

    void setChildContexts(const QQmlRefPointer<QQmlContextData> &childContexts)
    {
        m_childContexts = childContexts.data();
    }
    QQmlRefPointer<QQmlContextData> childContexts() const { return m_childContexts; }
    QQmlRefPointer<QQmlContextData> takeChildContexts()
    {
        QQmlRefPointer<QQmlContextData> childContexts = m_childContexts;
        m_childContexts = nullptr;
        return childContexts;
    }
    QQmlRefPointer<QQmlContextData> nextChild() const { return m_nextChild; }

    int numIdValues() const { return m_idValueCount; }
    void setIdValue(int index, QObject *idValue);
    bool isIdValueSet(int index) const { return m_idValues[index].wasSet(); }
    QQmlNotifier *idValueBindings(int index) const { return m_idValues[index].bindings(); }
    QObject *idValue(int index) const { return m_idValues[index].data(); }

    // Return the outermost id for obj, if any.
    QString findObjectId(const QObject *obj) const;

    // url() and urlString() prefer the CU's URL over explicitly set baseUrls. They
    // don't search the context hierarchy.
    // baseUrl() and baseUrlString() search the context hierarchy and prefer explicit
    // base URLs over CU Urls.

    QUrl url() const;
    QString urlString() const;

    void setBaseUrlString(const QString &baseUrlString) { m_baseUrlString = baseUrlString; }
    QString baseUrlString() const
    {
        for (const QQmlContextData *data = this; data; data = data->m_parent) {
            if (!data->m_baseUrlString.isEmpty())
                return data->m_baseUrlString;
            if (data->m_typeCompilationUnit)
                return data->m_typeCompilationUnit->finalUrlString();
        }
        return QString();
    }

    void setBaseUrl(const QUrl &baseUrl) { m_baseUrl = baseUrl; }
    QUrl baseUrl() const
    {
        for (const QQmlContextData *data = this; data; data = data->m_parent) {
            if (!data->m_baseUrl.isEmpty())
                return data->m_baseUrl;
            if (data->m_typeCompilationUnit)
                return data->m_typeCompilationUnit->finalUrl();
        }
        return QUrl();
    }

    QQmlRefPointer<QQmlTypeNameCache> imports() const { return m_imports; }
    void setImports(const QQmlRefPointer<QQmlTypeNameCache> &imports) { m_imports = imports; }

    QQmlIncubatorPrivate *incubator() const { return m_hasExtraObject ? nullptr : m_incubator; }
    void setIncubator(QQmlIncubatorPrivate *incubator)
    {
        Q_ASSERT(!m_hasExtraObject || m_extraObject == nullptr);
        m_hasExtraObject = false;
        m_incubator = incubator;
    }

    QObject *extraObject() const { return m_hasExtraObject ? m_extraObject : nullptr; }
    void setExtraObject(QObject *extraObject)
    {
        Q_ASSERT(m_hasExtraObject || m_incubator == nullptr);
        m_hasExtraObject = true;
        m_extraObject = extraObject;
    }

    bool isRootObjectInCreation() const { return m_isRootObjectInCreation; }
    void setRootObjectInCreation(bool rootInCreation) { m_isRootObjectInCreation = rootInCreation; }

    QV4::ReturnedValue importedScripts() const
    {
        if (m_hasWeakImportedScripts)
            return m_weakImportedScripts.value();
        else
            return m_importedScripts.value();
    }
    void setImportedScripts(QV4::ExecutionEngine *engine, QV4::Value scripts) {
        // setImportedScripts should not be called on an invalidated context
        Q_ASSERT(!m_hasWeakImportedScripts);
        m_importedScripts.set(engine, scripts);
    }

    /*
       we can safely pass a ReturnedValue here, as setImportedScripts will directly store
       scripts in a persistentValue, without any intermediate allocation that could trigger
       a gc run
    */
    void setImportedScripts(QV4::ExecutionEngine *engine, QV4::ReturnedValue scripts) {
        // setImportedScripts should not be called on an invalidated context
        Q_ASSERT(!m_hasWeakImportedScripts);
        m_importedScripts.set(engine, scripts);
    }

    QQmlRefPointer<QQmlContextData> linkedContext() const { return m_linkedContext; }
    void setLinkedContext(const QQmlRefPointer<QQmlContextData> &context) { m_linkedContext = context; }

    bool hasUnresolvedNames() const { return m_unresolvedNames; }
    void setUnresolvedNames(bool hasUnresolvedNames) { m_unresolvedNames = hasUnresolvedNames; }

    QQmlComponentAttached *componentAttacheds() const { return m_componentAttacheds; }
    void addComponentAttached(QQmlComponentAttached *attached);

    void addExpression(QQmlJavaScriptExpression *expression);

    bool valueTypesAreAddressable() const {
        return m_typeCompilationUnit && m_typeCompilationUnit->valueTypesAreAddressable();
    }

    bool valueTypesAreAssertable() const {
        return m_typeCompilationUnit && m_typeCompilationUnit->valueTypesAreAssertable();
    }

private:
    friend class QQmlGuardedContextData;
    friend class QQmlContextPrivate;

    enum Ownership {
        RefCounted,
        OwnedByParent,
        OwnedByPublicContext
    };

    // id guards
    struct ContextGuard : public QQmlGuard<QObject>
    {
        enum Tag {
            NoTag,
            ObjectWasSet
        };

        inline ContextGuard() : QQmlGuard<QObject>(&ContextGuard::objectDestroyedImpl, nullptr), m_context(nullptr) {}
        inline ContextGuard &operator=(QObject *obj);

        inline bool wasSet() const;

        QQmlNotifier *bindings() { return &m_bindings; }
        void setContext(const QQmlRefPointer<QQmlContextData> &context)
        {
            m_context = context.data();
        }

    private:
        inline static void objectDestroyedImpl(QQmlGuardImpl *);
        // Not refcounted, as it always belongs to the QQmlContextData.
        QTaggedPointer<QQmlContextData, Tag> m_context;
        QQmlNotifier m_bindings;
    };

    // It's OK to pass a half-created publicContext here. We will not dereference it during
    // construction.
    QQmlContextData(
            Ownership ownership, QQmlContext *publicContext,
            const QQmlRefPointer<QQmlContextData> &parent,  QQmlEngine *engine = nullptr)
        : m_parent(parent.data()),
          m_engine(engine ? engine : (parent.isNull() ? nullptr : parent->engine())),
          m_isInternal(false), m_isJSContext(false), m_isPragmaLibraryContext(false),
          m_unresolvedNames(false), m_hasEmittedDestruction(false), m_isRootObjectInCreation(false),
          m_ownedByParent(ownership == OwnedByParent),
          m_ownedByPublicContext(ownership == OwnedByPublicContext), m_hasExtraObject(false),
          m_hasWeakImportedScripts(false), m_dummy(0), m_publicContext(publicContext), m_incubator(nullptr)
    {
        Q_ASSERT(!m_ownedByParent || !m_ownedByPublicContext);
        if (!m_parent)
            return;

        m_nextChild = m_parent->m_childContexts;
        if (m_nextChild)
            m_nextChild->m_prevChild = &m_nextChild;
        m_prevChild = &m_parent->m_childContexts;
        m_parent->m_childContexts = this;
    }

    ~QQmlContextData();

    bool hasExpressionsToRun(bool isGlobalRefresh) const
    {
        return m_expressions && (!isGlobalRefresh || m_unresolvedNames);
    }

    void refreshExpressionsRecursive(bool isGlobal);
    void refreshExpressionsRecursive(QQmlJavaScriptExpression *);
    void initPropertyNames() const;

    void ensurePropertyNames() const
    {
        if (m_propertyNameCache.isEmpty())
            initPropertyNames();
        Q_ASSERT(!m_propertyNameCache.isEmpty());
    }

    // My parent context and engine
    QQmlContextData *m_parent = nullptr;
    QQmlEngine *m_engine = nullptr;

    mutable quint32 m_refCount = 1;
    quint32 m_isInternal:1;
    quint32 m_isJSContext:1;
    quint32 m_isPragmaLibraryContext:1;
    quint32 m_unresolvedNames:1; // True if expressions in this context failed to resolve a toplevel name
    quint32 m_hasEmittedDestruction:1;
    quint32 m_isRootObjectInCreation:1;
    quint32 m_ownedByParent:1;
    quint32 m_ownedByPublicContext:1;
    quint32 m_hasExtraObject:1; // used in QQmlDelegateModelItem::dataForObject to find the corresponding QQmlDelegateModelItem of an object
    quint32 m_hasWeakImportedScripts:1;
    Q_DECL_UNUSED_MEMBER quint32 m_dummy:22;
    QQmlContext *m_publicContext = nullptr;

    union {
        // The incubator that is constructing this context if any
        QQmlIncubatorPrivate *m_incubator;
        // a pointer to extra data, currently only used in QQmlDelegateModel
        QObject *m_extraObject;
    };

    // Compilation unit for contexts that belong to a compiled type.
    QQmlRefPointer<QV4::ExecutableCompilationUnit> m_typeCompilationUnit;

    // object index in CompiledData::Unit to component that created this context
    int m_componentObjectIndex = -1;

    // flag indicates whether the context owns the cache (after mutation) or not.
    mutable QV4::IdentifierHash m_propertyNameCache;

    // Context object
    QObject *m_contextObject = nullptr;

    // Any script blocks that exist on this context
    union {
        /* an invalidated context transitions from a strong reference to the scripts
           to a weak one, so that the context doesn't needlessly holds on to the scripts,
           but closures can still access them if needed
         */
        QV4::PersistentValue m_importedScripts = {}; // This is a JS Array
        QV4::WeakValue m_weakImportedScripts;
    };

    QUrl m_baseUrl;
    QString m_baseUrlString;

    // List of imports that apply to this context
    QQmlRefPointer<QQmlTypeNameCache> m_imports;

    // My children, not refcounted as that would create cyclic references
    QQmlContextData *m_childContexts = nullptr;

    // My peers in parent's childContexts list; not refcounted
    QQmlContextData  *m_nextChild = nullptr;
    QQmlContextData **m_prevChild = nullptr;

    // Expressions that use this context
    QQmlJavaScriptExpression *m_expressions = nullptr;

    // Doubly-linked list of objects that are owned by this context
    QQmlData *m_ownedObjects = nullptr;

    // Doubly-linked list of context guards (XXX merge with contextObjects)
    QQmlGuardedContextData *m_contextGuards = nullptr;

    ContextGuard *m_idValues = nullptr;
    int m_idValueCount = 0;

    // Linked contexts. this owns linkedContext.
    QQmlRefPointer<QQmlContextData> m_linkedContext;

    // Linked list of uses of the Component attached property in this context
    QQmlComponentAttached *m_componentAttacheds = nullptr;
};

QQmlContextData::ContextGuard &QQmlContextData::ContextGuard::operator=(QObject *obj)
{
    QQmlGuard<QObject>::operator=(obj);
    m_context.setTag(ObjectWasSet);
    m_bindings.notify(); // For alias connections
    return *this;
}

 void QQmlContextData::ContextGuard::objectDestroyedImpl(QQmlGuardImpl *impl)
{
    auto This = static_cast<QQmlContextData::ContextGuard *>(impl);
    if (QObject *contextObject = This->m_context->contextObject()) {
        if (!QObjectPrivate::get(contextObject)->wasDeleted)
            This->m_bindings.notify();
    }
}

bool QQmlContextData::ContextGuard::wasSet() const
{
    return m_context.tag() == ObjectWasSet;
}

QT_END_NAMESPACE

#endif // QQMLCONTEXTDATA_P_H
