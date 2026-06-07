// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QFILESYSTEMWATCHER_POLLING_P_H
#define QFILESYSTEMWATCHER_POLLING_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qbasictimer.h>
#include <QtCore/qfileinfo.h>
#include <QtCore/qmutex.h>
#include <QtCore/qdatetime.h>
#include <QtCore/qdir.h>
#include <QtCore/qdirlisting.h>
#include <QtCore/qhash.h>

#include "qfilesystemwatcher_p.h"

QT_REQUIRE_CONFIG(filesystemwatcher);
QT_BEGIN_NAMESPACE

class QPollingFileSystemWatcherEngine : public QFileSystemWatcherEngine
{
    Q_OBJECT

    class FileInfo
    {
        uint ownerId;
        uint groupId;
        QFile::Permissions permissions;
        QDateTime lastModified;
        QStringList entries;

        static QStringList dirEntryList(const QFileInfo &fileInfo)
        {
            Q_ASSERT(fileInfo.isDir());

            QStringList fileNames;
            using F = QDirListing::IteratorFlag;
            constexpr auto flags = F::ExcludeOther | F::IncludeDotAndDotDot;
            for (const auto &entry : QDirListing(fileInfo.absoluteFilePath(), flags))
                fileNames.emplace_back(entry.fileName());
            return fileNames;
        }

    public:
        FileInfo(const QFileInfo &fileInfo)
            : ownerId(fileInfo.ownerId()),
              groupId(fileInfo.groupId()),
              permissions(fileInfo.permissions()),
              lastModified(fileInfo.lastModified(QTimeZone::UTC))
        {
            if (fileInfo.isDir())
                entries = dirEntryList(fileInfo);
        }
        FileInfo &operator=(const QFileInfo &fileInfo)
        {
            *this = FileInfo(fileInfo);
            return *this;
        }

        bool operator!=(const QFileInfo &fileInfo) const
        {
            if (fileInfo.isDir() && entries != dirEntryList(fileInfo))
                return true;
            return (ownerId != fileInfo.ownerId()
                    || groupId != fileInfo.groupId()
                    || permissions != fileInfo.permissions()
                    || lastModified != fileInfo.lastModified(QTimeZone::UTC));
        }
    };

    QHash<QString, FileInfo> files, directories;

public:
    QPollingFileSystemWatcherEngine(QObject *parent);

    QStringList addPaths(const QStringList &paths, QStringList *files, QStringList *directories) override;
    QStringList removePaths(const QStringList &paths, QStringList *files, QStringList *directories) override;

private:
    void timerEvent(QTimerEvent *) final;

private:
    QBasicTimer timer;
};

QT_END_NAMESPACE
#endif // QFILESYSTEMWATCHER_POLLING_P_H

