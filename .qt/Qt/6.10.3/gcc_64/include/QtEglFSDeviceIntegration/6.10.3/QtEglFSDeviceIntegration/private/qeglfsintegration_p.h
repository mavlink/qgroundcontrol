// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QEGLFSINTEGRATION_H
#define QEGLFSINTEGRATION_H

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
#include <QtCore/QPointer>
#include <QtCore/QVariant>
#include <QtGui/QWindow>
#include <qpa/qplatformintegration.h>
#include <qpa/qplatformnativeinterface.h>
#include <qpa/qplatformopenglcontext.h>
#include <qpa/qplatformscreen.h>
#include <QtGui/private/qkeymapper_p.h>

QT_BEGIN_NAMESPACE

class QEglFSWindow;
class QEglFSContext;
class QFbVtHandler;
#if QT_CONFIG(evdev)
class QEvdevKeyboardManager;
#elif QT_CONFIG(vxworksevdev)
class QVxKeyboardManager;
#endif

class Q_EGLFS_EXPORT QEglFSIntegration : public QPlatformIntegration, public QPlatformNativeInterface
#if QT_CONFIG(evdev)
    , public QNativeInterface::Private::QEvdevKeyMapper
#elif QT_CONFIG(vxworksevdev)
    , public QNativeInterface::Private::QVxKeyMapper
#endif
#ifndef QT_NO_OPENGL
    , public QNativeInterface::Private::QEGLIntegration
#endif
{
public:
    QEglFSIntegration();

    void initialize() override;
    void destroy() override;

    EGLDisplay display() const { return m_display; }

    QAbstractEventDispatcher *createEventDispatcher() const override;
    QPlatformFontDatabase *fontDatabase() const override;
    QPlatformServices *services() const override;
    QPlatformInputContext *inputContext() const override { return m_inputContext; }
    QPlatformTheme *createPlatformTheme(const QString &name) const override;

    QPlatformWindow *createPlatformWindow(QWindow *window) const override;
    QPlatformBackingStore *createPlatformBackingStore(QWindow *window) const override;
#ifndef QT_NO_OPENGL
    QPlatformOpenGLContext *createPlatformOpenGLContext(QOpenGLContext *context) const override;
    QOpenGLContext *createOpenGLContext(EGLContext context, EGLDisplay display, QOpenGLContext *shareContext) const override;
    QPlatformOffscreenSurface *createPlatformOffscreenSurface(QOffscreenSurface *surface) const override;
#endif
    bool hasCapability(QPlatformIntegration::Capability cap) const override;

    QPlatformNativeInterface *nativeInterface() const override;

    // QPlatformNativeInterface
    void *nativeResourceForIntegration(const QByteArray &resource) override;
    void *nativeResourceForScreen(const QByteArray &resource, QScreen *screen) override;
    void *nativeResourceForWindow(const QByteArray &resource, QWindow *window) override;
#ifndef QT_NO_OPENGL
    void *nativeResourceForContext(const QByteArray &resource, QOpenGLContext *context) override;
#endif
    NativeResourceForContextFunction nativeResourceFunctionForContext(const QByteArray &resource) override;

    QFunctionPointer platformFunction(const QByteArray &function) const override;

    QVariant styleHint(QPlatformIntegration::StyleHint hint) const override;

    QFbVtHandler *vtHandler() { return m_vtHandler.data(); }

    QPointer<QWindow> pointerWindow() { return m_pointerWindow; }
    void setPointerWindow(QWindow *pointerWindow) { m_pointerWindow = pointerWindow; }

#if QT_CONFIG(evdev) || QT_CONFIG(vxworksevdev)
    void loadKeymap(const QString &filename) override;
    void switchLang() override;
#endif

protected:
    virtual void createInputHandlers();
#if QT_CONFIG(evdev)
    QEvdevKeyboardManager *m_kbdMgr = nullptr;
#elif QT_CONFIG(vxworksevdev)
    QVxKeyboardManager *m_kbdMgr = nullptr;
#endif

private:
    EGLNativeDisplayType nativeDisplay() const;

    EGLDisplay m_display;
    QPlatformInputContext *m_inputContext;
    QScopedPointer<QPlatformFontDatabase> m_fontDb;
    mutable QScopedPointer<QPlatformServices> m_services;
    QScopedPointer<QFbVtHandler> m_vtHandler;
    QPointer<QWindow> m_pointerWindow;
    bool m_disableInputHandlers;
};

QT_END_NAMESPACE

#endif // QEGLFSINTEGRATION_H
