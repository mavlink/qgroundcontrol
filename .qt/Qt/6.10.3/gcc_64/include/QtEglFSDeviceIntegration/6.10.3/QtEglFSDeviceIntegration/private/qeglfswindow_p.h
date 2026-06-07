// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QEGLFSWINDOW_H
#define QEGLFSWINDOW_H

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
#include "qeglfsintegration_p.h"
#include "qeglfsscreen_p.h"

#include <qpa/qplatformwindow.h>
#ifndef QT_NO_OPENGL
# include <QtOpenGL/private/qopenglcompositor_p.h>
#endif

QT_BEGIN_NAMESPACE

class QOpenGLCompositorBackingStore;
class QPlatformTextureList;

#ifndef QT_NO_OPENGL
class Q_EGLFS_EXPORT QEglFSWindow : public QPlatformWindow, public QOpenGLCompositorWindow
#else
class Q_EGLFS_EXPORT QEglFSWindow : public QPlatformWindow
#endif
{
public:
    QEglFSWindow(QWindow *w);
    ~QEglFSWindow();

    void create();
    void destroy();

    void setGeometry(const QRect &) override;
    QRect geometry() const override;
    void setVisible(bool visible) override;
    void requestActivateWindow() override;
    void raise() override;
    void lower() override;

    void propagateSizeHints() override { }
    void setMask(const QRegion &) override { }
    bool setKeyboardGrabEnabled(bool) override { return false; }
    bool setMouseGrabEnabled(bool) override { return false; }
    void setOpacity(qreal) override;
    WId winId() const override;

    QSurfaceFormat format() const override;

    EGLNativeWindowType eglWindow() const;
    EGLSurface surface() const;
    QEglFSScreen *screen() const override;

    bool hasNativeWindow() const { return m_flags.testFlag(HasNativeWindow); }

    void invalidateSurface() override;
    virtual void resetSurface();

    bool isRaster() const;

#ifndef QT_NO_OPENGL
    QOpenGLCompositorBackingStore *backingStore() const override { return m_backingStore; }
    void setBackingStore(QOpenGLCompositorBackingStore *backingStore) override;
    QWindow *sourceWindow() const override;
    const QPlatformTextureList *textures() const override;
    void endCompositing() override;
#endif

protected:
#ifndef QT_NO_OPENGL
    QOpenGLCompositorBackingStore *m_backingStore;
    QOpenGLContext *m_rasterCompositingContext;
#endif
    WId m_winId;

    EGLSurface m_surface;
    EGLNativeWindowType m_window;

    EGLConfig m_config;
    QSurfaceFormat m_format;

    enum Flag {
        Created = 0x01,
        HasNativeWindow = 0x02
    };
    Q_DECLARE_FLAGS(Flags, Flag)
    Flags m_flags;
};

QT_END_NAMESPACE

#endif // QEGLFSWINDOW_H
