// Copyright (C) 2016 LG Electronics Ltd
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWAYLANDINPUTDEVICEINTEGRATION_H
#define QWAYLANDINPUTDEVICEINTEGRATION_H

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
#include <QtWaylandClient/qtwaylandclientglobal.h>

#include <stdint.h>

QT_BEGIN_NAMESPACE

namespace QtWaylandClient {

class QWaylandDisplay;
class QWaylandInputDevice;

class Q_WAYLANDCLIENT_EXPORT QWaylandInputDeviceIntegration
{
public:
    QWaylandInputDeviceIntegration() {}
    virtual ~QWaylandInputDeviceIntegration() {}

    virtual QWaylandInputDevice *createInputDevice(QWaylandDisplay *d, int version, uint32_t id) = 0;
};

}

QT_END_NAMESPACE

#endif // QWAYLANDINPUTDEVICEINTEGRATION_H
