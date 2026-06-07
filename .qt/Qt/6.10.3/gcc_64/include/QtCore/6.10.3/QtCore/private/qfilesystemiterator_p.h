// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QFILESYSTEMITERATOR_P_H
#define QFILESYSTEMITERATOR_P_H

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

#include <QtCore/qglobal.h>

#ifndef QT_NO_FILESYSTEMITERATOR

#include <QtCore/qdir.h>
#include <QtCore/qstringlist.h>

#include <QtCore/private/qfilesystementry_p.h>
#include <QtCore/private/qfilesystemmetadata_p.h>

#if !defined(Q_OS_WIN)
#include <private/qstringconverter_p.h>
#endif

#include <memory>

QT_BEGIN_NAMESPACE

class QFileSystemIterator
{
public:
    QFileSystemIterator(const QFileSystemEntry &entry, QDir::Filters filters);
    QFileSystemIterator(const QFileSystemEntry &entry);
    QFileSystemIterator(const QFileSystemEntry &entry, QDirListing::IteratorFlags filters);
    ~QFileSystemIterator();

    bool advance(QFileSystemEntry &fileEntry, QFileSystemMetaData &metaData);

private:
    QString dirPath;

    // Platform-specific data
#if defined(Q_OS_WIN)
    QFileSystemEntry::NativePath nativePath;
    HANDLE findFileHandle;
    QStringList uncShares;
    bool uncFallback;
    int uncShareIndex;
    bool onlyDirs;
#else
    struct DirStreamCloser {
        void operator()(QT_DIR *dir) { if (dir) QT_CLOSEDIR(dir); }
    };
    using DirPtr = std::unique_ptr<QT_DIR, DirStreamCloser>;
    DirPtr dir;

    QT_DIRENT *dirEntry = nullptr;
    int lastError = 0;
    QStringDecoder toUtf16;
#endif

    Q_DISABLE_COPY_MOVE(QFileSystemIterator)
};

QT_END_NAMESPACE

#endif // QT_NO_FILESYSTEMITERATOR

#endif // include guard
