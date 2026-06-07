// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLPLUGINIMPORTER_P_H
#define QQMLPLUGINIMPORTER_P_H

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

#include <private/qtqmlglobal_p.h>
#include <private/qqmlimport_p.h>
#include <private/qqmltypeloaderqmldircontent_p.h>

#include <QtCore/qjsonarray.h>
#include <QtCore/qplugin.h>
#include <QtCore/qversionnumber.h>

QT_BEGIN_NAMESPACE

class QQmlPluginImporter
{
    Q_DISABLE_COPY_MOVE(QQmlPluginImporter)

public:
    QQmlPluginImporter(
            const QString &uri, QTypeRevision version, const QQmlTypeLoaderQmldirContent *qmldir,
            QQmlTypeLoader *typeLoader, QList<QQmlError> *errors)
        : uri(uri)
        , qmldirPath(truncateToDirectory(qmldir->qmldirLocation()))
        , qmldir(qmldir)
        , typeLoader(typeLoader)
        , errors(errors)
        , version(version)
    {}

    ~QQmlPluginImporter() = default;

    QTypeRevision importDynamicPlugin(
            const QString &filePath, const QString &pluginId, bool optional);
    QTypeRevision importStaticPlugin(QObject *instance, const QString &pluginId);
    QTypeRevision importPlugins();

    Q_AUTOTEST_EXPORT static bool removePlugin(const QString &pluginId);
    Q_AUTOTEST_EXPORT static QStringList plugins();

private:
    static QString truncateToDirectory(const QString &qmldirFilePath);

    QString resolvePlugin(const QString &qmldirPluginPath, const QString &baseName);
    void finalizePlugin(QObject *instance, const QString &path);

    const QString uri;
    const QString qmldirPath;

    const QQmlTypeLoaderQmldirContent *qmldir = nullptr;
    QQmlTypeLoader *typeLoader = nullptr;
    QList<QQmlError> *errors = nullptr;

    const QTypeRevision version;
};

QT_END_NAMESPACE

#endif // QQMLPLUGINIMPORTER_P_H
