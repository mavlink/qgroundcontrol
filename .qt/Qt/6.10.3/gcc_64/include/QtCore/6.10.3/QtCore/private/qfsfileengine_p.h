// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QFSFILEENGINE_P_H
#define QFSFILEENGINE_P_H

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

#include "qplatformdefs.h"
#include "QtCore/private/qabstractfileengine_p.h"
#include <QtCore/private/qfilesystementry_p.h>
#include <QtCore/private/qfilesystemmetadata_p.h>
#include <qhash.h>

#include <optional>

#ifdef Q_OS_UNIX
#include <sys/types.h> // for mode_t
#endif

#ifndef QT_NO_FSFILEENGINE

QT_BEGIN_NAMESPACE

struct ProcessOpenModeResult
{
    bool ok;
    QIODevice::OpenMode openMode;
    QString error;
};
Q_CORE_EXPORT ProcessOpenModeResult processOpenModeFlags(QIODevice::OpenMode mode);

class QFSFileEnginePrivate;

class Q_CORE_EXPORT QFSFileEngine : public QAbstractFileEngine
{
    Q_DECLARE_PRIVATE(QFSFileEngine)
public:
    QFSFileEngine();
    explicit QFSFileEngine(const QString &file);
    ~QFSFileEngine();

    bool open(QIODevice::OpenMode openMode, std::optional<QFile::Permissions> permissions) override;
    bool close() override;
    bool flush() override;
    bool syncToDisk() override;
    qint64 size() const override;
    qint64 pos() const override;
    bool seek(qint64) override;
    bool isSequential() const override;
    bool remove() override;
    bool copy(const QString &newName) override;

    bool rename(const QString &newName) override
    { return rename_helper(newName, Rename); }
    bool renameOverwrite(const QString &newName) override
    { return rename_helper(newName, RenameOverwrite); }

    bool link(const QString &newName) override;
    bool mkdir(const QString &dirName, bool createParentDirectories,
               std::optional<QFile::Permissions> permissions) const override;
    bool rmdir(const QString &dirName, bool recurseParentDirectories) const override;
    bool setSize(qint64 size) override;
    bool caseSensitive() const override;
    bool isRelativePath() const override;
    FileFlags fileFlags(FileFlags type) const override;
    bool setPermissions(uint perms) override;
    QByteArray id() const override;
    QString fileName(FileName file) const override;
    uint ownerId(FileOwner) const override;
    QString owner(FileOwner) const override;
    bool setFileTime(const QDateTime &newDate, QFile::FileTime time) override;
    QDateTime fileTime(QFile::FileTime time) const override;
    void setFileName(const QString &file) override;
    void setFileEntry(QFileSystemEntry &&entry);
    int handle() const override;

#ifndef QT_NO_FILESYSTEMITERATOR
    IteratorUniquePtr beginEntryList(const QString &path, QDirListing::IteratorFlags filters,
                                     const QStringList &filterNames) override;
#endif

    qint64 read(char *data, qint64 maxlen) override;
    qint64 readLine(char *data, qint64 maxlen) override;
    qint64 write(const char *data, qint64 len) override;
    TriStateResult cloneTo(QAbstractFileEngine *target) override;

    virtual bool isUnnamedFile() const
    { return false; }

    bool extension(Extension extension, const ExtensionOption *option = nullptr, ExtensionReturn *output = nullptr) override;
    bool supportsExtension(Extension extension) const override;

    //FS only!!
    bool open(QIODevice::OpenMode flags, int fd, QFile::FileHandleFlags handleFlags);
    bool open(QIODevice::OpenMode flags, FILE *fh, QFile::FileHandleFlags handleFlags);
    static bool setCurrentPath(const QString &path);
    static QString currentPath(const QString &path = QString());
    static QFileInfoList drives();

protected:
    QFSFileEngine(QFSFileEnginePrivate &dd);

private:
    enum RenameMode : int { Rename, RenameOverwrite };
    bool rename_helper(const QString &newName, RenameMode mode);
};

class Q_AUTOTEST_EXPORT QFSFileEnginePrivate : public QAbstractFileEnginePrivate
{
    Q_DECLARE_PUBLIC(QFSFileEngine)

public:
#ifdef Q_OS_WIN
    static QString longFileName(const QString &path);
#endif

    QFileSystemEntry fileEntry;
    QIODevice::OpenMode openMode;

    bool nativeOpen(QIODevice::OpenMode openMode, std::optional<QFile::Permissions> permissions);
    bool openFh(QIODevice::OpenMode flags, FILE *fh);
    bool openFd(QIODevice::OpenMode flags, int fd);
    bool nativeClose();
    bool closeFdFh();
    bool nativeFlush();
    bool nativeSyncToDisk();
    bool flushFh();
    qint64 nativeSize() const;
#ifndef Q_OS_WIN
    qint64 sizeFdFh() const;
#endif
    qint64 nativePos() const;
    qint64 posFdFh() const;
    bool nativeSeek(qint64);
    bool seekFdFh(qint64);
    qint64 nativeRead(char *data, qint64 maxlen);
    qint64 readFdFh(char *data, qint64 maxlen);
    qint64 nativeReadLine(char *data, qint64 maxlen);
    qint64 readLineFdFh(char *data, qint64 maxlen);
    qint64 nativeWrite(const char *data, qint64 len);
    qint64 writeFdFh(const char *data, qint64 len);
    int nativeHandle() const;
    bool nativeIsSequential() const;
#ifndef Q_OS_WIN
    bool isSequentialFdFh() const;
#endif
#ifdef Q_OS_WIN
    bool nativeRenameOverwrite(const QFileSystemEntry &newEntry);
#endif

    uchar *map(qint64 offset, qint64 size, QFile::MemoryMapFlags flags);
    bool unmap(uchar *ptr);
    void unmapAll();

    mutable QFileSystemMetaData metaData;

    FILE *fh;

#ifdef Q_OS_WIN
    HANDLE fileHandle;
    HANDLE mapHandle;
    QHash<uchar *, DWORD /* offset % AllocationGranularity */> maps;

    mutable int cachedFd;
    mutable DWORD fileAttrib;
#else
    struct StartAndLength {
        int start;     // offset % PageSize
        size_t length; // length + offset % PageSize
    };
    QHash<uchar *, StartAndLength> maps;
#endif
    int fd;

    enum LastIOCommand
    {
        IOFlushCommand,
        IOReadCommand,
        IOWriteCommand
    };
    LastIOCommand  lastIOCommand;
    bool lastFlushFailed;
    bool closeFileHandle;

    mutable uint is_sequential : 2;
    mutable uint tried_stat : 1;
    mutable uint need_lstat : 1;
    mutable uint is_link : 1;

#if defined(Q_OS_WIN)
    bool doStat(QFileSystemMetaData::MetaDataFlags flags) const;
#else
    bool doStat(QFileSystemMetaData::MetaDataFlags flags = QFileSystemMetaData::PosixStatFlags) const;
#endif
    bool isSymlink() const;

#if defined(Q_OS_WIN32)
    int sysOpen(const QString &, int flags);
#endif

    static bool openModeCanCreate(QIODevice::OpenMode openMode)
    {
        // WriteOnly can create, but only when ExistingOnly isn't specified.
        // ReadOnly by itself never creates.
        return (openMode & QFile::WriteOnly) && !(openMode & QFile::ExistingOnly);
    }
protected:
    QFSFileEnginePrivate(QAbstractFileEngine *q);

    void init();

    QAbstractFileEngine::FileFlags getPermissions(QAbstractFileEngine::FileFlags type) const;

#ifdef Q_OS_UNIX
    bool nativeOpenImpl(QIODevice::OpenMode openMode, mode_t mode);
#endif
};

QT_END_NAMESPACE

#endif // QT_NO_FSFILEENGINE

#endif // QFSFILEENGINE_P_H
