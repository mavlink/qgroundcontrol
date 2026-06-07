// Copyright (C) 2024 Ahmad Samir <a.samirh78@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QDIRENTRYINFO_P_H
#define QDIRENTRYINFO_P_H

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

#include <QtCore/private/qfileinfo_p.h>
#include <QtCore/private/qfilesystementry_p.h>
#include <QtCore/private/qfilesystemmetadata_p.h>

QT_BEGIN_NAMESPACE

class QDirEntryInfo
{
    const QFileSystemMetaData &ensureFilled(QFileSystemMetaData::MetaDataFlags what)
    {
        if (!metaData.hasFlags(what))
            QFileSystemEngine::fillMetaData(entry, metaData, what);
        return metaData;
    }

public:
    const QFileInfo &fileInfo()
    {
        if (!fileInfoOpt) {
            fileInfoOpt.emplace(new QFileInfoPrivate(entry, metaData));
            metaData.clear();
        }
        return *fileInfoOpt;
    }

    QString fileName()
    { return fileInfoOpt ? fileInfoOpt->fileName() : entry.fileName(); }
    QString baseName()
    { return fileInfoOpt ? fileInfoOpt->baseName() : entry.baseName(); }
    QString completeBaseName() const
    { return fileInfoOpt ? fileInfoOpt->completeBaseName() : entry.completeBaseName(); }
    QString suffix() const
    { return fileInfoOpt ? fileInfoOpt->suffix() : entry.suffix(); }
    QString completeSuffix() const
    { return fileInfoOpt ? fileInfoOpt->completeSuffix() : entry.completeSuffix(); }
    QString filePath()
    { return fileInfoOpt ? fileInfoOpt->filePath() : entry.filePath(); }

    QString bundleName() { return fileInfo().bundleName(); }

    QString canonicalFilePath()
    {
        // QFileInfo caches these strings
        return fileInfo().canonicalFilePath();
    }

    QString absoluteFilePath()  {
        // QFileInfo caches these strings
        return fileInfo().absoluteFilePath();
    }

    QString absolutePath()  {
        // QFileInfo caches these strings
        return fileInfo().absolutePath();
    }


    bool isDir() {
        if (fileInfoOpt)
            return fileInfoOpt->isDir();

        return ensureFilled(QFileSystemMetaData::DirectoryType).isDirectory();
    }

    bool isFile() {
        if (fileInfoOpt)
            return fileInfoOpt->isFile();

        return ensureFilled(QFileSystemMetaData::FileType).isFile();
    }

    bool isSymLink() {
        if (fileInfoOpt)
            return fileInfoOpt->isSymLink();

        return ensureFilled(QFileSystemMetaData::LegacyLinkType).isLegacyLink();
    }

    bool isSymbolicLink() {
        if (fileInfoOpt)
            return fileInfoOpt->isSymbolicLink();

        return ensureFilled(QFileSystemMetaData::LinkType).isLink();
    }

    bool exists() {
        if (fileInfoOpt)
            return fileInfoOpt->exists();

        return ensureFilled(QFileSystemMetaData::ExistsAttribute).exists();
    }

    bool isHidden() {
        if (fileInfoOpt)
            return fileInfoOpt->isHidden();

        return ensureFilled(QFileSystemMetaData::HiddenAttribute).isHidden();
    }

    bool isReadable() {
        if (fileInfoOpt)
            return fileInfoOpt->isReadable();

        return ensureFilled(QFileSystemMetaData::UserReadPermission).isReadable();
    }

    bool isWritable() {
        if (fileInfoOpt)
            return fileInfoOpt->isWritable();

        return ensureFilled(QFileSystemMetaData::UserWritePermission).isWritable();
    }

    bool isExecutable() {
        if (fileInfoOpt)
            return fileInfoOpt->isExecutable();

        return ensureFilled(QFileSystemMetaData::UserExecutePermission).isExecutable();
    }

    qint64 size() { return fileInfo().size(); }

    QDateTime fileTime(QFile::FileTime type, const QTimeZone &tz)
    {
        return fileInfo().fileTime(type, tz);
    }

private:
    friend class QDirListingPrivate;
    friend class QDirListing;

    QFileSystemEntry entry;
    QFileSystemMetaData metaData;
    std::optional<QFileInfo> fileInfoOpt = std::nullopt;
};

QT_END_NAMESPACE

#endif // QDIRENTRYINFO_P_H
