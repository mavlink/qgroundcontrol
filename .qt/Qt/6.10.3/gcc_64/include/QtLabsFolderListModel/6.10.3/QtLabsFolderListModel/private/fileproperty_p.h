// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef FILEPROPERTY_P_H
#define FILEPROPERTY_P_H

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

#include <QtQmlIntegration/qqmlintegration.h>
#include <QtCore/qfileinfo.h>
#include <QtCore/qdatetime.h>

#include <private/qglobal_p.h>

QT_BEGIN_NAMESPACE

class FileProperty
{
    Q_GADGET
    QML_ANONYMOUS
public:
    FileProperty(const QFileInfo &info = QFileInfo()) :
        mFileName(info.fileName()),
        mFilePath(info.filePath()),
        mBaseName(info.baseName()),
        mSuffix(info.completeSuffix()),
        mSize(info.size()),
        mIsDir(info.isDir()),
        mIsFile(info.isFile()),
        mLastModified(info.lastModified()),
        mLastRead(info.lastRead())
    {
    }

    inline QString fileName() const { return mFileName; }
    inline QString filePath() const { return mFilePath; }
    inline QString baseName() const { return mBaseName; }
    inline qint64 size() const { return mSize; }
    inline QString suffix() const { return mSuffix; }
    inline bool isDir() const { return mIsDir; }
    inline bool isFile() const { return mIsFile; }
    inline QDateTime lastModified() const { return mLastModified; }
    inline QDateTime lastRead() const { return mLastRead; }

    inline bool operator !=(const FileProperty &fileInfo) const {
        return !operator==(fileInfo);
    }
    bool operator ==(const FileProperty &property) const {
        return ((mFileName == property.mFileName) && (isDir() == property.isDir()));
    }

private:
    QString mFileName;
    QString mFilePath;
    QString mBaseName;
    QString mSuffix;
    qint64 mSize;
    bool mIsDir;
    bool mIsFile;
    QDateTime mLastModified;
    QDateTime mLastRead;
};

QT_END_NAMESPACE

#endif // FILEPROPERTY_P_H
