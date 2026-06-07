// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef FILEINFOTHREAD_P_H
#define FILEINFOTHREAD_P_H

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

#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#if QT_CONFIG(filesystemwatcher)
#include <QFileSystemWatcher>
#endif
#include <QFileInfo>
#include <QDir>

#include "fileproperty_p.h"
#include "qquickfolderlistmodel_p.h"

QT_BEGIN_NAMESPACE

class FileInfoThread : public QThread
{
    Q_OBJECT

Q_SIGNALS:
    void directoryChanged(const QString &directory, const QList<FileProperty> &list) const;
    void directoryUpdated(const QString &directory, const QList<FileProperty> &list, int fromIndex, int toIndex) const;
    void sortFinished(const QList<FileProperty> &list) const;
    void statusChanged(QQuickFolderListModel::Status status) const;

public:
    FileInfoThread(QObject *parent = nullptr);
    ~FileInfoThread();

    void clear();
    void removePath(const QString &path);
    void setPath(const QString &path);
    void setRootPath(const QString &path);
    void setSortFlags(QDir::SortFlags flags);
    void setNameFilters(const QStringList & nameFilters);
    void setShowFiles(bool show);
    void setShowDirs(bool showFolders);
    void setShowDirsFirst(bool show);
    void setShowDotAndDotDot(bool on);
    void setShowHidden(bool on);
    void setShowOnlyReadable(bool on);
    void setCaseSensitive(bool on);

public Q_SLOTS:
#if QT_CONFIG(filesystemwatcher)
    void dirChanged(const QString &directoryPath);
    void updateFile(const QString &path);
#endif

protected:
    void run() override;
    void runOnce();
    void initiateScan();
    void getFileInfos(const QString &path);
    void findChangeRange(const QList<FileProperty> &list, int &fromIndex, int &toIndex);

private:
    enum class UpdateType {
        None = 1 << 0,
        // The order of the files in the current folder changed.
        Sort = 1 << 1,
        // A subset of files in the current folder changed.
        Contents = 1 << 2
    };
    Q_DECLARE_FLAGS(UpdateTypes, UpdateType)

    // Declare these ourselves, as Q_DECLARE_OPERATORS_FOR_FLAGS needs the enum to be public.
    friend constexpr UpdateTypes operator|(UpdateType f1, UpdateTypes f2) noexcept;
    friend constexpr UpdateTypes operator&(UpdateType f1, UpdateTypes f2) noexcept;

    QMutex mutex;
    QWaitCondition condition;
    volatile bool abort;
    bool scanPending;

#if QT_CONFIG(filesystemwatcher)
    QFileSystemWatcher *watcher;
#endif
    QList<FileProperty> currentFileList;
    QDir::SortFlags sortFlags;
    QString currentPath;
    QString rootPath;
    QStringList nameFilters;
    bool needUpdate;
    UpdateTypes updateTypes;
    bool showFiles;
    bool showDirs;
    bool showDirsFirst;
    bool showDotAndDotDot;
    bool showHidden;
    bool showOnlyReadable;
    bool caseSensitive;
};

QT_END_NAMESPACE

#endif // FILEINFOTHREAD_P_H
