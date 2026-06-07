// Copyright (C) 2024 David Redondo <kde@david-redondo.de>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWAYLANDAPPMENU_H
#define QWAYLANDAPPMENU_H

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

#include <QtCore/QObject>

#include <QtWaylandClient/qtwaylandclientglobal.h>
#include <QtWaylandClient/private/qwayland-appmenu.h>

QT_BEGIN_NAMESPACE

namespace QtWaylandClient {

class QWaylandAppMenu : public QObject, public QtWayland::org_kde_kwin_appmenu
{
public:
    QWaylandAppMenu();
    ~QWaylandAppMenu();
};

class QWaylandAppMenuManager : public QtWayland::org_kde_kwin_appmenu_manager
{
public:
    QWaylandAppMenuManager(wl_registry *registry, quint32 id, int version);
    ~QWaylandAppMenuManager();
};

} // namespace QtWaylandClient
QT_END_NAMESPACE

#endif
