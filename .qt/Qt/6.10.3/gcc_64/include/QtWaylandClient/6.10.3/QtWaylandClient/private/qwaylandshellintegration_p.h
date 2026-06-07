// Copyright (C) 2016 Jolla Ltd
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWAYLANDSHELLINTEGRATION_H
#define QWAYLANDSHELLINTEGRATION_H

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
#include <QtWaylandClient/qwaylandclientextension.h>



#include <QDebug>
#include <private/qglobal_p.h>

struct wl_surface;
struct wl_registry;

QT_BEGIN_NAMESPACE

class QWindow;

namespace QtWaylandClient {

class QWaylandWindow;
class QWaylandDisplay;
class QWaylandShellSurface;

class Q_WAYLANDCLIENT_EXPORT QWaylandShellIntegration
{
public:
    QWaylandShellIntegration() {}
    virtual ~QWaylandShellIntegration();

    virtual bool initialize(QWaylandDisplay *display) = 0;
    virtual QWaylandShellSurface *createShellSurface(QWaylandWindow *window) = 0;
    virtual void *nativeResourceForWindow(const QByteArray &resource, QWindow *window) {
        Q_UNUSED(resource);
        Q_UNUSED(window);
        return nullptr;
    }

    static wl_surface *wlSurfaceForWindow(QWaylandWindow *window);

};

template <typename T>
class Q_WAYLANDCLIENT_EXPORT QWaylandShellIntegrationTemplate
    : public QWaylandClientExtension,
      public QWaylandShellIntegration
{
public:
    QWaylandShellIntegrationTemplate(const int ver) :
        QWaylandClientExtension(ver)
    {
    }

    bool initialize(QWaylandDisplay *) override
    {
        QWaylandClientExtension::initialize();
        return isActive();
    }

    const struct wl_interface *extensionInterface() const override
    {
        return T::interface();
    }

    void bind(struct ::wl_registry *registry, int id, int ver) override
    {
        T* instance = static_cast<T *>(this);
        // Make sure lowest version is used of the supplied version from the
        // developer and the version specified in the protocol and also the
        // compositor version.
        if (this->version() > T::interface()->version) {
            qWarning("Supplied protocol version to QWaylandClientExtensionTemplate is higher "
                     "than the version of the protocol, using protocol version instead.\n"
                     " interface.name: %s",
                     T::interface()->name);
        }
        int minVersion = qMin(ver, qMin(T::interface()->version, this->version()));
        setVersion(minVersion);
        instance->init(registry, id, minVersion);
    }
};


}

QT_END_NAMESPACE

#endif // QWAYLANDSHELLINTEGRATION_H
