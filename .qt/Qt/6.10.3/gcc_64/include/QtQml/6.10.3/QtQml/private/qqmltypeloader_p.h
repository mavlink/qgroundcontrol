// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLTYPELOADER_P_H
#define QQMLTYPELOADER_P_H

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

#include <private/qqmldatablob_p.h>
#include <private/qqmlimport_p.h>
#include <private/qqmlmetatype_p.h>
#include <private/qqmltypeloaderdata_p.h>
#include <private/qqmltypeloaderthread_p.h>
#include <private/qv4compileddata_p.h>
#include <private/qv4engine_p.h>

#include <QtQml/qtqmlglobal.h>
#include <QtQml/qqmlerror.h>

#include <memory>

QT_BEGIN_NAMESPACE

class QQmlEngine;
class QQmlEngineExtensionInterface;
class QQmlExtensionInterface;
class QQmlNetworkAccessManagerFactory;
class QQmlProfiler;
class QQmlQmldirData;
class QQmlScriptBlob;
class QQmlTypeData;
class QQmlTypeLoaderThread;

class Q_QML_EXPORT QQmlTypeLoader
{
    Q_DECLARE_TR_FUNCTIONS(QQmlTypeLoader)
public:
    using ChecksumCache = QQmlTypeLoaderThreadData::ChecksumCache;
    enum Mode { PreferSynchronous, Asynchronous, Synchronous };

    class Q_QML_EXPORT Blob : public QQmlDataBlob
    {
    public:
        Blob(const QUrl &url, QQmlDataBlob::Type type, QQmlTypeLoader *loader);
        ~Blob() override;

        const QQmlImports *imports() const { return m_importCache.data(); }

        void setCachedUnitStatus(QQmlMetaType::CachedUnitLookupError status) { m_cachedUnitStatus = status; }

        struct PendingImport
        {
            QString uri;
            QString qualifier;

            QV4::CompiledData::Import::ImportType type
                = QV4::CompiledData::Import::ImportType::ImportLibrary;
            QV4::CompiledData::Location location;

            QQmlImports::ImportFlags flags;
            quint8 precedence = 0;
            int priority = 0;

            QTypeRevision version;

            PendingImport() = default;
            PendingImport(const QQmlRefPointer<Blob> &blob, const QV4::CompiledData::Import *import,
                          QQmlImports::ImportFlags flags);
        };
        using PendingImportPtr = std::shared_ptr<PendingImport>;

        void importQmldirScripts(const PendingImportPtr &import, const QQmlTypeLoaderQmldirContent &qmldir, const QUrl &qmldirUrl);
        bool handleLocalQmldirForImport(
                const PendingImportPtr &import, const QString &qmldirFilePath,
                const QString &qmldirUrl, QList<QQmlError> *errors);

    protected:
        bool addImport(const QV4::CompiledData::Import *import, QQmlImports::ImportFlags,
                       QList<QQmlError> *errors);
        bool addImport(const PendingImportPtr &import, QList<QQmlError> *errors);

        bool fetchQmldir(
                const QUrl &url, const PendingImportPtr &import, int priority,
                QList<QQmlError> *errors);
        bool updateQmldir(const QQmlRefPointer<QQmlQmldirData> &data, const PendingImportPtr &import, QList<QQmlError> *errors);

    private:
        bool addScriptImport(const PendingImportPtr &import);
        bool addFileImport(const PendingImportPtr &import, QList<QQmlError> *errors);
        bool addLibraryImport(const PendingImportPtr &import, QList<QQmlError> *errors);

        virtual bool qmldirDataAvailable(const QQmlRefPointer<QQmlQmldirData> &, QList<QQmlError> *);

        virtual void scriptImported(
                const QQmlRefPointer<QQmlScriptBlob> &, const QV4::CompiledData::Location &,
                const QString &, const QString &)
        {
            assertTypeLoaderThread();
        }

        void dependencyComplete(const QQmlDataBlob::Ptr &) override;

        bool loadImportDependencies(
                const PendingImportPtr &currentImport, const QString &qmldirUri,
                QQmlImports::ImportFlags flags, QList<QQmlError> *errors);

    protected:

        bool registerPendingTypes(const PendingImportPtr &import);

        bool loadDependentImports(
                const QList<QQmlDirParser::Import> &imports, const QString &qualifier,
                QTypeRevision version, quint16 precedence, QQmlImports::ImportFlags flags,
                QList<QQmlError> *errors);
        virtual QString stringAt(int) const { return QString(); }

        QQmlRefPointer<QQmlImports> m_importCache;
        QVector<PendingImportPtr> m_unresolvedImports;
        QVector<QQmlRefPointer<QQmlQmldirData>> m_qmldirs;
        QQmlMetaType::CachedUnitLookupError m_cachedUnitStatus = QQmlMetaType::CachedUnitLookupError::NoError;
    };

    QQmlTypeLoader(QQmlEngine *);
    ~QQmlTypeLoader();

    template<
            typename Engine,
            typename EnginePrivate = QQmlEnginePrivate,
            typename = std::enable_if_t<std::is_same_v<Engine, QQmlEngine>>>
    static QQmlTypeLoader *get(Engine *engine)
    {
        return get(EnginePrivate::get(engine));
    }

    template<
            typename Engine,
            typename = std::enable_if_t<std::is_same_v<Engine, QQmlEnginePrivate>>>
    static QQmlTypeLoader *get(Engine *engine)
    {
        return &engine->typeLoader;
    }

    static void sanitizeUNCPath(QString *path)
    {
        // This handles the UNC path case as when the path is retrieved from the QUrl it
        // will convert the host name from upper case to lower case. So the absoluteFilePath
        // is changed at this point to make sure it will match later on in that case.
        if (path->startsWith(QStringLiteral("//"))) {
            // toLocalFile() since that faithfully restores all the things you can do to a
            // path but not a URL, in particular weird characters like '%'.
            *path = QUrl::fromLocalFile(*path).toLocalFile();
        }
    }

    // We can't include QQmlTypeData here.
    // Use a template specialized only for QQmlTypeData::TypeReference instead.
    template<typename TypeReference>
    QByteArray hashDependencies(
            QV4::CompiledData::ResolvedTypeReferenceMap *resolvedTypeCache,
            const QList<TypeReference> &compositeSingletons)
    {
        QQmlTypeLoaderThreadDataPtr data(&m_data);

        QCryptographicHash hash(QCryptographicHash::Md5);
        return (resolvedTypeCache->addToHash(&hash, &data->checksumCache)
                    && addTypeReferenceChecksumsToHash(
                        compositeSingletons, &data->checksumCache, &hash))
                ? hash.result()
                : QByteArray();
    }

    static QUrl normalize(const QUrl &unNormalizedUrl);

    QQmlRefPointer<QQmlTypeData> getType(
            const QUrl &unNormalizedUrl, Mode mode = PreferSynchronous);
    QQmlRefPointer<QQmlTypeData> getType(
            const QByteArray &data, const QUrl &url, Mode mode = PreferSynchronous);

    QQmlRefPointer<QV4::CompiledData::CompilationUnit> injectModule(
            const QUrl &relativeUrl, const QV4::CompiledData::Unit *unit);

    QQmlRefPointer<QQmlScriptBlob> getScript(const QUrl &unNormalizedUrl, const QUrl &relativeUrl);
    QQmlRefPointer<QQmlQmldirData> getQmldir(const QUrl &);

    QString absoluteFilePath(const QString &path);
    bool fileExists(const QString &path, const QString &file);
    bool directoryExists(const QString &path);

    const QQmlTypeLoaderQmldirContent qmldirContent(const QString &filePath);
    void setQmldirContent(const QString &filePath, const QString &content);

    void clearCache();
    void trimCache();

    bool isTypeLoaded(const QUrl &url) const;
    bool isScriptLoaded(const QUrl &url) const;

    void loadWithStaticData(
            const QQmlDataBlob::Ptr &blob, const QByteArray &data, Mode mode = PreferSynchronous);

    void drop(const QQmlDataBlob::Ptr &blob);

    void initializeEngine(QQmlEngineExtensionInterface *, const char *);
    void initializeEngine(QQmlExtensionInterface *, const char *);
    void invalidate();

    void addUrlInterceptor(QQmlAbstractUrlInterceptor *urlInterceptor);
    void removeUrlInterceptor(QQmlAbstractUrlInterceptor *urlInterceptor);
    QList<QQmlAbstractUrlInterceptor *> urlInterceptors() const;
    QUrl interceptUrl(const QUrl &url, QQmlAbstractUrlInterceptor::DataType type) const;
    bool hasUrlInterceptors() const;

#if !QT_CONFIG(qml_debug)
    quintptr profiler() const { return 0; }
    void setProfiler(quintptr) {}
#else
    QQmlProfiler *profiler() const
    {
        QQmlTypeLoaderConfiguredDataConstPtr data(&m_data);
        return data->profiler.data();
    }
    void setProfiler(QQmlProfiler *profiler);
#endif // QT_CONFIG(qml_debug)

    QStringList importPathList() const
    {
        QQmlTypeLoaderConfiguredDataConstPtr data(&m_data);
        return data->importPaths;
    }
    void setImportPathList(const QStringList &paths);
    void addImportPath(const QString& dir);

    QStringList pluginPathList() const
    {
        QQmlTypeLoaderConfiguredDataConstPtr data(&m_data);
        return data->pluginPaths;
    }
    void setPluginPathList(const QStringList &paths);
    void addPluginPath(const QString& path);

    void setPluginInitialized(const QString &plugin)
    {
        QQmlTypeLoaderThreadDataPtr data(&m_data);
        data->initializedPlugins.insert(plugin);
    }
    bool isPluginInitialized(const QString &plugin) const
    {
        QQmlTypeLoaderThreadDataConstPtr data(&m_data);
        return data->initializedPlugins.contains(plugin);
    }

    void setModulePluginProcessingDone(const QString &module)
    {
        QQmlTypeLoaderThreadDataPtr data(&m_data);
        data->modulesForWhichPluginsHaveBeenProcessed.insert(module);
    }
    bool isModulePluginProcessingDone(const QString &module)
    {
        QQmlTypeLoaderThreadDataConstPtr data(&m_data);
        return data->modulesForWhichPluginsHaveBeenProcessed.contains(module);
    }

#if QT_CONFIG(qml_network)
    QQmlNetworkAccessManagerFactoryPtrConst networkAccessManagerFactory() const;
    void setNetworkAccessManagerFactory(QQmlNetworkAccessManagerFactory *factory);
    QNetworkAccessManager *createNetworkAccessManager(QObject *parent) const;
#endif

    bool writeCacheFile();
    bool readCacheFile();
    bool isDebugging();

private:
    friend struct PlainLoader;
    friend struct CachedLoader;
    friend struct StaticLoader;

    friend class QQmlDataBlob;
    friend class QQmlTypeLoaderThread;
#if QT_CONFIG(qml_network)
    friend class QQmlTypeLoaderNetworkReplyProxy;
#endif // qml_network

    enum PathType { Local, Remote, LocalOrRemote };

    enum LocalQmldirResult {
        QmldirFound,
        QmldirNotFound,
        QmldirInterceptedToRemote,
        QmldirRejected
    };

    QQmlTypeLoaderThread *thread() const { return m_data.thread(); }
    QQmlEngine *engine() const { return m_data.engine(); }

    void startThread();
    void shutdownThread();
    QQmlTypeLoaderThread *ensureThread()
    {
        if (!thread())
            startThread();
        return thread();
    }

    void trimCache(const QQmlTypeLoaderSharedDataPtr &data);

    void loadThread(const QQmlDataBlob::Ptr &);
    void loadWithStaticDataThread(const QQmlDataBlob::Ptr &, const QByteArray &);
    void loadWithCachedUnitThread(const QQmlDataBlob::Ptr &blob, const QQmlPrivate::CachedQmlUnit *unit);
#if QT_CONFIG(qml_network)
    void networkReplyFinished(QNetworkReply *);
    void networkReplyProgress(QNetworkReply *, qint64, qint64);
#endif

    void setData(const QQmlDataBlob::Ptr &, const QByteArray &);
    void setData(const QQmlDataBlob::Ptr &, const QString &fileName);
    void setData(const QQmlDataBlob::Ptr &, const QQmlDataBlob::SourceCodeData &);
    void setCachedUnit(const QQmlDataBlob::Ptr &blob, const QQmlPrivate::CachedQmlUnit *unit);

    QStringList importPathList(PathType type) const;
    void clearQmldirInfo();

    LocalQmldirResult locateLocalQmldir(
            QQmlTypeLoader::Blob *blob, const QQmlTypeLoader::Blob::PendingImportPtr &import,
            QList<QQmlError> *errors);

    void load(const QQmlDataBlob::Ptr &blob, Mode mode = PreferSynchronous);
    void loadWithCachedUnit(
            const QQmlDataBlob::Ptr &blob, const QQmlPrivate::CachedQmlUnit *unit,
            Mode mode = PreferSynchronous);

    template<typename Loader>
    void doLoad(const Loader &loader, const QQmlDataBlob::Ptr &blob, Mode mode);
    void updateTypeCacheTrimThreshold(const QQmlTypeLoaderSharedDataPtr &data);

    template<typename TypeReference>
    static bool addTypeReferenceChecksumsToHash(
            const QList<TypeReference> &typeRefs,
            QHash<quintptr, QByteArray> *checksums, QCryptographicHash *hash)
    {
        for (const auto &typeRef: typeRefs) {
            if (typeRef.typeData) {
                const auto unit = typeRef.typeData->compilationUnit()->unitData();
                hash->addData({unit->md5Checksum, sizeof(unit->md5Checksum)});
            } else if (const QMetaObject *mo = typeRef.type.metaObject()) {
                const auto propertyCache = QQmlMetaType::propertyCache(mo);
                bool ok = false;
                hash->addData(propertyCache->checksum(checksums, &ok));
                if (!ok)
                    return false;
            }
        }
        return true;
    }

    QQmlMetaType::CacheMode aotCacheMode();

    QQmlTypeLoaderLockedData m_data;
};

QT_END_NAMESPACE

#endif // QQMLTYPELOADER_P_H
