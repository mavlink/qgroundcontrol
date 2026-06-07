// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLTYPELOADERDATA_P_H
#define QQMLTYPELOADERDATA_P_H

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

#include <private/qqmlrefcount_p.h>
#include <private/qqmltypeloaderqmldircontent_p.h>
#include <private/qqmltypeloaderthread_p.h>
#include <private/qv4engine_p.h>

#include <QtQml/qtqmlglobal.h>
#include <QtQml/qqmlengine.h>

#include <QtCore/qcache.h>
#include <QtCore/qthread.h>

QT_BEGIN_NAMESPACE

class QQmlProfiler;
class QQmlQmldirData;
class QQmlScriptBlob;
class QQmlTypeData;

class QQmlTypeLoaderSharedData
{
    Q_DISABLE_COPY_MOVE(QQmlTypeLoaderSharedData)
public:
    enum : int { MinimumTypeCacheTrimThreshold = 64 };

    using TypeCache = QHash<QUrl, QQmlRefPointer<QQmlTypeData>>;
    using ScriptCache = QHash<QUrl, QQmlRefPointer<QQmlScriptBlob>>;
    using QmldirCache = QHash<QUrl, QQmlRefPointer<QQmlQmldirData>>;
    using ImportDirCache = QCache<QString, QCache<QString, bool>>;

    QQmlTypeLoaderSharedData() = default;

    ScriptCache scriptCache;
    TypeCache typeCache;
    QmldirCache qmldirCache;
    ImportDirCache importDirCache;

    int typeCacheTrimThreshold = MinimumTypeCacheTrimThreshold;
};

class QQmlTypeLoaderThreadData
{
    Q_DISABLE_COPY_MOVE(QQmlTypeLoaderThreadData)
public:
    using ChecksumCache = QHash<quintptr, QByteArray>;
    using ImportQmlDirCache = QStringHash<QQmlTypeLoaderQmldirContent *>;

    struct QmldirInfo {
        QTypeRevision version;
        QString qmldirFilePath;
        QString qmldirPathUrl;
        QmldirInfo *next;
    };

    QQmlTypeLoaderThreadData() = default;

    ImportQmlDirCache importQmlDirCache;
    ChecksumCache checksumCache;

    // Maps from an import to a linked list of qmldir info.
    // Used in locateLocalQmldir()
    QStringHash<QmldirInfo *> qmldirInfo;

    // Modules for which plugins have been loaded and processed in the context of this type
    // loader's engine. Plugins can have engine-specific initialization callbacks. This is why
    // we have to keep track of this.
    QSet<QString> modulesForWhichPluginsHaveBeenProcessed;

    // Plugins that have been initialized in the context of this engine. In theory, the same
    // plugin can be used for multiple modules. Therefore, we need to keep track of this
    // separately from modulesForWhichPluginsHaveBeenProcessed.
    QSet<QString> initializedPlugins;

#if QT_CONFIG(qml_network)
    typedef QHash<QNetworkReply *, QQmlDataBlob::Ptr> NetworkReplies;
    NetworkReplies networkReplies;
#endif
};

class QQmlTypeLoaderConfiguredData
{
    Q_DISABLE_COPY_MOVE(QQmlTypeLoaderConfiguredData)
public:
    QQmlTypeLoaderConfiguredData() = default;

    QStringList importPaths;
    QStringList pluginPaths;

    // URL interceptors must be set before loading any types. Otherwise we get data races.
    QList<QQmlAbstractUrlInterceptor *> urlInterceptors;

#if QT_CONFIG(qml_debug)
    QScopedPointer<QQmlProfiler> profiler;
#endif

    QV4::ExecutionEngine::DiskCacheOptions diskCacheOptions
            = QV4::ExecutionEngine::DiskCache::Enabled;
    bool isDebugging = false;
    bool initialized = false;
};

#if QT_CONFIG(qml_network)
class QQmlTypeLoaderNetworkAccessManagerData
{
    Q_DISABLE_COPY_MOVE(QQmlTypeLoaderNetworkAccessManagerData)
public:
    QQmlTypeLoaderNetworkAccessManagerData() = default;

    QQmlNetworkAccessManagerFactory *networkAccessManagerFactory = nullptr;

    // We need a separate mutex because the network access manger factory can be accessed not
    // only from the engine thread and the type loader thread, but also from any WorkerScripts
    // running in parallel to both.
    mutable QMutex networkAccessManagerMutex;
};
#endif // QTCONFIG(qml_network)

class QQmlTypeLoaderLockedData
{
    template<typename Data, typename LockedData>
    friend class QQmlTypeLoaderSharedDataPtrBase;

    template<typename Data, typename LockedData>
    friend class QQmlTypeLoaderThreadDataPtrBase;

    template<typename Data, typename LockedData>
    friend class QQmlTypeLoaderConfiguredDataPtrBase;

    template<typename Data, typename LockedData>
    friend class QQmlNetworkAccessManagerFactoryPtrBase;
    friend class QQmlNetworkAccessManagerFactoryPtr;

    Q_DISABLE_COPY_MOVE(QQmlTypeLoaderLockedData)
public:
    QQmlTypeLoaderLockedData(QQmlEngine *engine);

    QQmlTypeLoaderThread *thread() const { return m_thread; }

    void createThread(QQmlTypeLoader *loader)
    {
        Q_ASSERT(m_engine->thread()->isCurrentThread());
        m_thread = new QQmlTypeLoaderThread(loader);
        m_thread->startup();
    }

    void deleteThread()
    {
        Q_ASSERT(m_engine->thread()->isCurrentThread());
        Q_ASSERT(m_thread);

        // Shut it down first, then set it to nullptr, then delete it.
        // This makes sure that any blobs deleted as part of the deletion
        // do not see the thread anymore.
        m_thread->shutdown();
        delete std::exchange(m_thread, nullptr);
    }

    QQmlEngine *engine() const
    {
        Q_ASSERT(m_engine->thread()->isCurrentThread());
        return m_engine;
    }

private:
    QQmlTypeLoaderSharedData m_sharedData;
    QQmlTypeLoaderThreadData m_threadData;
    QQmlTypeLoaderConfiguredData m_configuredData;
#if QT_CONFIG(qml_network)
    QQmlTypeLoaderNetworkAccessManagerData m_networkAccessManagerData;
#endif

    QQmlEngine *m_engine = nullptr;
    QQmlTypeLoaderThread *m_thread = nullptr;
};

template<typename Data, typename LockedData>
class QQmlTypeLoaderSharedDataPtrBase
{
    Q_DISABLE_COPY_MOVE(QQmlTypeLoaderSharedDataPtrBase)
public:
    Q_NODISCARD_CTOR QQmlTypeLoaderSharedDataPtrBase(LockedData *data) : data(data)
    {
        Q_ASSERT(data);
        if (QQmlTypeLoaderThread *thread = data->thread())
            thread->lock();
    }

    ~QQmlTypeLoaderSharedDataPtrBase()
    {
        Q_ASSERT(data);
        if (QQmlTypeLoaderThread *thread = data->thread())
            thread->unlock();
    }

    Data &operator*() const { return data->m_sharedData; }
    Data *operator->() const { return &data->m_sharedData; }
    operator Data *() const { return &data->m_sharedData; }

private:
    LockedData *data = nullptr;
};

using QQmlTypeLoaderSharedDataPtr
        = QQmlTypeLoaderSharedDataPtrBase<QQmlTypeLoaderSharedData, QQmlTypeLoaderLockedData>;
using QQmlTypeLoaderSharedDataConstPtr
        = QQmlTypeLoaderSharedDataPtrBase<const QQmlTypeLoaderSharedData, const QQmlTypeLoaderLockedData>;

template<typename Data, typename LockedData>
class QQmlTypeLoaderThreadDataPtrBase
{
    Q_DISABLE_COPY_MOVE(QQmlTypeLoaderThreadDataPtrBase)
public:
    Q_NODISCARD_CTOR QQmlTypeLoaderThreadDataPtrBase(LockedData *data) : data(data)
    {
        Q_ASSERT(data);

        // You have to either be on the type loader thread or shut it down before accessing this.
        Q_ASSERT(!data->thread() || data->thread()->isThisThread());
    }

    Data &operator*() const { return data->m_threadData; }
    Data *operator->() const { return &data->m_threadData; }
    operator Data *() const { return &data->m_threadData; }

private:
    LockedData *data = nullptr;
};

using QQmlTypeLoaderThreadDataPtr
        = QQmlTypeLoaderThreadDataPtrBase<QQmlTypeLoaderThreadData, QQmlTypeLoaderLockedData>;
using QQmlTypeLoaderThreadDataConstPtr
        = QQmlTypeLoaderThreadDataPtrBase<const QQmlTypeLoaderThreadData, const QQmlTypeLoaderLockedData>;


template<typename Data, typename LockedData>
class QQmlTypeLoaderConfiguredDataPtrBase
{
    Q_DISABLE_COPY_MOVE(QQmlTypeLoaderConfiguredDataPtrBase)
public:
    Q_NODISCARD_CTOR QQmlTypeLoaderConfiguredDataPtrBase(LockedData *data) : data(data)
    {
        Q_ASSERT(data);

        if constexpr (!std::is_const_v<Data>) {
            // const access is generally fine
            // For mutable access we first need to make sure the thread is shut down.
            if (data->thread())
                data->deleteThread();
        }
    }

    Data &operator*() const { return data->m_configuredData; }
    Data *operator->() const { return &data->m_configuredData; }
    operator Data *() const { return &data->m_configuredData; }

private:
    LockedData *data = nullptr;
};

using QQmlTypeLoaderConfiguredDataPtr
        = QQmlTypeLoaderConfiguredDataPtrBase<QQmlTypeLoaderConfiguredData, QQmlTypeLoaderLockedData>;
using QQmlTypeLoaderConfiguredDataConstPtr
        = QQmlTypeLoaderConfiguredDataPtrBase<const QQmlTypeLoaderConfiguredData, const QQmlTypeLoaderLockedData>;

#if QT_CONFIG(qml_network)
class QQmlEnginePublicAPIToken;
template<typename Data, typename LockedData>
class QQmlNetworkAccessManagerFactoryPtrBase
{
    Q_DISABLE_COPY_MOVE(QQmlNetworkAccessManagerFactoryPtrBase)
public:
    Q_NODISCARD_CTOR QQmlNetworkAccessManagerFactoryPtrBase(LockedData *data) : data(data)
    {
        Q_ASSERT(data);
        data->m_networkAccessManagerData.networkAccessManagerMutex.lock();
    }

    ~QQmlNetworkAccessManagerFactoryPtrBase()
    {
        Q_ASSERT(data);
        data->m_networkAccessManagerData.networkAccessManagerMutex.unlock();
    }

    QQmlNetworkAccessManagerFactory &operator*() const
    {
        return *data->m_networkAccessManagerData.networkAccessManagerFactory;
    }

    QQmlNetworkAccessManagerFactory *operator->() const
    {
        return data->m_networkAccessManagerData.networkAccessManagerFactory;
    }

    operator bool() const
    {
        return data->m_networkAccessManagerData.networkAccessManagerFactory != nullptr;
    }

    // This is dangerous since it allows you to subvert the locking.
    // You must only do this when serving public API that can't be changed for now.
    QQmlNetworkAccessManagerFactory *get(const QQmlEnginePublicAPIToken &) const
    {
        return data->m_networkAccessManagerData.networkAccessManagerFactory;
    }

protected:
    LockedData *data = nullptr;
};

// This is const in the sense that you cannot reset the pointer. Therefore the "Const" postfix.
// That's in contrast to the other *Ptr classes here that are const in the way that the _data_
// pointed to cannot be modified.
using QQmlNetworkAccessManagerFactoryPtrConst
        = QQmlNetworkAccessManagerFactoryPtrBase<const QQmlTypeLoaderNetworkAccessManagerData, const QQmlTypeLoaderLockedData>;

class QQmlNetworkAccessManagerFactoryPtr
    : public QQmlNetworkAccessManagerFactoryPtrBase<QQmlTypeLoaderNetworkAccessManagerData, QQmlTypeLoaderLockedData>
{
    Q_DISABLE_COPY(QQmlNetworkAccessManagerFactoryPtr)
public:
    Q_NODISCARD_CTOR QQmlNetworkAccessManagerFactoryPtr(QQmlTypeLoaderLockedData *lockedData)
        : QQmlNetworkAccessManagerFactoryPtrBase<QQmlTypeLoaderNetworkAccessManagerData, QQmlTypeLoaderLockedData>(lockedData)
    {
    }

    void reset(QQmlNetworkAccessManagerFactory *factory)
    {
        data->m_networkAccessManagerData.networkAccessManagerFactory = factory;
    }
};
#endif // QT_CONFIG(qml_network)

QT_END_NAMESPACE

#endif // QQMLTYPELOADERDATA_P_H
