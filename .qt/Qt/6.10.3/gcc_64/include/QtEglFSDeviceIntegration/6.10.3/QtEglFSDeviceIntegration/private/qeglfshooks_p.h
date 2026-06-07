// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QEGLFSHOOKS_H
#define QEGLFSHOOKS_H

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

#include "qeglfsglobal_p.h"
#include "qeglfsdeviceintegration_p.h"

QT_BEGIN_NAMESPACE

class QEglFSHooks : public QEglFSDeviceIntegration
{
};

Q_EGLFS_EXPORT QEglFSDeviceIntegration *qt_egl_device_integration();

QT_END_NAMESPACE

#endif // QEGLFSHOOKS_H
