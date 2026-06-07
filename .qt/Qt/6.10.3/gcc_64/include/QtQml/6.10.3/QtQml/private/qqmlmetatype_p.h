// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLMETATYPE_P_H
#define QQMLMETATYPE_P_H

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

#include <private/qqmldirparser_p.h>
#include <private/qqmlmetaobject_p.h>
#include <private/qqmlproxymetaobject_p.h>
#include <private/qqmltype_p.h>
#include <private/qtqmlglobal_p.h>

QT_BEGIN_NAMESPACE

class QQmlTypeModule;
class QRecursiveMutex;
class QQmlError;
class QQmlValueType;

namespace QV4 {
namespace CompiledData {
struct CompilationUnit;
}
}

class Q_QML_EXPORT QQmlMetaType
{
    friend class QQmlDesignerMetaObject;

public:

    enum class RegistrationResult {
        Success,
        Failure,
        NoRegistrationFunction
    };

    static QUrl inlineComponentUrl(const QUrl &baseUrl, const QString &name)
    {
        QUrl icUrl = baseUrl;
        icUrl.setFragment(name);
        return icUrl;
    }

    static bool equalBaseUrls(const QUrl &aUrl, const QUrl &bUrl)
    {
        // Everything but fragment has to match
        return aUrl.port() == bUrl.port()
                && aUrl.scheme() == bUrl.scheme()
                && aUrl.userName() == bUrl.userName()
                && aUrl.password() == bUrl.password()
                && aUrl.host() == bUrl.host()
                && aUrl.path() == bUrl.path()
                && aUrl.query() == bUrl.query();
    }

    enum CompositeTypeLookupMode {
        NonSingleton,
        Singleton,
        JavaScript,
    };

    static QQmlType findCompositeType(
            const QUrl &url,
            const QQmlRefPointer<QV4::CompiledData::CompilationUnit> &compilationUnit,
            CompositeTypeLookupMode mode = NonSingleton);
    static QQmlType findInlineComponentType(
            const QUrl &url,
            const QQmlRefPointer<QV4::CompiledData::CompilationUnit> &compilationUnit);
    static QQmlType findInlineComponentType(
            const QUrl &baseUrl, const QString &name,
            const QQmlRefPointer<QV4::CompiledData::CompilationUnit> &compilationUnit)
    {
        return findInlineComponentType(inlineComponentUrl(baseUrl, name), compilationUnit);
    }

    static QQmlType registerType(const QQmlPrivate::RegisterType &type);
    static QQmlType registerInterface(const QQmlPrivate::RegisterInterface &type);
    static QQmlType registerSingletonType(
            const QQmlPrivate::RegisterSingletonType &type,
            const QQmlType::SingletonInstanceInfo::ConstPtr &siinfo);
    static QQmlType registerCompositeSingletonType(
            const QQmlPrivate::RegisterCompositeSingletonType &type,
            const QQmlType::SingletonInstanceInfo::ConstPtr &siinfo);
    static QQmlType registerCompositeType(const QQmlPrivate::RegisterCompositeType &type);
    static RegistrationResult registerPluginTypes(QObject *instance, const QString &basePath,
                                                  const QString &uri, const QString &typeNamespace,
                                                  QTypeRevision version, QList<QQmlError> *errors);

    static QQmlType typeForUrl(const QString &urlString, const QHashedStringRef& typeName,
                               CompositeTypeLookupMode mode, QList<QQmlError> *errors,
                               QTypeRevision version = QTypeRevision());

    static QQmlType fetchOrCreateInlineComponentTypeForUrl(const QUrl &url);
    static QQmlType inlineComponentType(const QQmlType &outerType, const QString &name)
    {
        return outerType.isComposite()
                ? fetchOrCreateInlineComponentTypeForUrl(
                        inlineComponentUrl(outerType.sourceUrl(), name))
                : QQmlType();
    }

    static void unregisterType(int type);

    static void registerMetaObjectForType(const QMetaObject *metaobject, QQmlTypePrivate *type);

    static void registerModule(const char *uri, QTypeRevision version);
    static bool protectModule(const QString &uri, QTypeRevision version,
                              bool weakProtectAllVersions = false);

    static void registerModuleImport(const QString &uri, QTypeRevision version,
                                     const QQmlDirParser::Import &import);
    static void unregisterModuleImport(const QString &uri, QTypeRevision version,
                                       const QQmlDirParser::Import &import);
    static QList<QQmlDirParser::Import> moduleImports(const QString &uri, QTypeRevision version);

    static int typeId(const char *uri, QTypeRevision version, const char *qmlName);

    static void registerUndeletableType(const QQmlType &dtype);

    static QList<QString> qmlTypeNames();
    static QList<QQmlType> qmlTypes();
    static QList<QQmlType> qmlSingletonTypes();
    static QList<QQmlType> qmlAllTypes();

    static QQmlType qmlType(const QString &qualifiedName, QTypeRevision version);
    static QQmlType qmlType(const QHashedStringRef &name, const QHashedStringRef &module, QTypeRevision version);
    static QQmlType qmlType(const QMetaObject *);
    static QQmlType qmlType(const QMetaObject *metaObject, const QHashedStringRef &module, QTypeRevision version);
    static QQmlType qmlTypeById(int qmlTypeId);

    static QQmlType qmlType(QMetaType metaType);
    static QQmlType qmlListType(QMetaType metaType);

    static QQmlType qmlType(const QUrl &unNormalizedUrl);

    static QQmlPropertyCache::ConstPtr propertyCache(
            QObject *object, QTypeRevision version = QTypeRevision());
    static QQmlPropertyCache::ConstPtr propertyCache(
            const QMetaObject *metaObject, QTypeRevision version = QTypeRevision());
    static QQmlPropertyCache::ConstPtr propertyCache(
            const QQmlType &type, QTypeRevision version);

    // These methods may be called from the loader thread
    static QQmlMetaObject rawMetaObjectForType(QMetaType metaType);
    static QQmlMetaObject metaObjectForType(QMetaType metaType);
    static QQmlPropertyCache::ConstPtr propertyCacheForType(QMetaType metaType);
    static QQmlPropertyCache::ConstPtr rawPropertyCacheForType(QMetaType metaType);
    static QQmlPropertyCache::ConstPtr rawPropertyCacheForType(
            QMetaType metaType, QTypeRevision version);

    static bool canConvert(QObject *o, QMetaType metaType);
    static bool canConvert(const QQmlPropertyCache::ConstPtr &from, QMetaType metaType);

    static void freeUnusedTypesAndCaches();

    static QMetaProperty defaultProperty(const QMetaObject *);
    static QMetaProperty defaultProperty(QObject *);
    static QMetaMethod defaultMethod(const QMetaObject *);
    static QMetaMethod defaultMethod(QObject *);

    static QObject *toQObject(const QVariant &, bool *ok = nullptr);

    static QMetaType listValueType(QMetaType type);
    static QQmlAttachedPropertiesFunc attachedPropertiesFunc(QQmlEnginePrivate *,
                                                             const QMetaObject *);
    static bool isInterface(QMetaType type);
    static const char *interfaceIId(QMetaType type);
    static bool isList(QMetaType type);

    static QTypeRevision latestModuleVersion(const QString &uri);
    static bool isStronglyLockedModule(const QString &uri, QTypeRevision version);
    static QTypeRevision matchingModuleVersion(const QString &module, QTypeRevision version);
    static QQmlTypeModule *typeModule(const QString &uri, QTypeRevision version);

    static QList<QQmlPrivate::AutoParentFunction> parentFunctions();

    enum class CachedUnitLookupError {
        NoError,
        NoUnitFound,
        VersionMismatch,
        NotFullyTyped
    };

    enum CacheMode { RejectAll, AcceptUntyped, RequireFullyTyped };
    static const QQmlPrivate::CachedQmlUnit *findCachedCompilationUnit(
            const QUrl &uri, CacheMode mode, CachedUnitLookupError *status);

    // used by tst_qqmlcachegen.cpp
    static void prependCachedUnitLookupFunction(QQmlPrivate::QmlUnitCacheLookupFunction handler);
    static void removeCachedUnitLookupFunction(QQmlPrivate::QmlUnitCacheLookupFunction handler);

    static QString prettyTypeName(const QObject *object);

    template <typename QQmlTypeContainer>
    static void removeQQmlTypePrivate(QQmlTypeContainer &container,
                                      const QQmlTypePrivate *reference)
    {
        for (typename QQmlTypeContainer::iterator it = container.begin(); it != container.end();) {
            if (*it == reference)
                it = container.erase(it);
            else
                ++it;
        }
    }

    template <typename InlineComponentContainer>
    static void removeFromInlineComponents(
        InlineComponentContainer &container, const QQmlTypePrivate *reference)
    {
        const QUrl referenceUrl = QQmlType(reference).sourceUrl();
        for (auto it = container.begin(), end = container.end(); it != end;) {
            if (equalBaseUrls(it.key(), referenceUrl))
                it = container.erase(it);
            else
                ++it;
        }
    }

    static void registerTypeAlias(int typeId, const QString &name);

    static int registerAutoParentFunction(const QQmlPrivate::RegisterAutoParent &autoparent);
    static void unregisterAutoParentFunction(const QQmlPrivate::AutoParentFunction &function);

    static QQmlType registerSequentialContainer(
            const QQmlPrivate::RegisterSequentialContainer &sequenceRegistration);
    static void unregisterSequentialContainer(int id);

    static int registerUnitCacheHook(const QQmlPrivate::RegisterQmlUnitCacheHook &hookRegistration);
    static void clearTypeRegistrations();

    static QList<QQmlProxyMetaObject::ProxyData> proxyData(const QMetaObject *mo,
                                                           const QMetaObject *baseMetaObject,
                                                           QMetaObject *lastMetaObject);

    enum ClonePolicy {
        CloneAll, // default
        CloneEnumsOnly, // skip properties and methods
    };
    static void clone(QMetaObjectBuilder &builder, const QMetaObject *mo,
                      const QMetaObject *ignoreStart, const QMetaObject *ignoreEnd,
                      ClonePolicy policy);

    static void qmlInsertModuleRegistration(const QString &uri, void (*registerFunction)());
    static void qmlRemoveModuleRegistration(const QString &uri);

    static bool qmlRegisterModuleTypes(const QString &uri);

    static bool isValueType(QMetaType type);
    static QQmlValueType *valueType(QMetaType metaType);
    static const QMetaObject *metaObjectForValueType(QMetaType type);

    static QQmlPropertyCache::ConstPtr findPropertyCacheInCompositeTypes(QMetaType t);
    static void registerInternalCompositeType(
            const QQmlRefPointer<QV4::CompiledData::CompilationUnit> &compilationUnit);
    static void unregisterInternalCompositeType(
            const QQmlRefPointer<QV4::CompiledData::CompilationUnit> &compilationUnit);
    static int countInternalCompositeTypeSelfReferences(
            const QQmlRefPointer<QV4::CompiledData::CompilationUnit> &compilationUnit);
    static QQmlRefPointer<QV4::CompiledData::CompilationUnit> obtainCompilationUnit(
            QMetaType type);
    static QQmlRefPointer<QV4::CompiledData::CompilationUnit> obtainCompilationUnit(
            const QUrl &url);
};

Q_DECLARE_TYPEINFO(QQmlMetaType, Q_RELOCATABLE_TYPE);

// used in QQmlListMetaType to tag the metatpye
inline const QMetaObject *dynamicQmlListMarker(const QtPrivate::QMetaTypeInterface *) {
    return nullptr;
};

inline const QMetaObject *dynamicQmlMetaObject(const QtPrivate::QMetaTypeInterface *iface) {
    return QQmlMetaType::metaObjectForType(QMetaType(iface)).metaObject();
};

// metatype interface for composite QML types
struct QQmlMetaTypeInterface : QtPrivate::QMetaTypeInterface
{
    QByteArray name;
    QQmlMetaTypeInterface(QByteArray name)
        : QMetaTypeInterface {
            /*.revision=*/ QMetaTypeInterface::CurrentRevision,
            /*.alignment=*/ alignof(QObject *),
            /*.size=*/ sizeof(QObject *),
            /*.flags=*/ QtPrivate::QMetaTypeForType<QObject *>::flags(),
            /*.typeId=*/ 0,
            /*.metaObjectFn=*/ &dynamicQmlMetaObject,
            /*.name=*/ name.constData(),
            /*.defaultCtr=*/ [](const QMetaTypeInterface *, void *addr) {
                *static_cast<QObject **>(addr) = nullptr;
            },
            /*.copyCtr=*/ [](const QMetaTypeInterface *, void *addr, const void *other) {
                *static_cast<QObject **>(addr) = *static_cast<QObject *const *>(other);
            },
            /*.moveCtr=*/ [](const QMetaTypeInterface *, void *addr, void *other) {
                *static_cast<QObject **>(addr) = *static_cast<QObject **>(other);
            },
            /*.dtor=*/ [](const QMetaTypeInterface *, void *) {},
            /*.equals*/ nullptr,
            /*.lessThan*/ nullptr,
            /*.debugStream=*/ nullptr,
            /*.dataStreamOut=*/ nullptr,
            /*.dataStreamIn=*/ nullptr,
            /*.legacyRegisterOp=*/ nullptr
        }
        , name(std::move(name)) { }
};

// metatype for qml list types
struct QQmlListMetaTypeInterface : QtPrivate::QMetaTypeInterface
{
    QByteArray name;
    // if this interface is for list<type>; valueType stores the interface for type
    const QtPrivate::QMetaTypeInterface *valueType;
    QQmlListMetaTypeInterface(QByteArray name, const QtPrivate::QMetaTypeInterface *valueType)
        : QMetaTypeInterface {
            /*.revision=*/ QMetaTypeInterface::CurrentRevision,
            /*.alignment=*/ alignof(QQmlListProperty<QObject>),
            /*.size=*/ sizeof(QQmlListProperty<QObject>),
            /*.flags=*/ QtPrivate::QMetaTypeForType<QQmlListProperty<QObject>>::flags(),
            /*.typeId=*/ 0,
            /*.metaObjectFn=*/ &dynamicQmlListMarker,
            /*.name=*/ name.constData(),
            /*.defaultCtr=*/ [](const QMetaTypeInterface *, void *addr) {
                new (addr) QQmlListProperty<QObject> ();
            },
            /*.copyCtr=*/ [](const QMetaTypeInterface *, void *addr, const void *other) {
                new (addr) QQmlListProperty<QObject>(
                        *static_cast<const QQmlListProperty<QObject> *>(other));
            },
            /*.moveCtr=*/ [](const QMetaTypeInterface *, void *addr, void *other) {
                new (addr) QQmlListProperty<QObject>(
                        std::move(*static_cast<QQmlListProperty<QObject> *>(other)));
            },
            /*.dtor=*/ [](const QMetaTypeInterface *, void *addr) {
                static_cast<QQmlListProperty<QObject> *>(addr)->~QQmlListProperty<QObject>();
            },
            /*.equals*/ nullptr,
            /*.lessThan*/ nullptr,
            /*.debugStream=*/ nullptr,
            /*.dataStreamOut=*/ nullptr,
            /*.dataStreamIn=*/ nullptr,
            /*.legacyRegisterOp=*/ nullptr
        }
        , name(std::move(name)), valueType(valueType) { }
};

QT_END_NAMESPACE

#endif // QQMLMETATYPE_P_H

