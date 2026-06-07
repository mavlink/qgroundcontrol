// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QFILESYSTEMENTRY_P_H
#define QFILESYSTEMENTRY_P_H

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
#include <QtCore/qstring.h>
#include <QtCore/qbytearray.h>

QT_BEGIN_NAMESPACE

class QFileSystemEntry
{
public:

#ifndef Q_OS_WIN
    typedef QByteArray NativePath;
#else
    typedef QString NativePath;
#endif
    struct FromNativePath{};
    struct FromInternalPath{};

    Q_AUTOTEST_EXPORT QFileSystemEntry();
    Q_AUTOTEST_EXPORT explicit QFileSystemEntry(const QString &filePath);

    Q_AUTOTEST_EXPORT QFileSystemEntry(const QString &filePath, FromInternalPath dummy);
    Q_AUTOTEST_EXPORT QFileSystemEntry(const NativePath &nativeFilePath, FromNativePath dummy);
    Q_AUTOTEST_EXPORT QFileSystemEntry(const QString &filePath, const NativePath &nativeFilePath);

    Q_AUTOTEST_EXPORT QString filePath() const;
    Q_AUTOTEST_EXPORT QString fileName() const;
    Q_AUTOTEST_EXPORT QString path() const;
    Q_AUTOTEST_EXPORT NativePath nativeFilePath() const;
    Q_AUTOTEST_EXPORT QString baseName() const;
    Q_AUTOTEST_EXPORT QString completeBaseName() const;
    Q_AUTOTEST_EXPORT QString suffix() const;
    Q_AUTOTEST_EXPORT QString completeSuffix() const;
    Q_AUTOTEST_EXPORT bool isAbsolute() const;
    Q_AUTOTEST_EXPORT bool isRelative() const;
    Q_AUTOTEST_EXPORT bool isClean() const;

#if defined(Q_OS_WIN)
    Q_AUTOTEST_EXPORT bool isDriveRoot() const;
    static bool isDriveRootPath(const QString &path);
    static QString removeUncOrLongPathPrefix(QString path);
#endif
    Q_AUTOTEST_EXPORT bool isRoot() const;

    Q_AUTOTEST_EXPORT bool isEmpty() const;

    void clear()
    {
        *this = QFileSystemEntry();
    }

    Q_CORE_EXPORT static bool isRootPath(const QString &path);

private:
    // creates the QString version out of the bytearray version
    void resolveFilePath() const;
    // creates the bytearray version out of the QString version
    void resolveNativeFilePath() const;
    // resolves the separator
    void findLastSeparator() const;
    // resolves the dots and the separator
    void findFileNameSeparators() const;

    mutable QString m_filePath; // always has slashes as separator
    mutable NativePath m_nativeFilePath; // native encoding and separators

    mutable qint16 m_lastSeparator; // index in m_filePath of last separator
    mutable qint16 m_firstDotInFileName; // index after m_filePath for first dot (.)
    mutable qint16 m_lastDotInFileName; // index after m_firstDotInFileName for last dot (.)
};

QT_END_NAMESPACE

#endif // include guard
