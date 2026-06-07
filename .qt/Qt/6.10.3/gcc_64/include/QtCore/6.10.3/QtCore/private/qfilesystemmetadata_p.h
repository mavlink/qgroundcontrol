// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QFILESYSTEMMETADATA_P_H
#define QFILESYSTEMMETADATA_P_H

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
#include <QtCore/qglobal.h>
#include <QtCore/qdatetime.h>
#include <QtCore/qtimezone.h>
#include <QtCore/private/qabstractfileengine_p.h>

// Platform-specific includes
#ifdef Q_OS_WIN
#  include <QtCore/qt_windows.h>
#  ifndef IO_REPARSE_TAG_SYMLINK
#     define IO_REPARSE_TAG_SYMLINK (0xA000000CL)
#  endif
#endif

#ifdef Q_OS_UNIX
struct statx;
#endif

QT_BEGIN_NAMESPACE

class QFileSystemEngine;

class Q_AUTOTEST_EXPORT QFileSystemMetaData
{
public:
    QFileSystemMetaData()
        : size_(-1)
    {
    }

    enum MetaDataFlag {
        // Permissions, overlaps with QFile::Permissions
        OtherReadPermission = 0x00000004,   OtherWritePermission = 0x00000002,  OtherExecutePermission = 0x00000001,
        GroupReadPermission = 0x00000040,   GroupWritePermission = 0x00000020,  GroupExecutePermission = 0x00000010,
        UserReadPermission  = 0x00000400,   UserWritePermission  = 0x00000200,  UserExecutePermission  = 0x00000100,
        OwnerReadPermission = 0x00004000,   OwnerWritePermission = 0x00002000,  OwnerExecutePermission = 0x00001000,

        OtherPermissions    = OtherReadPermission | OtherWritePermission | OtherExecutePermission,
        GroupPermissions    = GroupReadPermission | GroupWritePermission | GroupExecutePermission,
        UserPermissions     = UserReadPermission  | UserWritePermission  | UserExecutePermission,
        OwnerPermissions    = OwnerReadPermission | OwnerWritePermission | OwnerExecutePermission,

        ReadPermissions     = OtherReadPermission | GroupReadPermission | UserReadPermission | OwnerReadPermission,
        WritePermissions    = OtherWritePermission | GroupWritePermission | UserWritePermission | OwnerWritePermission,
        ExecutePermissions  = OtherExecutePermission | GroupExecutePermission | UserExecutePermission | OwnerExecutePermission,

        Permissions         = OtherPermissions | GroupPermissions | UserPermissions | OwnerPermissions,

        // Type
        LinkType            = 0x00010000,
        FileType            = 0x00020000,
        DirectoryType       = 0x00040000,
#if defined(Q_OS_DARWIN)
        BundleType          = 0x00080000,
        AliasType           = 0x08000000,
#else
        BundleType          =        0x0,
        AliasType           =        0x0,
#endif
#if defined(Q_OS_WIN)
        JunctionType        = 0x04000000,
        WinLnkType          = 0x08000000,   // Note: Uses the same position for AliasType on Mac
#else
        JunctionType        =        0x0,
        WinLnkType          =        0x0,
#endif
        SequentialType      = 0x00800000,   // Note: overlaps with QAbstractFileEngine::RootFlag

        LegacyLinkType      = LinkType | AliasType | WinLnkType,

        Type                = LinkType | FileType | DirectoryType | BundleType | SequentialType | AliasType,

        // Attributes
        HiddenAttribute     = 0x00100000,
        SizeAttribute       = 0x00200000,   // Note: overlaps with QAbstractFileEngine::LocalDiskFlag
        ExistsAttribute     = 0x00400000,   // For historical reasons, indicates existence of data, not the file
#if defined(Q_OS_WIN)
        WasDeletedAttribute =        0x0,
#else
        WasDeletedAttribute = 0x40000000,   // Indicates the file was deleted
#endif

        Attributes          = HiddenAttribute | SizeAttribute | ExistsAttribute | WasDeletedAttribute,

        // Times - if we know one of them, we know them all
        AccessTime          = 0x02000000,
        BirthTime           = 0x02000000,
        MetadataChangeTime  = 0x02000000,
        ModificationTime    = 0x02000000,

        Times               = AccessTime | BirthTime | MetadataChangeTime | ModificationTime,

        // Owner IDs
        UserId              = 0x10000000,
        GroupId             = 0x20000000,

        CaseSensitive       = 0x80000000,

        OwnerIds            = UserId | GroupId,

        PosixStatFlags      = OtherPermissions
                            | GroupPermissions
                            | OwnerPermissions
                            | FileType
                            | DirectoryType
                            | SequentialType
                            | SizeAttribute
                            | WasDeletedAttribute
                            | Times
                            | OwnerIds,

#if defined(Q_OS_WIN)
        WinStatFlags        = FileType
                            | DirectoryType
                            | HiddenAttribute
                            | ExistsAttribute
                            | SizeAttribute
                            | Times,
#endif

        AllMetaDataFlags    = 0xFFFFFFFF

    };
    Q_DECLARE_FLAGS(MetaDataFlags, MetaDataFlag)

    bool hasFlags(MetaDataFlags flags) const
    {
        return ((knownFlagsMask & flags) == flags);
    }

    MetaDataFlags missingFlags(MetaDataFlags flags)
    {
        return flags & ~knownFlagsMask;
    }

    void clear()
    {
        knownFlagsMask = {};
    }

    void clearFlags(MetaDataFlags flags = AllMetaDataFlags)
    {
        knownFlagsMask &= ~flags;
    }

    bool exists() const                     { return entryFlags.testAnyFlag(ExistsAttribute); }

    bool isLink() const                     { return entryFlags.testAnyFlag(LinkType); }
    bool isFile() const                     { return entryFlags.testAnyFlag(FileType); }
    bool isDirectory() const                { return entryFlags.testAnyFlag(DirectoryType); }
    bool isBundle() const;
    bool isAlias() const;
    bool isLegacyLink() const               { return entryFlags.testAnyFlag(LegacyLinkType); }
    bool isSequential() const               { return entryFlags.testAnyFlag(SequentialType); }
    bool isHidden() const                   { return entryFlags.testAnyFlag(HiddenAttribute); }
    bool wasDeleted() const                 { return entryFlags.testAnyFlag(WasDeletedAttribute); }
#if defined(Q_OS_WIN)
    bool isLnkFile() const                  { return entryFlags.testAnyFlag(WinLnkType); }
    bool isJunction() const                 { return entryFlags.testAnyFlag(JunctionType); }
#else
    bool isLnkFile() const                  { return false; }
    bool isJunction() const                 { return false; }
#endif

    qint64 size() const                     { return size_; }

    inline QFile::Permissions permissions() const;
    // Has to be defined after the
    // Q_DECLARE_OPERATORS_FOR_FLAGS(QFileSystemMetaData::MetaDataFlags) call below.
    inline void setPermissions(QFile::Permissions permissions);

    QDateTime accessTime() const;
    QDateTime birthTime() const;
    QDateTime metadataChangeTime() const;
    QDateTime modificationTime() const;

    QDateTime fileTime(QFile::FileTime time) const;
    uint userId() const;
    uint groupId() const;
    uint ownerId(QAbstractFileEngine::FileOwner owner) const;

    bool isReadable() const   { return permissions().testAnyFlags(QFile::ReadUser); }
    bool isWritable() const   { return permissions().testAnyFlags(QFile::WriteUser); }
    bool isExecutable() const { return permissions().testAnyFlags(QFile::ExeUser); }

#ifdef Q_OS_UNIX
    void fillFromStatxBuf(const struct statx &statBuffer);
    void fillFromStatBuf(const QT_STATBUF &statBuffer);
    void fillFromDirEnt(const QT_DIRENT &statBuffer);
#endif

#if defined(Q_OS_WIN)
    inline void fillFromFileAttribute(DWORD fileAttribute, bool isDriveRoot = false);
    inline void fillFromFindData(WIN32_FIND_DATA &findData, bool setLinkType = false, bool isDriveRoot = false);
    inline void fillFromFindInfo(BY_HANDLE_FILE_INFORMATION &fileInfo);
#endif
private:
    friend class QFileSystemEngine;

    MetaDataFlags knownFlagsMask;
    MetaDataFlags entryFlags;

    qint64 size_ = 0;

    // Platform-specific data goes here:
#if defined(Q_OS_WIN)
    DWORD fileAttribute_;
    FILETIME birthTime_;
    FILETIME changeTime_;
    FILETIME lastAccessTime_;
    FILETIME lastWriteTime_;
#else
    // msec precision
    qint64 accessTime_ = 0;
    qint64 birthTime_ = 0;
    qint64 metadataChangeTime_ = 0;
    qint64 modificationTime_ = 0;

    uint userId_ = (uint) -2;
    uint groupId_ = (uint) -2;
#endif

};

Q_DECLARE_OPERATORS_FOR_FLAGS(QFileSystemMetaData::MetaDataFlags)

inline QFile::Permissions QFileSystemMetaData::permissions() const { return QFile::Permissions::fromInt((Permissions & entryFlags).toInt()); }

void QFileSystemMetaData::setPermissions(QFile::Permissions permissions)
{
    entryFlags &= ~Permissions;
    entryFlags |= MetaDataFlag(uint(permissions.toInt()));
    knownFlagsMask |= Permissions;
}

#if defined(Q_OS_DARWIN)
inline bool QFileSystemMetaData::isBundle() const                   { return entryFlags.testAnyFlag(BundleType); }
inline bool QFileSystemMetaData::isAlias() const                    { return entryFlags.testAnyFlag(AliasType); }
#else
inline bool QFileSystemMetaData::isBundle() const                   { return false; }
inline bool QFileSystemMetaData::isAlias() const                    { return false; }
#endif

#if defined(Q_OS_UNIX) || defined (Q_OS_WIN)
inline QDateTime QFileSystemMetaData::fileTime(QFile::FileTime time) const
{
    switch (time) {
    case QFile::FileModificationTime:
        return modificationTime();

    case QFile::FileAccessTime:
        return accessTime();

    case QFile::FileBirthTime:
        return birthTime();

    case QFile::FileMetadataChangeTime:
        return metadataChangeTime();
    }

    return QDateTime();
}
#endif

#if defined(Q_OS_UNIX)
inline QDateTime QFileSystemMetaData::birthTime() const
{
    return birthTime_
        ? QDateTime::fromMSecsSinceEpoch(birthTime_, QTimeZone::UTC)
        : QDateTime();
}
inline QDateTime QFileSystemMetaData::metadataChangeTime() const
{
    return metadataChangeTime_
        ? QDateTime::fromMSecsSinceEpoch(metadataChangeTime_, QTimeZone::UTC)
        : QDateTime();
}
inline QDateTime QFileSystemMetaData::modificationTime() const
{
    return modificationTime_
        ? QDateTime::fromMSecsSinceEpoch(modificationTime_, QTimeZone::UTC)
        : QDateTime();
}
inline QDateTime QFileSystemMetaData::accessTime() const
{
    return accessTime_
        ? QDateTime::fromMSecsSinceEpoch(accessTime_, QTimeZone::UTC)
        : QDateTime();
}

inline uint QFileSystemMetaData::userId() const                     { return userId_; }
inline uint QFileSystemMetaData::groupId() const                    { return groupId_; }

inline uint QFileSystemMetaData::ownerId(QAbstractFileEngine::FileOwner owner) const
{
    if (owner == QAbstractFileEngine::OwnerUser)
        return userId();
    else
        return groupId();
}
#endif

#if defined(Q_OS_WIN)
inline uint QFileSystemMetaData::userId() const                     { return (uint) -2; }
inline uint QFileSystemMetaData::groupId() const                    { return (uint) -2; }
inline uint QFileSystemMetaData::ownerId(QAbstractFileEngine::FileOwner owner) const
{
    if (owner == QAbstractFileEngine::OwnerUser)
        return userId();
    else
        return groupId();
}

inline void QFileSystemMetaData::fillFromFileAttribute(DWORD fileAttribute,bool isDriveRoot)
{
    fileAttribute_ = fileAttribute;
    // Ignore the hidden attribute for drives.
    if (!isDriveRoot && (fileAttribute_ & FILE_ATTRIBUTE_HIDDEN))
        entryFlags |= HiddenAttribute;
    entryFlags |= ((fileAttribute & FILE_ATTRIBUTE_DIRECTORY) ? DirectoryType: FileType);
    entryFlags |= ExistsAttribute;
    knownFlagsMask |= FileType | DirectoryType | HiddenAttribute | ExistsAttribute;

    // this function is never called for a .lnk file
    knownFlagsMask |= WinLnkType;
}

inline void QFileSystemMetaData::fillFromFindData(WIN32_FIND_DATA &findData, bool setLinkType, bool isDriveRoot)
{
    fillFromFileAttribute(findData.dwFileAttributes, isDriveRoot);
    birthTime_ = findData.ftCreationTime;
    lastAccessTime_ = findData.ftLastAccessTime;
    changeTime_ = lastWriteTime_ = findData.ftLastWriteTime;
    if (fileAttribute_ & FILE_ATTRIBUTE_DIRECTORY) {
        size_ = 0;
    } else {
        size_ = findData.nFileSizeHigh;
        size_ <<= 32;
        size_ += findData.nFileSizeLow;
    }
    knownFlagsMask |=  Times | SizeAttribute;
    if (setLinkType) {
        knownFlagsMask |=  LinkType;
        entryFlags &= ~LinkType;
        if (fileAttribute_ & FILE_ATTRIBUTE_REPARSE_POINT) {
            if (findData.dwReserved0 == IO_REPARSE_TAG_SYMLINK) {
                entryFlags |= LinkType;
#if defined(IO_REPARSE_TAG_MOUNT_POINT)
            } else if ((fileAttribute_ & FILE_ATTRIBUTE_DIRECTORY)
                    && (findData.dwReserved0 == IO_REPARSE_TAG_MOUNT_POINT)) {
                entryFlags |= JunctionType;
#endif
            }
        }
    }
}

inline void QFileSystemMetaData::fillFromFindInfo(BY_HANDLE_FILE_INFORMATION &fileInfo)
{
    fillFromFileAttribute(fileInfo.dwFileAttributes);
    birthTime_ = fileInfo.ftCreationTime;
    lastAccessTime_ = fileInfo.ftLastAccessTime;
    changeTime_ = lastWriteTime_ = fileInfo.ftLastWriteTime;
    if (fileAttribute_ & FILE_ATTRIBUTE_DIRECTORY) {
        size_ = 0;
    } else {
        size_ = fileInfo.nFileSizeHigh;
        size_ <<= 32;
        size_ += fileInfo.nFileSizeLow;
    }
    knownFlagsMask |=  Times | SizeAttribute;
}
#endif // Q_OS_WIN

QT_END_NAMESPACE

#endif // include guard
