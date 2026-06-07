// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLMETATYPEDATA_P_H
#define QQMLMETATYPEDATA_P_H

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

#include <private/qqmltype_p.h>
#include <private/qqmlmetatype_p.h>
#include <private/qhashedstring_p.h>
#include <private/qqmlvaluetype_p.h>

#include <QtCore/qset.h>
#include <QtCore/qvector.h>

QT_BEGIN_NAMESPACE

class QQmlTypePrivate;
struct QQmlMetaTypeData
{
    QQmlMetaTypeData();
    ~QQmlMetaTypeData();
    void registerType(QQmlTypePrivate *priv);
    QList<QQmlType> types;
    QSet<QQmlType> undeletableTypes;
    typedef QHash<int, QQmlTypePrivate *> Ids;
    Ids idToType;

    using Names = QMultiHash<QHashedString, const QQmlTypePrivate *>;
    Names nameToType;

    typedef QHash<QUrl, const QQmlTypePrivate *> Files; //For file imported composite types only
    Files urlToType;

    typedef QMultiHash<const QMetaObject *, QQmlTypePrivate *> MetaObjects;
    MetaObjects metaObjectToType;
    QVector<QHash<QTypeRevision, QQmlPropertyCache::ConstPtr>> typePropertyCaches;
    QHash<int, QQmlValueType *> metaTypeToValueType;

    // This has to be a multihash because a user can create a compilation unit using arbitrary
    // static data, via e.g. QQmlComponent::setData() or Qt.createQmlObject(). Since you can also
    // freely choose the URL and therefore the metatype for those, we can end up we can end up
    // with multiple compilation units per URL. Some compilation units need special handling on
    // removal. Therefore we need to hold on to all of them.
    using CompositeTypes = QMultiHash<const QtPrivate::QMetaTypeInterface *,
                                      QQmlRefPointer<QV4::CompiledData::CompilationUnit>>;
    CompositeTypes compositeTypes;

    struct VersionedUri {
        VersionedUri() = default;
        VersionedUri(const QString &uri, QTypeRevision version)
            : uri(uri), majorVersion(version.majorVersion()) {}
        VersionedUri(const std::unique_ptr<QQmlTypeModule> &module);

        friend bool operator==(const VersionedUri &a, const VersionedUri &b)
        {
            return a.majorVersion == b.majorVersion && a.uri == b.uri;
        }

        friend size_t qHash(const VersionedUri &v, size_t seed = 0)
        {
            return qHashMulti(seed, v.uri, v.majorVersion);
        }

        friend bool operator<(const QQmlMetaTypeData::VersionedUri &a,
                              const QQmlMetaTypeData::VersionedUri &b)
        {
            const int diff = a.uri.compare(b.uri);
            return diff < 0 || (diff == 0 && a.majorVersion < b.majorVersion);
        }

        QString uri;
        quint8 majorVersion = 0;
    };

    typedef std::vector<std::unique_ptr<QQmlTypeModule>> TypeModules;
    TypeModules uriToModule;
    QQmlTypeModule *findTypeModule(const QString &module, QTypeRevision version);
    QQmlTypeModule *addTypeModule(std::unique_ptr<QQmlTypeModule> module);

    using ModuleImports = QMultiMap<VersionedUri, QQmlDirParser::Import>;
    ModuleImports moduleImports;

    QHash<QString, void (*)()> moduleTypeRegistrationFunctions;
    bool registerModuleTypes(const QString &uri);

    QSet<int> interfaces;

    QList<QQmlPrivate::AutoParentFunction> parentFunctions;
    QVector<QQmlPrivate::QmlUnitCacheLookupFunction> lookupCachedQmlUnit;

    QHash<const QMetaObject *, QQmlPropertyCache::ConstPtr> propertyCaches;

    struct CompositeMetaTypes
    {
        QQmlMetaTypeInterface *type = nullptr;
        QQmlListMetaTypeInterface *listType = nullptr;
    };
    QHash<QUrl, CompositeMetaTypes> compositeMetaTypes;

    QQmlPropertyCache::ConstPtr propertyCacheForVersion(int index, QTypeRevision version) const;
    void setPropertyCacheForVersion(
            int index, QTypeRevision version, const QQmlPropertyCache::ConstPtr &cache);
    void clearPropertyCachesForVersion(int index);

    QQmlPropertyCache::ConstPtr propertyCache(const QMetaObject *metaObject, QTypeRevision version);
    QQmlPropertyCache::ConstPtr propertyCache(const QQmlType &type, QTypeRevision version);
    QQmlPropertyCache::ConstPtr findPropertyCacheInCompositeTypes(QMetaType t) const;

    static QQmlPropertyCache::ConstPtr propertyCacheForPotentialInlineComponentType(
            QMetaType t, const QQmlMetaTypeData::CompositeTypes::const_iterator &iter);

    void setTypeRegistrationFailures(QStringList *failures)
    {
        m_typeRegistrationFailures = failures;
    }

    void recordTypeRegFailure(const QString &message)
    {
        if (m_typeRegistrationFailures)
            m_typeRegistrationFailures->append(message);
        else
            qWarning("%s", message.toUtf8().constData());
    }

    void clearCompositeMetaTypes();
private:
    QStringList *m_typeRegistrationFailures = nullptr;
};

QT_END_NAMESPACE

#endif // QQMLMETATYPEDATA_P_H
