// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWAYLANDHARDWAREINTEGRATION_H
#define QWAYLANDHARDWAREINTEGRATION_H

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

#include <QtWaylandClient/private/qwayland-hardware-integration.h>
#include <QtWaylandClient/qtwaylandclientglobal.h>
#include <QtCore/private/qglobal_p.h>

QT_BEGIN_NAMESPACE

namespace QtWaylandClient {

class QWaylandDisplay;

class Q_WAYLANDCLIENT_EXPORT QWaylandHardwareIntegration : public QtWayland::qt_hardware_integration
{
public:
    QWaylandHardwareIntegration(struct ::wl_registry *registry, int id);

    QString clientBufferIntegration();
    QString serverBufferIntegration();

protected:
    void hardware_integration_client_backend(const QString &name) override;
    void hardware_integration_server_backend(const QString &name) override;

private:
    QString m_client_buffer;
    QString m_server_buffer;
};

}

QT_END_NAMESPACE

#endif
