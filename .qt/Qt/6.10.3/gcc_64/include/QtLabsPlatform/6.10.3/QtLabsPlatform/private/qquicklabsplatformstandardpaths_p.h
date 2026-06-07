// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKLABSPLATFORMSTANDARDPATHS_P_H
#define QQUICKLABSPLATFORMSTANDARDPATHS_P_H

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

#include <QtCore/qobject.h>
#include <QtCore/qstandardpaths.h>
#include <QtCore/qurl.h>
#include <QtQml/qqml.h>
#include <QtCore/private/qglobal_p.h>

#if QT_DEPRECATED_SINCE(6, 4)

QT_BEGIN_NAMESPACE

class QQmlEngine;
class QJSEngine;

class QQuickLabsPlatformStandardPaths : public QObject
{
    Q_OBJECT
    QML_SINGLETON
    QML_NAMED_ELEMENT(StandardPaths)
    QML_EXTENDED_NAMESPACE(QStandardPaths)

public:
    explicit QQuickLabsPlatformStandardPaths(QObject *parent = nullptr);

    static QObject *create(QQmlEngine *engine, QJSEngine *scriptEngine);

    Q_INVOKABLE static QString displayName(QStandardPaths::StandardLocation type);
    Q_INVOKABLE static QUrl findExecutable(const QString &executableName, const QStringList &paths = QStringList());
    Q_INVOKABLE static QUrl locate(QStandardPaths::StandardLocation type, const QString &fileName, QStandardPaths::LocateOptions options = QStandardPaths::LocateFile);
    Q_INVOKABLE static QList<QUrl> locateAll(QStandardPaths::StandardLocation type, const QString &fileName, QStandardPaths::LocateOptions options = QStandardPaths::LocateFile);
    Q_INVOKABLE static void setTestModeEnabled(bool testMode);
    Q_INVOKABLE static QList<QUrl> standardLocations(QStandardPaths::StandardLocation type);
    Q_INVOKABLE static QUrl writableLocation(QStandardPaths::StandardLocation type);

private:
    Q_DISABLE_COPY(QQuickLabsPlatformStandardPaths)
};

QT_END_NAMESPACE

#endif // QT_DEPRECATED_SINCE(6, 4)

#endif // QQUICKLABSPLATFORMSTANDARDPATHS_P_H
