// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of Qt Designer.  This header
// file may change from version to version without notice, or even be removed.
//
// We mean it.
//

#ifndef QTRESOURCEMODEL_H
#define QTRESOURCEMODEL_H

#include "shared_global_p.h"
#include <QtCore/qmap.h>
#include <QtCore/qobject.h>
#include <QtCore/qscopedpointer.h>

QT_BEGIN_NAMESPACE

class QtResourceModel;

class QDESIGNER_SHARED_EXPORT QtResourceSet // one instance per one form
{
public:
    QStringList activeResourceFilePaths() const;

    // activateQrcPaths(): if this QtResourceSet is active it emits resourceSetActivated();
    // otherwise only in case if active QtResource set contains one of
    // paths which was marked as modified by this resource set, the signal
    // is emitted (with reload = true);
    // If new path appears on the list it is automatically added to
    // loaded list of QtResourceModel. In addition it is marked as modified in case
    // QtResourceModel didn't contain the path.
    // If some path is removed from that list (and is not used in any other
    // resource set) it is automatically unloaded. The removed file can also be
    // marked as modified (later when another resource set which contains
    // removed path is activated will be reloaded)
    void activateResourceFilePaths(const QStringList &paths, int *errorCount = nullptr, QString *errorMessages = nullptr);

    bool isModified(const QString &path) const; // for all paths in resource model (redundant here, maybe it should be removed from here)
    void setModified(const QString &path);      // for all paths in resource model (redundant here, maybe it should be removed from here)
private:
    QtResourceSet();
    QtResourceSet(QtResourceModel *model);
    ~QtResourceSet();
    friend class QtResourceModel;

    QScopedPointer<class QtResourceSetPrivate> d_ptr;
    Q_DECLARE_PRIVATE(QtResourceSet)
    Q_DISABLE_COPY_MOVE(QtResourceSet)
};

class QDESIGNER_SHARED_EXPORT QtResourceModel : public QObject // one instance per whole designer
{
    Q_OBJECT
public:
    QtResourceModel(QObject *parent = nullptr);
    ~QtResourceModel();

    QStringList loadedQrcFiles() const;
    bool isModified(const QString &path) const; // only for paths which are on loadedQrcFiles() list
    void setModified(const QString &path);      // only for paths which are on loadedQrcPaths() list

    QList<QtResourceSet *> resourceSets() const;

    QtResourceSet *currentResourceSet() const;
    void setCurrentResourceSet(QtResourceSet *resourceSet, int *errorCount = nullptr, QString *errorMessages = nullptr);

    QtResourceSet *addResourceSet(const QStringList &paths);
    void removeResourceSet(QtResourceSet *resourceSet);

    void reload(const QString &path, int *errorCount = nullptr, QString *errorMessages = nullptr);
    void reload(int *errorCount = nullptr, QString *errorMessages = nullptr);

    // Contents of the current resource set (content file to qrc path)
    QMap<QString, QString> contents() const;
    // Find the qrc file belonging to the contained file (from current resource set)
    QString qrcPath(const QString &file) const;

    void setWatcherEnabled(bool enable);
    bool isWatcherEnabled() const;

    void setWatcherEnabled(const QString &path, bool enable);
    bool isWatcherEnabled(const QString &path);

signals:
    void resourceSetActivated(QtResourceSet *resourceSet, bool resourceSetChanged); // resourceSetChanged since last time it was activated!
    void qrcFileModifiedExternally(const QString &path);

private:
    friend class QtResourceSet;

    QScopedPointer<class QtResourceModelPrivate> d_ptr;
    Q_DECLARE_PRIVATE(QtResourceModel)
    Q_DISABLE_COPY_MOVE(QtResourceModel)
};

QT_END_NAMESPACE

#endif
