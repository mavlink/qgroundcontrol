// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QFILEINFO_P_H
#define QFILEINFO_P_H

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

#include "qfileinfo.h"
#include "qdatetime.h"
#include "qatomic.h"
#include "qshareddata.h"
#include "qfilesystemengine_p.h"

#include <QtCore/private/qabstractfileengine_p.h>
#include <QtCore/private/qfilesystementry_p.h>
#include <QtCore/private/qfilesystemmetadata_p.h>

#include <memory>

QT_BEGIN_NAMESPACE

class QFileInfoPrivate : public QSharedData
{
public:
    enum {
        // note: cachedFlags is only 30-bits wide
        CachedFileFlags         = 0x01,
        CachedLinkTypeFlag      = 0x02,
        CachedBundleTypeFlag    = 0x04,
        CachedSize              = 0x08,
        CachedATime             = 0x10,
        CachedBTime             = 0x20,
        CachedMCTime            = 0x40,
        CachedMTime             = 0x80,
        CachedPerms             = 0x100
    };

    static QFileInfoPrivate *get(QFileInfo *fi) { return fi->d_func(); }

    inline QFileInfoPrivate()
        : QSharedData(), fileEngine(nullptr),
        cachedFlags(0),
        isDefaultConstructed(true),
        cache_enabled(true), fileFlags(0), fileSize(0)
    {}
    inline QFileInfoPrivate(const QFileInfoPrivate &copy)
        : QSharedData(copy),
        fileEntry(copy.fileEntry),
        metaData(copy.metaData),
        fileEngine(QFileSystemEngine::createLegacyEngine(fileEntry, metaData)),
        cachedFlags(0),
#ifndef QT_NO_FSFILEENGINE
        isDefaultConstructed(false),
#else
        isDefaultConstructed(!fileEngine),
#endif
        cache_enabled(copy.cache_enabled), fileFlags(0), fileSize(0)
    {}
    inline QFileInfoPrivate(const QString &file)
        : fileEntry(file),
        fileEngine(QFileSystemEngine::createLegacyEngine(fileEntry, metaData)),
        cachedFlags(0),
#ifndef QT_NO_FSFILEENGINE
        isDefaultConstructed(file.isEmpty()),
#else
        isDefaultConstructed(!fileEngine),
#endif
        cache_enabled(true), fileFlags(0), fileSize(0)
    {
    }

    inline QFileInfoPrivate(const QFileSystemEntry &file, const QFileSystemMetaData &data)
        : QSharedData(),
        fileEntry(file),
        metaData(data),
        fileEngine(QFileSystemEngine::createLegacyEngine(fileEntry, metaData)),
        cachedFlags(0),
        isDefaultConstructed(false),
        cache_enabled(true), fileFlags(0), fileSize(0)
    {
        //If the file engine is not null, this maybe a "mount point" for a custom file engine
        //in which case we can't trust the metadata
        if (fileEngine)
            metaData = QFileSystemMetaData();
    }

    inline QFileInfoPrivate(const QFileSystemEntry &file, const QFileSystemMetaData &data, std::unique_ptr<QAbstractFileEngine> engine)
        : fileEntry(file),
        metaData(data),
        fileEngine{std::move(engine)},
        cachedFlags(0),
#ifndef QT_NO_FSFILEENGINE
        isDefaultConstructed(false),
#else
        isDefaultConstructed(!fileEngine),
#endif
        cache_enabled(true), fileFlags(0), fileSize(0)
    {
    }

    inline void clearFlags() const {
        fileFlags = 0;
        cachedFlags = 0;
        if (fileEngine)
            (void)fileEngine->fileFlags(QAbstractFileEngine::Refresh);
    }
    inline void clear() {
        metaData.clear();
        clearFlags();
        for (int i = QAbstractFileEngine::NFileNames - 1 ; i >= 0 ; --i)
            fileNames[i].clear();
        fileOwners[1].clear();
        fileOwners[0].clear();
    }

    uint getFileFlags(QAbstractFileEngine::FileFlags) const;
    QDateTime &getFileTime(QFile::FileTime) const;
    QString getFileName(QAbstractFileEngine::FileName) const;
    QString getFileOwner(QAbstractFileEngine::FileOwner own) const;

    QFileSystemEntry fileEntry;
    mutable QFileSystemMetaData metaData;

    std::unique_ptr<QAbstractFileEngine> const fileEngine;

    mutable QString fileNames[QAbstractFileEngine::NFileNames];
    mutable QString fileOwners[2];  // QAbstractFileEngine::FileOwner: OwnerUser and OwnerGroup
    mutable QDateTime fileTimes[4]; // QFile::FileTime: FileBirthTime, FileMetadataChangeTime, FileModificationTime, FileAccessTime

    mutable uint cachedFlags : 30;
    bool const isDefaultConstructed : 1; // QFileInfo is a default constructed instance
    bool cache_enabled : 1;
    mutable uint fileFlags;
    mutable qint64 fileSize;
    inline bool getCachedFlag(uint c) const
    { return cache_enabled ? (cachedFlags & c) : 0; }
    inline void setCachedFlag(uint c) const
    { if (cache_enabled) cachedFlags |= c; }

    template <typename Ret, typename FSLambda, typename EngineLambda>
    Ret checkAttribute(Ret defaultValue, QFileSystemMetaData::MetaDataFlags fsFlags,
                       FSLambda fsLambda, EngineLambda engineLambda) const
    {
        if (isDefaultConstructed)
            return defaultValue;
        if (fileEngine)
            return engineLambda();
        if (!cache_enabled || !metaData.hasFlags(fsFlags)) {
            QFileSystemEngine::fillMetaData(fileEntry, metaData, fsFlags);
            // ignore errors, fillMetaData will have cleared the flags
        }
        return fsLambda();
    }

    template <typename Ret, typename FSLambda, typename EngineLambda>
    Ret checkAttribute(QFileSystemMetaData::MetaDataFlags fsFlags, FSLambda fsLambda,
                       EngineLambda engineLambda) const
    {
        return checkAttribute(Ret(), std::move(fsFlags), std::move(fsLambda), engineLambda);
    }
};

QT_END_NAMESPACE

#endif // QFILEINFO_P_H
