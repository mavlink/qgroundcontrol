// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QFILESYSTEMWATCHER_P_H
#define QFILESYSTEMWATCHER_P_H

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

#include "qfilesystemwatcher.h"

QT_REQUIRE_CONFIG(filesystemwatcher);

#include <private/qobject_p.h>

#include <QtCore/qstringlist.h>
#include <QtCore/qhash.h>

QT_BEGIN_NAMESPACE

class QFileSystemWatcherEngine : public QObject
{
    Q_OBJECT

protected:
    inline QFileSystemWatcherEngine(QObject *parent)
        : QObject(parent)
    {
    }

public:
    // fills \a files and \a directories with the \a paths it could
    // watch, and returns a list of paths this engine could not watch
    virtual QStringList addPaths(const QStringList &paths,
                                 QStringList *files,
                                 QStringList *directories) = 0;
    // removes \a paths from \a files and \a directories, and returns
    // a list of paths this engine does not know about (either addPath
    // failed or wasn't called)
    virtual QStringList removePaths(const QStringList &paths,
                                    QStringList *files,
                                    QStringList *directories) = 0;

Q_SIGNALS:
    void fileChanged(const QString &path, bool removed);
    void directoryChanged(const QString &path, bool removed);
};

class QFileSystemWatcherPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QFileSystemWatcher)

    static QFileSystemWatcherEngine *createNativeEngine(QObject *parent);

public:
    QFileSystemWatcherPrivate();
    void init();
    void initPollerEngine();

    QFileSystemWatcherEngine *native, *poller;
    QStringList files, directories;

    // private slots
    void fileChanged(const QString &path, bool removed);
    void directoryChanged(const QString &path, bool removed);

    void connectEngine(QFileSystemWatcherEngine *e);

#if defined(Q_OS_WIN)
    void winDriveLockForRemoval(const QString &);
    void winDriveLockForRemovalFailed(const QString &);
    void winDriveRemoved(const QString &);

private:
    QHash<QChar, QStringList> temporarilyRemovedPaths;
#endif // Q_OS_WIN
};


QT_END_NAMESPACE
#endif // QFILESYSTEMWATCHER_P_H
