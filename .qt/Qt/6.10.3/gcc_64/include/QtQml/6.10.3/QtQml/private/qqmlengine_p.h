// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLENGINE_P_H
#define QQMLENGINE_P_H

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

#include "qqmlengine.h"

#include <private/qfieldlist_p.h>
#include <private/qintrusivelist_p.h>
#include <private/qjsengine_p.h>
#include <private/qjsvalue_p.h>
#include <private/qpodvector_p.h>
#include <private/qqmldirparser_p.h>
#include <private/qqmlimport_p.h>
#include <private/qqmlmetatype_p.h>
#include <private/qqmlnotifier_p.h>
#include <private/qqmlproperty_p.h>
#include <private/qqmltypeloader_p.h>
#include <private/qqmlvaluetype_p.h>
#include <private/qrecyclepool_p.h>
#include <private/qv4engine_p.h>

#include <QtQml/qqml.h>
#include <QtQml/qqmlcontext.h>

#include <QtCore/qlist.h>
#include <QtCore/qmetaobject.h>
#include <QtCore/qmutex.h>
#include <QtCore/qpointer.h>
#include <QtCore/qproperty.h>
#include <QtCore/qstack.h>
#include <QtCore/qstring.h>
#include <QtCore/qthread.h>

#include <atomic>

QT_BEGIN_NAMESPACE

class QNetworkAccessManager;
class QQmlDelayedError;
class QQmlIncubator;
class QQmlMetaObject;
class QQmlNetworkAccessManagerFactory;
class QQmlObjectCreator;
class QQmlProfiler;
class QQmlPropertyCapture;

// This needs to be declared here so that the pool for it can live in QQmlEnginePrivate.
// The inline method definitions are in qqmljavascriptexpression_p.h
class QQmlJavaScriptExpressionGuard : public QQmlNotifierEndpoint
{
public:
    inline QQmlJavaScriptExpressionGuard(QQmlJavaScriptExpression *);

    static inline QQmlJavaScriptExpressionGuard *New(QQmlJavaScriptExpression *e,
                                                             QQmlEngine *engine);
    inline void Delete();

    QQmlJavaScriptExpression *expression;
    QQmlJavaScriptExpressionGuard *next;
};

struct QPropertyChangeTrigger : QPropertyObserver {
    Q_DISABLE_COPY_MOVE(QPropertyChangeTrigger)

    QPropertyChangeTrigger(QQmlJavaScriptExpression *expression)
        : QPropertyObserver(&QPropertyChangeTrigger::trigger)
        , m_expression(expression)
    {
    }

    QPointer<QObject> target;
    QQmlJavaScriptExpression *m_expression;
    int propertyIndex = 0;
    static void trigger(QPropertyObserver *, QUntypedPropertyData *);

    QMetaProperty property() const;
};

struct TriggerList : QPropertyChangeTrigger {
    TriggerList(QQmlJavaScriptExpression *expression)
        : QPropertyChangeTrigger(expression)
    {}
    TriggerList *next = nullptr;
};

class Q_QML_EXPORT QQmlEnginePrivate : public QJSEnginePrivate
{
    Q_DECLARE_PUBLIC(QQmlEngine)
public:
    explicit QQmlEnginePrivate(QQmlEngine *q) : typeLoader(q) {}
    ~QQmlEnginePrivate() override;

    void init();
    // No mutex protecting baseModulesUninitialized, because use outside QQmlEngine
    // is just qmlClearTypeRegistrations (which can't be called while an engine exists)
    static bool baseModulesUninitialized;

    QQmlPropertyCapture *propertyCapture = nullptr;

    QRecyclePool<QQmlJavaScriptExpressionGuard> jsExpressionGuardPool;
    QRecyclePool<TriggerList> qPropertyTriggerPool;

    QQmlContext *rootContext = nullptr;
    Q_OBJECT_BINDABLE_PROPERTY(QQmlEnginePrivate, QString, translationLanguage);

#if !QT_CONFIG(qml_debug)
    static const quintptr profiler = 0;
#else
    QQmlProfiler *profiler = nullptr;
#endif

    bool outputWarningsToMsgLog = true;

    // Bindings that have had errors during startup
    QQmlDelayedError *erroredBindings = nullptr;
    int inProgressCreations = 0;

    QV4::ExecutionEngine *v4engine() const { return q_func()->handle(); }

#if QT_CONFIG(qml_worker_script)
    QThread *workerScriptEngine = nullptr;
#endif

    QUrl baseUrl;

    QQmlObjectCreator *activeObjectCreator = nullptr;
#if QT_CONFIG(qml_network)
    QNetworkAccessManager *getNetworkAccessManager();
    QNetworkAccessManager *networkAccessManager = nullptr;
#endif
    mutable QRecursiveMutex imageProviderMutex;
    QHash<QString,QSharedPointer<QQmlImageProviderBase> > imageProviders;
    QSharedPointer<QQmlImageProviderBase> imageProvider(const QString &providerId) const;

    int scarceResourcesRefCount = 0;
    void referenceScarceResources();
    void dereferenceScarceResources();

    QQmlTypeLoader typeLoader;

    QString offlineStoragePath;

    // Unfortunate workaround to avoid a circular dependency between
    // qqmlengine_p.h and qqmlincubator_p.h
    struct Incubator {
        QIntrusiveListNode next;
    };
    QIntrusiveList<Incubator, &Incubator::next> incubatorList;
    unsigned int incubatorCount = 0;
    QQmlIncubationController *incubationController = nullptr;
    void incubate(QQmlIncubator &, const QQmlRefPointer<QQmlContextData> &);

    // These methods may be called from any thread
    QString offlineStorageDatabaseDirectory() const;

    bool isTypeLoaded(const QUrl &url) const;
    bool isScriptLoaded(const QUrl &url) const;

    template <typename T>
    T singletonInstance(const QQmlType &type);

    void sendQuit();
    void sendExit(int retCode = 0);
    void warning(const QQmlError &);
    void warning(const QList<QQmlError> &);
    static void warning(QQmlEngine *, const QQmlError &);
    static void warning(QQmlEngine *, const QList<QQmlError> &);
    static void warning(QQmlEnginePrivate *, const QQmlError &);
    static void warning(QQmlEnginePrivate *, const QList<QQmlError> &);

    inline static QV4::ExecutionEngine *getV4Engine(QQmlEngine *e);
    inline static QQmlEnginePrivate *get(QQmlEngine *e);
    inline static const QQmlEnginePrivate *get(const QQmlEngine *e);
    inline static QQmlEnginePrivate *get(QQmlContext *c);
    inline static QQmlEnginePrivate *get(const QQmlRefPointer<QQmlContextData> &c);
    inline static QQmlEngine *get(QQmlEnginePrivate *p);
    inline static QQmlEnginePrivate *get(QV4::ExecutionEngine *e);

    static QList<QQmlError> qmlErrorFromDiagnostics(const QString &fileName, const QList<QQmlJS::DiagnosticMessage> &diagnosticMessages);

    static bool designerMode();
    static void activateDesignerMode();

    static std::atomic<bool> qml_debugging_enabled;

    QQmlGadgetPtrWrapper *valueTypeInstance(QMetaType type)
    {
        int typeIndex = type.id();
        auto it = cachedValueTypeInstances.constFind(typeIndex);
        if (it != cachedValueTypeInstances.cend())
            return *it;

        if (QQmlValueType *valueType = QQmlMetaType::valueType(type)) {
            QQmlGadgetPtrWrapper *instance = new QQmlGadgetPtrWrapper(valueType);
            cachedValueTypeInstances.insert(typeIndex, instance);
            return instance;
        }

        return nullptr;
    }

    void executeRuntimeFunction(const QUrl &url, qsizetype functionIndex, QObject *thisObject,
                                int argc = 0, void **args = nullptr, QMetaType *types = nullptr);
    void executeRuntimeFunction(const QV4::ExecutableCompilationUnit *unit, qsizetype functionIndex,
                                QObject *thisObject, int argc = 0, void **args = nullptr,
                                QMetaType *types = nullptr);
    QV4::ExecutableCompilationUnit *compilationUnitFromUrl(const QUrl &url);
    QQmlRefPointer<QQmlContextData>
    createInternalContext(const QQmlRefPointer<QV4::ExecutableCompilationUnit> &unit,
                          const QQmlRefPointer<QQmlContextData> &parentContext,
                          int subComponentIndex, bool isComponentRoot);
    static void setInternalContext(QObject *This, const QQmlRefPointer<QQmlContextData> &context,
                                   QQmlContextData::QmlObjectKind kind)
    {
        Q_ASSERT(This);
        QQmlData *ddata = QQmlData::get(This, /*create*/ true);
        // NB: copied from QQmlObjectCreator::createInstance()
        //
        // the if-statement logic to determine the kind is:
        // if (static_cast<quint32>(index) == 0 || ddata->rootObjectInCreation || isInlineComponent)
        // then QQmlContextData::DocumentRoot. here, we pass this through qmltc
        context->installContext(ddata, kind);
        Q_ASSERT(qmlEngine(This));
    }

private:
    class SingletonInstances : private QHash<QQmlType::SingletonInstanceInfo::ConstPtr, QJSValue>
    {
    public:
        void convertAndInsert(
                QV4::ExecutionEngine *engine, const QQmlType::SingletonInstanceInfo::ConstPtr &type,
                QJSValue *value)
        {
            QJSValuePrivate::manageStringOnV4Heap(engine, value);
            insert(type, *value);
        }

        void clear()
        {
            const auto canDelete = [](QObject *instance, const auto &siinfo) -> bool {
                if (!instance)
                    return false;

                if (!siinfo->url.isEmpty())
                    return true;

                const auto *ddata = QQmlData::get(instance, false);
                return !(ddata && ddata->indestructible && ddata->explicitIndestructibleSet);
            };

            for (auto it = constBegin(), end = constEnd(); it != end; ++it) {
                auto *instance = it.value().toQObject();
                if (canDelete(instance, it.key()))
                    QQmlData::markAsDeleted(instance);
            }

            for (auto it = constBegin(), end = constEnd(); it != end; ++it) {
                QObject *instance = it.value().toQObject();

                if (canDelete(instance, it.key()))
                    delete instance;
            }

            QHash<QQmlType::SingletonInstanceInfo::ConstPtr, QJSValue>::clear();
        }

        using QHash<QQmlType::SingletonInstanceInfo::ConstPtr, QJSValue>::value;
        using QHash<QQmlType::SingletonInstanceInfo::ConstPtr, QJSValue>::take;
    };

    SingletonInstances singletonInstances;
    QHash<int, QQmlGadgetPtrWrapper *> cachedValueTypeInstances;

    static bool s_designerMode;

    void cleanupScarceResources();
};

/*
   This function should be called prior to evaluation of any js expression,
   so that scarce resources are not freed prematurely (eg, if there is a
   nested javascript expression).
 */
inline void QQmlEnginePrivate::referenceScarceResources()
{
    scarceResourcesRefCount += 1;
}

/*
   This function should be called after evaluation of the js expression is
   complete, and so the scarce resources may be freed safely.
 */
inline void QQmlEnginePrivate::dereferenceScarceResources()
{
    Q_ASSERT(scarceResourcesRefCount > 0);
    scarceResourcesRefCount -= 1;

    // if the refcount is zero, then evaluation of the "top level"
    // expression must have completed.  We can safely release the
    // scarce resources.
    if (Q_LIKELY(scarceResourcesRefCount == 0)) {
        QV4::ExecutionEngine *engine = v4engine();
        if (Q_UNLIKELY(!engine->scarceResources.isEmpty())) {
            cleanupScarceResources();
        }
    }
}

QV4::ExecutionEngine *QQmlEnginePrivate::getV4Engine(QQmlEngine *e)
{
    Q_ASSERT(e);

    return e->handle();
}

QQmlEnginePrivate *QQmlEnginePrivate::get(QQmlEngine *e)
{
    Q_ASSERT(e);

    return e->d_func();
}

const QQmlEnginePrivate *QQmlEnginePrivate::get(const QQmlEngine *e)
{
    Q_ASSERT(e);

    return e ? e->d_func() : nullptr;
}

template<typename Context>
QQmlEnginePrivate *contextEngine(const Context &context)
{
    if (!context)
        return nullptr;
    if (QQmlEngine *engine = context->engine())
        return QQmlEnginePrivate::get(engine);
    return nullptr;
}

QQmlEnginePrivate *QQmlEnginePrivate::get(QQmlContext *c)
{
    return contextEngine(c);
}

QQmlEnginePrivate *QQmlEnginePrivate::get(const QQmlRefPointer<QQmlContextData> &c)
{
    return contextEngine(c);
}

QQmlEngine *QQmlEnginePrivate::get(QQmlEnginePrivate *p)
{
    Q_ASSERT(p);

    return p->q_func();
}

QQmlEnginePrivate *QQmlEnginePrivate::get(QV4::ExecutionEngine *e)
{
    QQmlEngine *qmlEngine = e->qmlEngine();
    if (!qmlEngine)
        return nullptr;
    return get(qmlEngine);
}

template<>
Q_QML_EXPORT QJSValue QQmlEnginePrivate::singletonInstance<QJSValue>(const QQmlType &type);

template<typename T>
T QQmlEnginePrivate::singletonInstance(const QQmlType &type) {
    return qobject_cast<T>(singletonInstance<QJSValue>(type).toQObject());
}

class QQmlComponentPrivate;
struct LoadHelper final : QQmlTypeLoader::Blob
{
public:
    enum class ResolveTypeResult {
        NoSuchModule,
        ModuleFound
    };

    LoadHelper(
            QQmlTypeLoader *loader, QAnyStringView uri, QAnyStringView typeName,
            QQmlTypeLoader::Mode mode);

    QQmlType type() const { return m_type; }
    QQmlTypeLoader::Mode mode() const { return m_mode; }
    ResolveTypeResult resolveTypeResult() const { return m_resolveTypeResult; }

    void registerCallback(QQmlComponentPrivate *callback);
    void unregisterCallback(QQmlComponentPrivate *callback);

protected:
    void done() final;
    void completed() final;
    void dataReceived(const SourceCodeData &) final;
    void initializeFromCachedUnit(const QQmlPrivate::CachedQmlUnit *) final { Q_UNREACHABLE(); }

private:
    bool couldFindModule() const;
    QString m_uri;
    QString m_typeName;
    QQmlType m_type;
    QQmlComponentPrivate *m_callback = nullptr;
    QQmlTypeLoader::Mode m_mode = QQmlTypeLoader::Synchronous;
    ResolveTypeResult m_resolveTypeResult = ResolveTypeResult::NoSuchModule;
};


QT_END_NAMESPACE

#endif // QQMLENGINE_P_H
