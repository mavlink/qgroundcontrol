// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QGUIAPPLICATION_PLATFORM_H
#define QGUIAPPLICATION_PLATFORM_H

//
//  W A R N I N G
//  -------------
//
// This file is part of the native interface APIs. Usage of
// this API may make your code source and binary incompatible
// with future versions of Qt.
//

#include <QtGui/qtguiglobal.h>

#include <QtCore/qnativeinterface.h>
#include <QtGui/qguiapplication.h>

#if QT_CONFIG(xcb)
typedef struct _XDisplay Display;
struct xcb_connection_t;
#endif

#if QT_CONFIG(wayland)
struct wl_display;
struct wl_compositor;
struct wl_seat;
struct wl_keyboard;
struct wl_pointer;
struct wl_touch;
#endif

#if defined(Q_OS_VISIONOS) || defined(Q_QDOC)
#  ifdef __OBJC__
Q_FORWARD_DECLARE_OBJC_CLASS(CP_OBJECT_cp_layer_renderer_capabilities);
typedef CP_OBJECT_cp_layer_renderer_capabilities *cp_layer_renderer_capabilities_t;
Q_FORWARD_DECLARE_OBJC_CLASS(CP_OBJECT_cp_layer_renderer_configuration);
typedef CP_OBJECT_cp_layer_renderer_configuration *cp_layer_renderer_configuration_t;
Q_FORWARD_DECLARE_OBJC_CLASS(CP_OBJECT_cp_layer_renderer);
typedef CP_OBJECT_cp_layer_renderer *cp_layer_renderer_t;
#  else
typedef struct cp_layer_renderer_capabilities_s *cp_layer_renderer_capabilities_t;
typedef struct cp_layer_renderer_configuration_s *cp_layer_renderer_configuration_t;
typedef struct cp_layer_renderer_s *cp_layer_renderer_t;
#  endif
#endif


QT_BEGIN_NAMESPACE

namespace QNativeInterface
{

#if QT_CONFIG(xcb) || defined(Q_QDOC)
struct Q_GUI_EXPORT QX11Application
{
    QT_DECLARE_NATIVE_INTERFACE(QX11Application, 1, QGuiApplication)
    virtual Display *display() const = 0;
    virtual xcb_connection_t *connection() const = 0;
};
#endif

#if QT_CONFIG(wayland) || defined(Q_QDOC)
struct Q_GUI_EXPORT QWaylandApplication
{
    QT_DECLARE_NATIVE_INTERFACE(QWaylandApplication, 1, QGuiApplication)
    virtual wl_display *display() const = 0;
    virtual wl_compositor *compositor() const = 0;
    virtual wl_seat *seat() const = 0;
    virtual wl_keyboard *keyboard() const = 0;
    virtual wl_pointer *pointer() const = 0;
    virtual wl_touch *touch() const = 0;
    virtual uint lastInputSerial() const = 0;
    virtual wl_seat *lastInputSeat() const = 0;
};
#endif

#if defined(Q_OS_VISIONOS) || defined(Q_QDOC)
struct Q_GUI_EXPORT QVisionOSApplication
{
    QT_DECLARE_NATIVE_INTERFACE(QVisionOSApplication, 1, QGuiApplication)
    struct ImmersiveSpaceCompositorLayer {
        virtual void configure(cp_layer_renderer_capabilities_t, cp_layer_renderer_configuration_t) const {}
        virtual void render(cp_layer_renderer_t) = 0;
        virtual void handleSpatialEvents(const QJsonObject &) {};
    };
    virtual void setImmersiveSpaceCompositorLayer(ImmersiveSpaceCompositorLayer *layer) = 0;
    virtual void openImmersiveSpace() = 0;
    virtual void dismissImmersiveSpace() = 0;
};
#endif

} // QNativeInterface

QT_END_NAMESPACE

#endif // QGUIAPPLICATION_PLATFORM_H
