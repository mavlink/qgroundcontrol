// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWAYLANDCLIENTSHELLAPI_P_H
#define QWAYLANDCLIENTSHELLAPI_P_H

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

// N O T E
// -------
// This file provides a supported API for creating client-side shell
// extensions. Source compatibility will be preserved, but we may break
// forward and backward binary compatibility, even in patch releases.
//
// The supported API contains these classes:
//
// QtWaylandClient::QWaylandShellSurface
// QtWaylandClient::QWaylandShellIntegration
// QtWaylandClient::QWaylandShellIntegrationPlugin

#include "QtWaylandClient/private/qwaylandshellsurface_p.h"
#include "QtWaylandClient/private/qwaylandshellintegration_p.h"
#include "QtWaylandClient/private/qwaylandshellintegrationplugin_p.h"

#endif // QWAYLANDCLIENTSHELLAPI_P_H
