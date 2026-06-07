// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "private/qeglfshooks_p.h"
#include <EGL/fbdev_window.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/fb.h>

QT_BEGIN_NAMESPACE

class QEglFS8726MHooks : public QEglFSHooks
{
public:
    virtual QSize screenSize() const;
    virtual EGLNativeWindowType createNativeWindow(QPlatformWindow *window, const QSize &size, const QSurfaceFormat &format);
    virtual void destroyNativeWindow(EGLNativeWindowType window);
};

QSize QEglFS8726MHooks::screenSize() const
{
    int fd = open("/dev/fb0", O_RDONLY);
    if (fd == -1) {
        qFatal("Failed to open fb to detect screen resolution!");
    }

    struct fb_var_screeninfo vinfo;
    if (ioctl(fd, FBIOGET_VSCREENINFO, &vinfo) == -1) {
        qFatal("Could not get variable screen info");
    }

    close(fd);

    return QSize(vinfo.xres, vinfo.yres);
}

EGLNativeWindowType QEglFS8726MHooks::createNativeWindow(QPlatformWindow *window, const QSize &size, const QSurfaceFormat &format)
{
    Q_UNUSED(window);
    Q_UNUSED(format);

    fbdev_window *window = new fbdev_window;
    window->width = size.width();
    window->height = size.height();

    return window;
}

void QEglFS8726MHooks::destroyNativeWindow(EGLNativeWindowType window)
{
    delete window;
}

QEglFS8726MHooks eglFS8726MHooks;
QEglFSHooks *platformHooks = &eglFS8726MHooks;

QT_END_NAMESPACE
