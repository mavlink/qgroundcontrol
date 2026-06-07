// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWAYLANDSERVERBUFFERINTEGRATIONPLUGIN_H
#define QWAYLANDSERVERBUFFERINTEGRATIONPLUGIN_H

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

#include <QtWaylandClient/qtwaylandclientglobal.h>

#include <QtCore/qplugin.h>
#include <QtCore/qfactoryinterface.h>
#include <QtCore/QObject>
#include <QtCore/private/qglobal_p.h>

QT_BEGIN_NAMESPACE

namespace QtWaylandClient {

class QWaylandServerBufferIntegration;

#define QWaylandServerBufferIntegrationFactoryInterface_iid "org.qt-project.Qt.WaylandClient.QWaylandServerBufferIntegrationFactoryInterface.5.3"

class Q_WAYLANDCLIENT_EXPORT QWaylandServerBufferIntegrationPlugin : public QObject
{
    Q_OBJECT
public:
    explicit QWaylandServerBufferIntegrationPlugin(QObject *parent = nullptr);
    ~QWaylandServerBufferIntegrationPlugin() override;

    virtual QWaylandServerBufferIntegration *create(const QString &key, const QStringList &paramList) = 0;
};

}

QT_END_NAMESPACE

#endif // QWAYLANDSERVERBUFFERINTEGRATIONPLUGIN_H
