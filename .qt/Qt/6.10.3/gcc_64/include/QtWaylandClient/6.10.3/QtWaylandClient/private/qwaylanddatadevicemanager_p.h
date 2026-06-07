// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWAYLANDDATADEVICEMANAGER_H
#define QWAYLANDDATADEVICEMANAGER_H

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

#include <QtWaylandClient/private/qtwaylandclientglobal_p.h>
#include <QtWaylandClient/private/qwayland-wayland.h>

QT_REQUIRE_CONFIG(wayland_datadevice);

QT_BEGIN_NAMESPACE

namespace QtWaylandClient {

class QWaylandDisplay;
class QWaylandDataDevice;
class QWaylandDataSource;
class QWaylandInputDevice;

class Q_WAYLANDCLIENT_EXPORT QWaylandDataDeviceManager : public QtWayland::wl_data_device_manager
{
public:
    QWaylandDataDeviceManager(QWaylandDisplay *display, int version, uint32_t id);
    ~QWaylandDataDeviceManager() override;

    QWaylandDataDevice *getDataDevice(QWaylandInputDevice *inputDevice);

    QWaylandDisplay *display() const;

private:
    QWaylandDisplay *m_display = nullptr;
};

}

QT_END_NAMESPACE

#endif // QWAYLANDDATADEVICEMANAGER_H
