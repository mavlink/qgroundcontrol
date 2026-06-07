// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:data-parser

#ifndef QFILESYSTEMENGINE_P_H
#define QFILESYSTEMENGINE_P_H

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

#include "qfile.h"
#include "qfilesystementry_p.h"
#include "qfilesystemmetadata_p.h"
#include <QtCore/private/qsystemerror_p.h>

#include <memory>
#include <optional>

QT_BEGIN_NAMESPACE

#define Q_RETURN_ON_INVALID_FILENAME(message, result) \
    { \
        qWarning(message); \
        errno = EINVAL; \
        return (result); \
    }

inline bool qIsFilenameBroken(const QByteArray &name)
{
    return name.contains('\0');
}

inline bool qIsFilenameBroken(const QString &name)
{
    return name.contains(QLatin1Char('\0'));
}

inline bool qIsFilenameBroken(const QFileSystemEntry &entry)
{
    return qIsFilenameBroken(entry.nativeFilePath());
}

#define Q_CHECK_FILE_NAME(name, result) \
    do { \
        if (Q_UNLIKELY((name).isEmpty())) \
            Q_RETURN_ON_INVALID_FILENAME("Empty filename passed to function", (result)); \
        if (Q_UNLIKELY(qIsFilenameBroken(name))) \
            Q_RETURN_ON_INVALID_FILENAME("Broken filename passed to function", (result)); \
    } while (false)

Q_CORE_EXPORT bool qt_isCaseSensitive(const QFileSystemEntry &entry, QFileSystemMetaData &data);

class Q_AUTOTEST_EXPORT QFileSystemEngine
{
public:
    using TriStateResult = QAbstractFileEngine::TriStateResult;

    static bool isCaseSensitive(const QFileSystemEntry &entry, QFileSystemMetaData &data);

    static QFileSystemEntry getLinkTarget(const QFileSystemEntry &link, QFileSystemMetaData &data);
    static QFileSystemEntry getRawLinkPath(const QFileSystemEntry &link,
                                           QFileSystemMetaData &data);
    static QFileSystemEntry getJunctionTarget(const QFileSystemEntry &link, QFileSystemMetaData &data);
    static QFileSystemEntry canonicalName(const QFileSystemEntry &entry, QFileSystemMetaData &data);
    static QFileSystemEntry absoluteName(const QFileSystemEntry &entry);
    static QByteArray id(const QFileSystemEntry &entry);
    static QString resolveUserName(const QFileSystemEntry &entry, QFileSystemMetaData &data);
    static QString resolveGroupName(const QFileSystemEntry &entry, QFileSystemMetaData &data);

#if defined(Q_OS_UNIX)
    static QString resolveUserName(uint userId);
    static QString resolveGroupName(uint groupId);
#endif

#if defined(Q_OS_DARWIN)
    static QString bundleName(const QFileSystemEntry &entry);
#else
    static QString bundleName(const QFileSystemEntry &) { return QString(); }
#endif

    static bool fillMetaData(const QFileSystemEntry &entry, QFileSystemMetaData &data,
                             QFileSystemMetaData::MetaDataFlags what);
#if defined(Q_OS_UNIX)
    static TriStateResult cloneFile(int srcfd, int dstfd, const QFileSystemMetaData &knownData);
    static bool fillMetaData(int fd, QFileSystemMetaData &data); // what = PosixStatFlags
    static QByteArray id(int fd);
    static bool setFileTime(int fd, const QDateTime &newDate,
                            QFile::FileTime whatTime, QSystemError &error);
    static bool setPermissions(int fd, QFile::Permissions permissions, QSystemError &error);
#endif
#if defined(Q_OS_WIN)
    static QFileSystemEntry junctionTarget(const QFileSystemEntry &link, QFileSystemMetaData &data);
    static bool uncListSharesOnServer(const QString &server, QStringList *list); //Used also by QFSFileEngineIterator::hasNext()
    static bool fillMetaData(int fd, QFileSystemMetaData &data,
                             QFileSystemMetaData::MetaDataFlags what);
    static bool fillMetaData(HANDLE fHandle, QFileSystemMetaData &data,
                             QFileSystemMetaData::MetaDataFlags what);
    static bool fillPermissions(const QFileSystemEntry &entry, QFileSystemMetaData &data,
                                QFileSystemMetaData::MetaDataFlags what);
    static QByteArray id(HANDLE fHandle);
    static bool setFileTime(HANDLE fHandle, const QDateTime &newDate,
                            QFile::FileTime whatTime, QSystemError &error);
    static QString owner(const QFileSystemEntry &entry, QAbstractFileEngine::FileOwner own);
    static QString nativeAbsoluteFilePath(const QString &path);
    static bool isDirPath(const QString &path, bool *existed);
#endif
    //homePath, rootPath and tempPath shall return clean paths
    static QString homePath();
    static QString rootPath();
    static QString tempPath();

    static bool createDirectory(const QFileSystemEntry &entry, bool createParents,
                                std::optional<QFile::Permissions> permissions = std::nullopt)
    {
        if (createParents)
            return mkpath(entry, permissions);
        return mkdir(entry, permissions);
    }

    static bool mkdir(const QFileSystemEntry &entry,
                      std::optional<QFile::Permissions> permissions = std::nullopt);
    static bool mkpath(const QFileSystemEntry &entry,
                       std::optional<QFile::Permissions> permissions = std::nullopt);

    static bool removeDirectory(const QFileSystemEntry &entry, bool removeEmptyParents)
    {
        if (removeEmptyParents)
            return rmpath(entry);
        return rmdir(entry);
    }

    static bool rmdir(const QFileSystemEntry &entry);
    static bool rmpath(const QFileSystemEntry &entry);

    static bool createLink(const QFileSystemEntry &source, const QFileSystemEntry &target, QSystemError &error);

    static bool copyFile(const QFileSystemEntry &source, const QFileSystemEntry &target, QSystemError &error);
    static bool supportsMoveFileToTrash();
    static bool moveFileToTrash(const QFileSystemEntry &source, QFileSystemEntry &newLocation, QSystemError &error);
    static bool renameFile(const QFileSystemEntry &source, const QFileSystemEntry &target, QSystemError &error);
    static bool renameOverwriteFile(const QFileSystemEntry &source, const QFileSystemEntry &target, QSystemError &error);
    static bool removeFile(const QFileSystemEntry &entry, QSystemError &error);

    static bool setPermissions(const QFileSystemEntry &entry, QFile::Permissions permissions,
                               QSystemError &error);

    // unused, therefore not implemented
    static bool setFileTime(const QFileSystemEntry &entry, const QDateTime &newDate,
                            QFile::FileTime whatTime, QSystemError &error);

    static bool setCurrentPath(const QFileSystemEntry &entry);
    static QFileSystemEntry currentPath();

    static std::unique_ptr<QAbstractFileEngine>
    createLegacyEngine(QFileSystemEntry &entry, QFileSystemMetaData &data);

private:
    static QString slowCanonicalized(const QString &path);
#if defined(Q_OS_WIN)
    static void clearWinStatData(QFileSystemMetaData &data);
#endif
};

QT_END_NAMESPACE

#endif // include guard
