// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QABSTRACTFILEENGINE_P_H
#define QABSTRACTFILEENGINE_P_H

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

#include <QtCore/private/qglobal_p.h>
#include "QtCore/qfile.h"
#include "QtCore/qdir.h"
#include "QtCore/qdirlisting.h"

#include <memory>
#include <optional>

#ifdef open
#error qabstractfileengine_p.h must be included before any header file that defines open
#endif

QT_BEGIN_NAMESPACE

class QVariant;
class QAbstractFileEngineIterator;
class QAbstractFileEnginePrivate;

class Q_CORE_EXPORT QAbstractFileEngine
{
public:
    enum FileFlag {
        //perms (overlaps the QFile::Permission)
        ReadOwnerPerm = 0x4000, WriteOwnerPerm = 0x2000, ExeOwnerPerm = 0x1000,
        ReadUserPerm  = 0x0400, WriteUserPerm  = 0x0200, ExeUserPerm  = 0x0100,
        ReadGroupPerm = 0x0040, WriteGroupPerm = 0x0020, ExeGroupPerm = 0x0010,
        ReadOtherPerm = 0x0004, WriteOtherPerm = 0x0002, ExeOtherPerm = 0x0001,

        //types
        LinkType      = 0x10000,
        FileType      = 0x20000,
        DirectoryType = 0x40000,
        BundleType    = 0x80000,

        //flags
        HiddenFlag     = 0x0100000,
        LocalDiskFlag  = 0x0200000,
        ExistsFlag     = 0x0400000,
        RootFlag       = 0x0800000,
        Refresh        = 0x1000000,

        //masks
        PermsMask  = 0x0000FFFF,
        TypesMask  = 0x000F0000,
        FlagsMask  = 0x0FF00000,
        FileInfoAll = FlagsMask | PermsMask | TypesMask
    };
    Q_DECLARE_FLAGS(FileFlags, FileFlag)

    enum FileName {
        DefaultName,
        BaseName,
        PathName,
        AbsoluteName,
        AbsolutePathName,
        AbsoluteLinkTarget,
        CanonicalName,
        CanonicalPathName,
        BundleName,
        JunctionName,
        RawLinkPath,
        NFileNames  // Must be last.
    };
    enum FileOwner {
        OwnerUser,
        OwnerGroup
    };

    enum class TriStateResult : qint8 {
        NotSupported = -1,
        Failed = 0,
        Success = 1,
    };

    virtual ~QAbstractFileEngine();

    virtual bool open(QIODevice::OpenMode openMode,
                      std::optional<QFile::Permissions> permissions = std::nullopt);
    virtual bool close();
    virtual bool flush();
    virtual bool syncToDisk();
    virtual qint64 size() const;
    virtual qint64 pos() const;
    virtual bool seek(qint64 pos);
    virtual bool isSequential() const;
    virtual bool remove();
    virtual bool copy(const QString &newName);
    virtual bool rename(const QString &newName);
    virtual bool renameOverwrite(const QString &newName);
    virtual bool link(const QString &newName);
    virtual bool mkdir(const QString &dirName, bool createParentDirectories,
                       std::optional<QFile::Permissions> permissions = std::nullopt) const;
    virtual bool rmdir(const QString &dirName, bool recurseParentDirectories) const;
    virtual bool setSize(qint64 size);
    virtual bool caseSensitive() const;
    virtual bool isRelativePath() const;
    virtual QStringList entryList(QDir::Filters filters, const QStringList &filterNames) const;
    virtual QStringList entryList(QDirListing::IteratorFlags filters,
                                  const QStringList &filterNames) const;
    virtual FileFlags fileFlags(FileFlags type=FileInfoAll) const;
    virtual bool setPermissions(uint perms);
    virtual QByteArray id() const;
    virtual QString fileName(FileName file=DefaultName) const;
    virtual uint ownerId(FileOwner) const;
    virtual QString owner(FileOwner) const;
    virtual bool setFileTime(const QDateTime &newDate, QFile::FileTime time);
    virtual QDateTime fileTime(QFile::FileTime time) const;
    virtual void setFileName(const QString &file);
    virtual int handle() const;
    virtual TriStateResult cloneTo(QAbstractFileEngine *target);
    bool atEnd() const;
    uchar *map(qint64 offset, qint64 size, QFile::MemoryMapFlags flags);
    bool unmap(uchar *ptr);

    typedef QAbstractFileEngineIterator Iterator;
    using IteratorUniquePtr = std::unique_ptr<Iterator>;

    virtual IteratorUniquePtr endEntryList() { return {}; }
    virtual IteratorUniquePtr
    beginEntryList(const QString &path, QDirListing::IteratorFlags filters,
                   const QStringList &filterNames);

    virtual qint64 read(char *data, qint64 maxlen);
    virtual qint64 readLine(char *data, qint64 maxlen);
    virtual qint64 write(const char *data, qint64 len);

    QFile::FileError error() const;
    QString errorString() const;

    enum Extension {
        AtEndExtension,
        FastReadLineExtension,
        MapExtension,
        UnMapExtension
    };
    class ExtensionOption
    {};
    class ExtensionReturn
    {};

    class MapExtensionOption : public ExtensionOption {
        Q_DISABLE_COPY_MOVE(MapExtensionOption)
    public:
        qint64 offset;
        qint64 size;
        QFile::MemoryMapFlags flags;
        constexpr MapExtensionOption(qint64 off, qint64 sz, QFile::MemoryMapFlags f)
            : offset(off), size(sz), flags(f) {}
    };
    class MapExtensionReturn : public ExtensionReturn {
        Q_DISABLE_COPY_MOVE(MapExtensionReturn)
    public:
        MapExtensionReturn() = default;
        uchar *address = nullptr;
    };

    class UnMapExtensionOption : public ExtensionOption {
        Q_DISABLE_COPY_MOVE(UnMapExtensionOption)
    public:
        uchar *address = nullptr;
        constexpr UnMapExtensionOption(uchar *p) : address(p) {}
    };

    virtual bool extension(Extension extension, const ExtensionOption *option = nullptr, ExtensionReturn *output = nullptr);
    virtual bool supportsExtension(Extension extension) const;

    // Factory
    static std::unique_ptr<QAbstractFileEngine> create(const QString &fileName);

protected:
    void setError(QFile::FileError error, const QString &str);

    QAbstractFileEngine();
    QAbstractFileEngine(QAbstractFileEnginePrivate &);

    QScopedPointer<QAbstractFileEnginePrivate> d_ptr;
private:
    Q_DECLARE_PRIVATE(QAbstractFileEngine)
    Q_DISABLE_COPY_MOVE(QAbstractFileEngine)
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QAbstractFileEngine::FileFlags)

class Q_CORE_EXPORT QAbstractFileEngineHandler
{
    Q_DISABLE_COPY_MOVE(QAbstractFileEngineHandler)
public:
    QAbstractFileEngineHandler();
    virtual ~QAbstractFileEngineHandler();
    virtual std::unique_ptr<QAbstractFileEngine> create(const QString &fileName) const = 0;
};

class Q_CORE_EXPORT QAbstractFileEngineIterator
{
public:
    QAbstractFileEngineIterator(const QString &path, QDir::Filters filters,
                                const QStringList &nameFilters);
    QAbstractFileEngineIterator(const QString &path, QDirListing::IteratorFlags filters,
                                const QStringList &nameFilters);
    virtual ~QAbstractFileEngineIterator();

    virtual bool advance() = 0;

    QString path() const;
    QStringList nameFilters() const;
    QDir::Filters filters() const;

    virtual QString currentFileName() const = 0;
    virtual QFileInfo currentFileInfo() const;
    virtual QString currentFilePath() const;

protected:
    mutable QFileInfo m_fileInfo;

private:
    Q_DISABLE_COPY_MOVE(QAbstractFileEngineIterator)
    friend class QDirIterator;
    friend class QDirIteratorPrivate;
    friend class QDirListingPrivate;

    QDir::Filters m_filters;
    QDirListing::IteratorFlags m_listingFilters;
    QStringList m_nameFilters;
    QString m_path;
};

class QAbstractFileEnginePrivate
{
public:
    inline QAbstractFileEnginePrivate(QAbstractFileEngine *q)
        : fileError(QFile::UnspecifiedError), q_ptr(q)
    {
    }
    virtual ~QAbstractFileEnginePrivate();

    QFile::FileError fileError;
    QString errorString;

    QAbstractFileEngine *const q_ptr;
    Q_DECLARE_PUBLIC(QAbstractFileEngine)
};

std::unique_ptr<QAbstractFileEngine> qt_custom_file_engine_handler_create(const QString &path);

QT_END_NAMESPACE

#endif // QABSTRACTFILEENGINE_P_H
