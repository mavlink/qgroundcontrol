// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QTEMPORARYFILE_P_H
#define QTEMPORARYFILE_P_H

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

#include "private/qfsfileengine_p.h"
#include "private/qfilesystemengine_p.h"
#include "private/qfile_p.h"
#include "qtemporaryfile.h"

#if defined(Q_OS_LINUX) && QT_CONFIG(linkat)
#  include <fcntl.h>
#  ifdef O_TMPFILE
// some early libc support had the wrong values for O_TMPFILE
// (see https://bugzilla.gnome.org/show_bug.cgi?id=769453#c18)
#    if (O_TMPFILE & O_DIRECTORY) == O_DIRECTORY
#      define LINUX_UNNAMED_TMPFILE
#    endif
#  endif
#endif

QT_BEGIN_NAMESPACE

struct QTemporaryFileName
{
    QFileSystemEntry::NativePath path;
    qsizetype pos;
    qsizetype length;

    QTemporaryFileName(const QString &templateName);
    QFileSystemEntry::NativePath generateNext();
};

#if QT_CONFIG(temporaryfile)

class QTemporaryFilePrivate : public QFilePrivate
{
    Q_DECLARE_PUBLIC(QTemporaryFile)

public:
    QTemporaryFilePrivate();
    explicit QTemporaryFilePrivate(const QString &templateNameIn);
    ~QTemporaryFilePrivate();

    bool rename(const QString &newName, bool overwrite);

    QAbstractFileEngine *engine() const override;
    void resetFileEngine() const;
    void materializeUnnamedFile();

    bool autoRemove = true;
    QString templateName = defaultTemplateName();

    static QString defaultTemplateName();

    friend class QLockFilePrivate;
};

class QTemporaryFileEngine : public QFSFileEngine
{
    Q_DECLARE_PRIVATE(QFSFileEngine)
public:
    enum Flags { Win32NonShared = 0x1 };

    explicit QTemporaryFileEngine(const QString *_templateName, int _flags = 0)
        : templateName(*_templateName), flags(_flags)
    {}

    void initialize(const QString &file, quint32 mode, bool nameIsTemplate = true)
    {
        Q_D(QFSFileEngine);
        Q_ASSERT(!isReallyOpen());
        fileMode = mode;
        filePathIsTemplate = filePathWasTemplate = nameIsTemplate;

        if (filePathIsTemplate) {
            d->fileEntry.clear();
        } else {
            QFSFileEngine::setFileEntry(QFileSystemEntry(file));
        }
    }
    ~QTemporaryFileEngine();

    bool isReallyOpen() const;
    void setFileName(const QString &file) override;

    bool open(QIODevice::OpenMode flags, std::optional<QFile::Permissions> permissions) override;
    bool remove() override;
    bool rename(const QString &newName) override;
    bool renameOverwrite(const QString &newName) override;
    bool close() override;
    QString fileName(FileName file) const override;

    enum MaterializationMode { Overwrite, DontOverwrite, NameIsTemplate };
    bool materializeUnnamedFile(const QString &newName, MaterializationMode mode);
    bool isUnnamedFile() const override final;

    const QString &templateName;
    quint32 fileMode = 0;
    int flags = 0;
    bool filePathIsTemplate = true;
    bool filePathWasTemplate = true;
    bool unnamedFile = false;
};

#endif // QT_CONFIG(temporaryfile)

QT_END_NAMESPACE

#endif /* QTEMPORARYFILE_P_H */

