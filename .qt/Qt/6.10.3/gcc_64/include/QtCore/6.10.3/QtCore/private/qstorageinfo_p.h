// Copyright (C) 2014 Ivan Komissarov <ABBAPOH@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QSTORAGEINFO_P_H
#define QSTORAGEINFO_P_H

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

#include <QtCore/qloggingcategory.h>
#include <QtCore/qsystemdetection.h>
#include <QtCore/qtenvironmentvariables.h>
#include <QtCore/private/qglobal_p.h>
#include "qstorageinfo.h"

#ifdef Q_OS_UNIX
#include <sys/types.h> // dev_t
#endif

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(lcStorageInfo)

class QStorageInfoPrivate : public QSharedData
{
public:
    QStorageInfoPrivate() = default;

    void doStat();

    static QList<QStorageInfo> mountedVolumes();

    static QStorageInfo root()
    {
#ifdef Q_OS_WIN
        return QStorageInfo(QDir::fromNativeSeparators(QFile::decodeName(qgetenv("SystemDrive"))));
#else
        return QStorageInfo(QStringLiteral("/"));
#endif
    }

protected:
#if defined(Q_OS_WIN)
    void initRootPath();
    void retrieveVolumeInfo();
    void retrieveDiskFreeSpace();
    bool queryStorageProperty();
    void queryFileFsSectorSizeInformation();
#elif defined(Q_OS_DARWIN)
    void initRootPath();
    void retrievePosixInfo();
    void retrieveUrlProperties(bool initRootPath = false);
    void retrieveLabel();
#elif defined(Q_OS_LINUX)
    void retrieveVolumeInfo();

public:
    struct MountInfo {
        QString mountPoint;
        QByteArray fsType;
        QByteArray device;
        QByteArray fsRoot;
        dev_t stDev = 0;
        quint64 mntid = 0;
    };

    void setFromMountInfo(MountInfo &&info)
    {
        rootPath = std::move(info.mountPoint);
        fileSystemType = std::move(info.fsType);
        device = std::move(info.device);
        subvolume = std::move(info.fsRoot);
    }

    QStorageInfoPrivate(MountInfo &&info)
    {
        setFromMountInfo(std::move(info));
    }

#elif defined(Q_OS_UNIX)
    void initRootPath();
    void retrieveVolumeInfo();
#endif

#ifdef Q_OS_UNIX
    // Common helper functions
    template <typename String>
    static bool isParentOf(const String &parent, const QString &dirName)
    {
        return dirName.startsWith(parent) &&
                (dirName.size() == parent.size() || dirName.at(parent.size()) == u'/' ||
                 parent.size() == 1);
    }
    static inline bool shouldIncludeFs(const QString &mountDir, const QByteArray &fsType);
#endif

public:
    QString rootPath;
    QByteArray device;
    QByteArray subvolume;
    QByteArray fileSystemType;
    QString name;

    qint64 bytesTotal = -1;
    qint64 bytesFree = -1;
    qint64 bytesAvailable = -1;
    int blockSize = -1;

    bool readOnly = false;
    bool ready = false;
    bool valid = false;
};

#ifdef Q_OS_UNIX
bool QStorageInfoPrivate::shouldIncludeFs(const QString &mountDir, const QByteArray &fsType)
{
#if defined(Q_OS_ANDROID)
    // "rootfs" is the filesystem type of "/" on Android
    static constexpr char RootFsStr[] = "";
#else
    // "rootfs" is a type of ramfs on Linux, used in the initrd on some distros
    static constexpr char RootFsStr[] = "rootfs";
#endif

    using namespace Qt::StringLiterals;
    /*
     * This function implements a heuristic algorithm to determine whether a
     * given mount should be reported to the user. Our objective is to list
     * only entries that the end-user would find useful.
     *
     * We therefore ignore:
     *  - mounted in /dev, /proc, /sys: special mounts
     *    (this will catch /sys/fs/cgroup, /proc/sys/fs/binfmt_misc, /dev/pts,
     *    some of which are tmpfs on Linux)
     *  - mounted in /var/run or /var/lock: most likely pseudofs
     *    (on earlier systemd versions, /var/run was a bind-mount of /run, so
     *    everything would be unnecessarily duplicated)
     *  - filesystem type is "rootfs": artifact of the root-pivot on some Linux
     *    initrd
     *  - if the filesystem total size is zero, it's a pseudo-fs (not checked here).
     */

    if (isParentOf("/dev"_L1, mountDir)
        || isParentOf("/proc"_L1, mountDir)
        || isParentOf("/sys"_L1, mountDir)
        || isParentOf("/var/run"_L1, mountDir)
        || isParentOf("/var/lock"_L1, mountDir)) {
        return false;
    }

    if (!fsType.isEmpty() && fsType == RootFsStr)
        return false;

    // size checking in QStorageInfo::mountedVolumes()
    return true;
}
#endif // Q_OS_UNIX

QT_END_NAMESPACE

#endif // QSTORAGEINFO_P_H
