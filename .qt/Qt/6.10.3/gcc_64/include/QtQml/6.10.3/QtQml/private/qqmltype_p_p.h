// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLTYPE_P_P_H
#define QQMLTYPE_P_P_H

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

#include <private/qqmlengine_p.h>
#include <private/qqmlmetatype_p.h>
#include <private/qqmlpropertycache_p.h>
#include <private/qqmlproxymetaobject_p.h>
#include <private/qqmlrefcount_p.h>
#include <private/qqmltype_p.h>
#include <private/qqmltypeloader_p.h>
#include <private/qstringhash_p.h>
#include <private/qv4engine_p.h>
#include <private/qv4executablecompilationunit_p.h>
#include <private/qv4resolvedtypereference_p.h>

#include <QAtomicInteger>

QT_BEGIN_NAMESPACE

class QQmlTypePrivate final : public QQmlRefCounted<QQmlTypePrivate>
{
    Q_DISABLE_COPY_MOVE(QQmlTypePrivate)
public:
    struct ProxyMetaObjects
    {
        ~ProxyMetaObjects()
        {
            for (const QQmlProxyMetaObject::ProxyData &metaObject : data)
                free(metaObject.metaObject);
        }

        QList<QQmlProxyMetaObject::ProxyData> data;
        bool containsRevisionedAttributes = false;
    };

    struct Enums
    {
        enum Scoping { Scoped, Unscoped };
        ~Enums()
        {
            qDeleteAll(scopedEnums);
            qDeleteAll(unscopedEnums);
        }

        QStringHash<int> enums;
        QStringHash<int> scopedEnumIndex; // maps from enum name to index in scopedEnums
        QStringHash<int> unscopedEnumIndex; // maps from enum name to index in unscopedEnums
        QList<QStringHash<int> *> scopedEnums;
        QList<QStringHash<int> *> unscopedEnums;
    };

    QQmlTypePrivate(QQmlType::RegistrationType type);

    const ProxyMetaObjects *init() const;

    QUrl sourceUrl() const
    {
        switch (regType) {
        case QQmlType::CompositeType:
            return extraData.compositeTypeData;
        case QQmlType::CompositeSingletonType:
            return extraData.singletonTypeData->singletonInstanceInfo->url;
        case QQmlType::InlineComponentType:
            return extraData.inlineComponentTypeData;
        case QQmlType::JavaScriptType:
            return extraData.javaScriptTypeData;
        default:
            return QUrl();
        }
    }

    const QQmlTypePrivate *attachedPropertiesBase(QQmlTypeLoader *typeLoader) const
    {
        for (const QQmlTypePrivate *d = this; d;
             d = d->resolveCompositeBaseType(typeLoader).d.data()) {
            if (d->regType == QQmlType::CppType)
                return d->extraData.cppTypeData->attachedPropertiesType ? d : nullptr;

            if (d->regType != QQmlType::CompositeType)
                return nullptr;
        }
        return nullptr;
    }

    bool isComposite() const
    {
        return regType == QQmlType::CompositeType || regType == QQmlType::CompositeSingletonType;
    }

    bool isValueType() const
    {
        return regType == QQmlType::CppType && !(typeId.flags() & QMetaType::PointerToQObject);
    }

    QQmlType resolveCompositeBaseType(QQmlTypeLoader *typeLoader) const;
    QQmlPropertyCache::ConstPtr compositePropertyCache(QQmlTypeLoader *typeLoader) const;

    struct QQmlCppTypeData
    {
        int allocationSize;
        void (*newFunc)(void *, void *);
        void *userdata = nullptr;
        QString noCreationReason;
        QVariant (*createValueTypeFunc)(const QJSValue &);
        int parserStatusCast;
        QObject *(*extFunc)(QObject *);
        const QMetaObject *extMetaObject;
        QQmlCustomParser *customParser;
        QQmlAttachedPropertiesFunc attachedPropertiesFunc;
        const QMetaObject *attachedPropertiesType;
        int propertyValueSourceCast;
        int propertyValueInterceptorCast;
        int finalizerCast;
        bool registerEnumClassesUnscoped;
        bool registerEnumsFromRelatedTypes;
        bool constructValueType;
        bool populateValueType;
    };

    struct QQmlSingletonTypeData
    {
        QQmlType::SingletonInstanceInfo::ConstPtr singletonInstanceInfo;
        QObject *(*extFunc)(QObject *);
        const QMetaObject *extMetaObject;
    };

    int index = -1;

    union extraData {
        extraData() {}  // QQmlTypePrivate() does the actual construction.
        ~extraData() {} // ~QQmlTypePrivate() does the actual destruction.

        QQmlCppTypeData *cppTypeData;
        QQmlSingletonTypeData *singletonTypeData;
        QUrl compositeTypeData;
        QUrl javaScriptTypeData;
        QUrl inlineComponentTypeData;
        QMetaSequence sequentialContainerTypeData;
        const char *interfaceTypeData;
    } extraData;
    static_assert(sizeof(extraData) == sizeof(void *));

    QHashedString module;
    QString name;
    QString elementName;
    QMetaType typeId;
    QMetaType listId;
    QQmlType::RegistrationType regType;
    QTypeRevision version;
    QTypeRevision revision = QTypeRevision::zero();
    const QMetaObject *baseMetaObject = nullptr;

    void setName(const QString &uri, const QString &element);

    template<typename String>
    static int enumValue(
            const QQmlRefPointer<const QQmlTypePrivate> &d, QQmlTypeLoader *typeLoader,
            const String &name, bool *ok)
    {
        const auto *rv = doGetEnumOp<const int *>(
                d, typeLoader, [&](const QQmlTypePrivate::Enums *enums) {
            return enums->enums.value(name);
        }, [](const int *p) { return !!p; }, ok);
        return rv ? *rv : -1;
    }

    template<Enums::Scoping scoping, typename String>
    static int enumIndex(
            const QQmlRefPointer<const QQmlTypePrivate> &d, QQmlTypeLoader *typeLoader,
            const String &name, bool *ok)
    {
        const auto *rv = doGetEnumOp<const int *> (
                d, typeLoader, [&](const QQmlTypePrivate::Enums *enums) {
            if constexpr (scoping == Enums::Scoped)
                return enums->scopedEnumIndex.value(name);
            else
                return enums->unscopedEnumIndex.value(name);
        }, [](const int *p) { return !!p; }, ok);
        return rv ? *rv : -1;
    }

    template<Enums::Scoping scoping, typename String>
    static int enumValue(
            const QQmlRefPointer<const QQmlTypePrivate> &d, QQmlTypeLoader *typeLoader, int index,
            const String &name, bool *ok)
    {
        const auto *rv = doGetEnumOp<const int *>(
                d, typeLoader, [&](const QQmlTypePrivate::Enums *enums) {
            if constexpr (scoping == Enums::Scoped) {
                Q_ASSERT(index > -1 && index < enums->scopedEnums.size());
                return enums->scopedEnums.at(index)->value(name);
            } else {
                Q_ASSERT(index > -1 && index < enums->unscopedEnums.size());
                return enums->unscopedEnums.at(index)->value(name);
            }
        }, [](const int *p) { return !!p; }, ok);
        return rv ? *rv : -1;
    }

    template<Enums::Scoping scoping, typename String1, typename String2>
    static int enumValue(
            const QQmlRefPointer<const QQmlTypePrivate> &d, QQmlTypeLoader *typeLoader,
            const String1 &scopedEnumName, const String2 &name, bool *ok)
    {
        const auto *rv = doGetEnumOp<const int *>(
                d, typeLoader, [&](const QQmlTypePrivate::Enums *enums) {
            const QStringHash<int> *enumIndex;
            const QList<QStringHash<int> *> *_enums;
            if constexpr (scoping == Enums::Scoped) {
                enumIndex = &enums->scopedEnumIndex;
                _enums = &enums->scopedEnums;
            } else {
                enumIndex = &enums->unscopedEnumIndex;
                _enums = &enums->unscopedEnums;
            }

            const int *rv = enumIndex->value(scopedEnumName);
            if (!rv)
                return static_cast<int *>(nullptr);

            const int index = *rv;
            Q_ASSERT(index > -1 && index < _enums->size());
            return _enums->at(index)->value(name);
        }, [](const int *p) { return !!p; }, ok);
        return rv ? *rv : -1;
    }

    template<Enums::Scoping scoping>
    static QString enumKey(
            const QQmlRefPointer<const QQmlTypePrivate> &d, QQmlTypeLoader *typeLoader,
            int index, int value, bool *ok)
    {
        return doGetEnumOp<QString>(d, typeLoader, [&](const QQmlTypePrivate::Enums *enums) {
            const QList<QStringHash<int> *> *_enums;
            if constexpr (scoping == Enums::Scoped)
                _enums = &enums->scopedEnums;
            else
                _enums = &enums->unscopedEnums;

            Q_ASSERT(index > -1 && index < _enums->size());
            const auto hash = _enums->at(index);
            for (auto it = hash->constBegin(), end = hash->constEnd(); it != end; ++it) {
                if (it.value() == value)
                    return QString(it.key());
            }
            return QString();
        }, [](const QString &s) { return !s.isEmpty(); }, ok);
    }

    template<Enums::Scoping scoping>
    static QStringList enumKeys(
            const QQmlRefPointer<const QQmlTypePrivate> &d, QQmlTypeLoader *typeLoader,
            int index, int value, bool *ok)
    {
        return doGetEnumOp<QStringList>(d, typeLoader, [&](const QQmlTypePrivate::Enums *enums) {
            const QList<QStringHash<int> *> *_enums;
            if constexpr (scoping == Enums::Scoped)
                _enums = &enums->scopedEnums;
            else
                _enums = &enums->unscopedEnums;

            Q_ASSERT(index > -1 && index < _enums->size());
            QStringList keys;
            const auto hash = _enums->at(index);
            for (auto it = hash->constBegin(), end = hash->constEnd(); it != end; ++it) {
                if (it.value() == value)
                    keys.append(QString(it.key()));
            }
            std::reverse(keys.begin(), keys.end());
            return keys;
        }, [](const QStringList &l) { return !l.empty(); }, ok);
    }

    const QMetaObject *metaObject() const
    {
        if (isValueType())
            return metaObjectForValueType();

        const QQmlTypePrivate::ProxyMetaObjects *proxies = init();
        return proxies->data.isEmpty()
                ? baseMetaObject
                : proxies->data.constFirst().metaObject;
    }

    const QMetaObject *metaObjectForValueType() const
    {
        Q_ASSERT(isValueType());

        // Prefer the extension meta object, if any.
        // Extensions allow registration of non-gadget value types.
        if (const QMetaObject *extensionMetaObject = extraData.cppTypeData->extMetaObject) {
            // This may be a namespace even if the original metaType isn't.
            // You can do such things with QML_FOREIGN declarations.
            if (extensionMetaObject->metaType().flags() & QMetaType::IsGadget)
                return extensionMetaObject;
        }

        if (baseMetaObject) {
            // This may be a namespace even if the original metaType isn't.
            // You can do such things with QML_FOREIGN declarations.
            if (baseMetaObject->metaType().flags() & QMetaType::IsGadget)
                return baseMetaObject;
        }

        return nullptr;
    }

    static QQmlType visibleQmlTypeByName(
            const QQmlRefPointer<QV4::CompiledData::CompilationUnit> &unit,
            const QString &elementName, QQmlTypeLoader *typeLoader)
    {
        const QQmlType qmltype = unit->typeNameCache->query<QQmlImport::AllowRecursion>(
                                                            elementName, typeLoader).type;

        if (qmltype.isValid() && qmltype.isInlineComponentType()
                && !QQmlMetaType::obtainCompilationUnit(qmltype.typeId())) {
            // If it seems to be an IC type, make sure there is an actual
            // compilation unit for it. We create inline component types speculatively.
            return QQmlType();
        }

        return qmltype;
    }

    // Tries the base unit's resolvedTypes first. If successful, that is cheap
    // because it's just a hash. Otherwise falls back to typeNameCache.
    // typeNameCache is slower because it will do a generic type search on all imports.
    // This can involve iterating all the types of an import or querying QQmlMetaType for
    // further details.
    // TODO: Not all referenced types are pre-resolved when loading. That should be fixed.
    //       In particular, types only used in function signatures are not resolved.
    static QQmlType visibleQmlTypeByName(
            const QV4::ExecutableCompilationUnit *unit, int elementNameId,
            QQmlTypeLoader *typeLoader = nullptr)
    {
        const auto &base = unit->baseCompilationUnit();
        const auto it = base->resolvedTypes.constFind(elementNameId);
        if (it == base->resolvedTypes.constEnd()) {
            return visibleQmlTypeByName(
                    base, base->stringAt(elementNameId),
                    typeLoader ? typeLoader : unit->engine->typeLoader());
        }

        if (const QQmlType type = (*it)->type(); type.isValid())
            return type;

        if (const auto cu = (*it)->compilationUnit())
            return cu->qmlType;

        return QQmlType();
    }

private:
    mutable QAtomicPointer<const ProxyMetaObjects> proxyMetaObjects;
    mutable QAtomicPointer<const Enums> enums;

    ~QQmlTypePrivate();
    friend class QQmlRefCounted<QQmlTypePrivate>;

    struct EnumInfo {
        QStringList path;
        QString metaObjectName;
        QString enumName;
        QString enumKey;
        QString metaEnumScope;
        bool scoped;
    };

    template<typename Ret, typename Op, typename Check>
    static Ret doGetEnumOp(const QQmlRefPointer<const QQmlTypePrivate> &d,
                           QQmlTypeLoader *typeLoader, Op &&op, Check &&check, bool *ok)
    {
        Q_ASSERT(ok);
        if (d) {
            if (const QQmlTypePrivate::Enums *enums = d->initEnums(typeLoader)) {
                if (Ret rv = op(enums); check(rv)) {
                    *ok = true;
                    return rv;
                }
            }
        }

        *ok = false;
        return Ret();
    }

    const Enums *initEnums(QQmlTypeLoader *typeLoader) const;
    void insertEnums(Enums *enums, const QMetaObject *metaObject) const;
    void insertEnumsFromPropertyCache(Enums *enums, const QQmlPropertyCache::ConstPtr &cache) const;

    void createListOfPossibleConflictingItems(const QMetaObject *metaObject, QList<EnumInfo> &enumInfoList, QStringList path) const;
    void createEnumConflictReport(const QMetaObject *metaObject, const QString &conflictingKey) const;
};

QT_END_NAMESPACE

#endif // QQMLTYPE_P_P_H
