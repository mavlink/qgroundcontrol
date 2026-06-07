// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
// Qt-Security score:significant

#ifndef QQMLJSRESOURCEFILEMAPPER_P_H
#define QQMLJSRESOURCEFILEMAPPER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.

#include <qtqmlcompilerexports.h>

#include <QStringList>
#include <QHash>
#include <QFile>
#include <private/qglobal_p.h>

QT_BEGIN_NAMESPACE

struct Q_QMLCOMPILER_EXPORT QQmlJSResourceFileMapper
{
    struct Entry
    {
        QString resourcePath;
        QString filePath;
        bool isValid() const { return !resourcePath.isEmpty() && !filePath.isEmpty(); }
    };

    enum FilterMode {
        File      = 0x0, // Default is local (non-directory) file, without recursion
        Directory = 0x1, // Directory, either local or resource
        Resource  = 0x2, // Resource path, either to file or directory
        Recurse   = 0x4, // Recurse into subdirectories if Directory
    };
    Q_DECLARE_FLAGS(FilterFlags, FilterMode);

    struct Filter {
        QString path;
        QStringList suffixes;
        FilterFlags flags;
    };

    static Filter allQmlJSFilter();
    static Filter localFileFilter(const QString &file);
    static Filter resourceFileFilter(const QString &file);
    static Filter resourceQmlDirectoryFilter(const QString &directory);

    QQmlJSResourceFileMapper(const QStringList &resourceFiles);

    bool isEmpty() const;
    bool isFile(QStringView resourcePath) const;

    QList<Entry> filter(const Filter &filter) const;
    QStringList filePaths(const Filter &filter) const;
    QStringList resourcePaths(const Filter &filter) const;
    Entry entry(const Filter &filter) const;

private:
    void populateFromQrcFile(QFile &file);

    QList<Entry> qrcPathToFileSystemPath;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QQmlJSResourceFileMapper::FilterFlags);

QT_END_NAMESPACE

#endif // QMLJSRESOURCEFILEMAPPER_P_H
