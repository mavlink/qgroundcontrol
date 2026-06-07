// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QDIR_P_H
#define QDIR_P_H

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

#include "qfilesystementry_p.h"
#include "qfilesystemmetadata_p.h"

#include <QtCore/qmutex.h>

#include <memory>

QT_BEGIN_NAMESPACE

class QDirPrivate : public QSharedData
{
public:
    enum PathNormalization {
        DefaultNormalization = 0x00,
        UrlNormalizationMode = 0x01,
        RemotePath = 0x02,
    };
    Q_DECLARE_FLAGS(PathNormalizations, PathNormalization)
    Q_FLAGS(PathNormalizations)

    explicit QDirPrivate(const QString &path, const QStringList &nameFilters_ = QStringList(),
                         QDir::SortFlags sort_ = QDir::SortFlags(QDir::Name | QDir::IgnoreCase),
                         QDir::Filters filters_ = QDir::AllEntries);

    explicit QDirPrivate(const QDirPrivate &copy); // Copies everything except mutex and fileEngine

    bool exists() const;

    void initFileLists(const QDir &dir) const;

    static void sortFileList(QDir::SortFlags, const QFileInfoList &, QStringList *, QFileInfoList *);

    static inline QChar getFilterSepChar(const QString &nameFilter);

    static inline QStringList splitFilters(const QString &nameFilter, QChar sep = {});

    void setPath(const QString &path);

    enum MetaDataClearing { KeepMetaData, IncludingMetaData };
    void clearCache(MetaDataClearing mode);

    QString resolveAbsoluteEntry() const;

    QStringList nameFilters;
    QDir::SortFlags sort;
    QDir::Filters filters;

    std::unique_ptr<QAbstractFileEngine> fileEngine;

    QFileSystemEntry dirEntry;

    struct FileCache
    {
        QMutex mutex;
        QStringList files;
        QFileInfoList fileInfos;
        std::atomic<bool> fileListsInitialized = false;
        QFileSystemEntry absoluteDirEntry;
        QFileSystemMetaData metaData;
    };
    mutable FileCache fileCache;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QDirPrivate::PathNormalizations)

bool qt_isPathNormalized(const QString &path, QDirPrivate::PathNormalizations flags) noexcept;
Q_AUTOTEST_EXPORT bool qt_normalizePathSegments(QString *path, QDirPrivate::PathNormalizations flags);

QT_END_NAMESPACE

#endif
