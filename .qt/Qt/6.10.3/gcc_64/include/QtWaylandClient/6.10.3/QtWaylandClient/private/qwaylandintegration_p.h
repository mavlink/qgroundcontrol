// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QPLATFORMINTEGRATION_WAYLAND_H
#define QPLATFORMINTEGRATION_WAYLAND_H

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
#include <qpa/qplatformintegration.h>
#include <qpa/qplatformopenglcontext.h>
#include <QtCore/QScopedPointer>
#include <QtCore/QMutex>
#include <QtCore/private/qglobal_p.h>

QT_BEGIN_NAMESPACE

namespace QtWaylandClient {

class QWaylandBuffer;
class QWaylandDisplay;
class QWaylandClientBufferIntegration;
class QWaylandServerBufferIntegration;
class QWaylandShellIntegration;
class QWaylandInputDeviceIntegration;
class QWaylandInputDevice;
class QWaylandScreen;
class QWaylandCursor;
class QWaylandPlatformServices;

class Q_WAYLANDCLIENT_EXPORT QWaylandIntegration : public QPlatformIntegration
#if QT_CONFIG(opengl)
    , public QNativeInterface::Private::QEGLIntegration
#endif
{
public:
    QWaylandIntegration(const QString &platformName);
    ~QWaylandIntegration() override;

    static QWaylandIntegration *instance() { return sInstance; }

    bool init();

    bool hasCapability(QPlatformIntegration::Capability cap) const override;
    QPlatformWindow *createPlatformWindow(QWindow *window) const override;
#if QT_CONFIG(opengl)
    QPlatformOpenGLContext *createPlatformOpenGLContext(QOpenGLContext *context) const override;
    QOpenGLContext *createOpenGLContext(EGLContext context, EGLDisplay display, QOpenGLContext *shareContext) const override;
#endif
    QPlatformBackingStore *createPlatformBackingStore(QWindow *window) const override;

    QAbstractEventDispatcher *createEventDispatcher() const override;
    void initialize() override;

    QPlatformFontDatabase *fontDatabase() const override;

    QPlatformNativeInterface *nativeInterface() const override;
#if QT_CONFIG(clipboard)
    QPlatformClipboard *clipboard() const override;
#endif
#if QT_CONFIG(draganddrop)
    QPlatformDrag *drag() const override;
#endif
    QPlatformInputContext *inputContext() const override;

    QVariant styleHint(StyleHint hint) const override;

#if QT_CONFIG(accessibility)
    QPlatformAccessibility *accessibility() const override;
#endif

    QPlatformServices *services() const override;

    QWaylandDisplay *display() const;

    Qt::KeyboardModifiers queryKeyboardModifiers() const override;

    QList<int> possibleKeys(const QKeyEvent *event) const override;

    QStringList themeNames() const override;

    QPlatformTheme *createPlatformTheme(const QString &name) const override;

#if QT_CONFIG(vulkan)
    QPlatformVulkanInstance *createPlatformVulkanInstance(QVulkanInstance *instance) const override;
#endif

    void setApplicationBadge(qint64 number) override;
    void beep() const override;

    virtual QWaylandInputDevice *createInputDevice(QWaylandDisplay *display, int version, uint32_t id) const;
    virtual QWaylandScreen *createPlatformScreen(QWaylandDisplay *waylandDisplay, int version, uint32_t id) const;
    virtual QWaylandCursor *createPlatformCursor(QWaylandDisplay *display) const;

    virtual QWaylandClientBufferIntegration *clientBufferIntegration() const;
    virtual QWaylandServerBufferIntegration *serverBufferIntegration() const;
    virtual QWaylandShellIntegration *shellIntegration() const;

    void reconfigureInputContext();

protected:
    // NOTE: mDisplay *must* be destructed after mDrag and mClientBufferIntegration
    // and mShellIntegration.
    // Do not move this definition into the private section at the bottom.
    QScopedPointer<QWaylandDisplay> mDisplay;

protected:
    void reset();
    virtual QPlatformNativeInterface *createPlatformNativeInterface();

    QScopedPointer<QWaylandClientBufferIntegration> mClientBufferIntegration;
    QScopedPointer<QWaylandServerBufferIntegration> mServerBufferIntegration;
    QScopedPointer<QWaylandShellIntegration> mShellIntegration;
    QScopedPointer<QWaylandInputDeviceIntegration> mInputDeviceIntegration;

    QScopedPointer<QPlatformInputContext> mInputContext;

private:
    void initializePlatform();
    void initializeClientBufferIntegration();
    void initializeServerBufferIntegration();
    void initializeShellIntegration();
    void initializeInputDeviceIntegration();
    QWaylandShellIntegration *createShellIntegration(const QString& interfaceName);

    const QString mPlatformName;
    QScopedPointer<QPlatformFontDatabase> mFontDb;
#if QT_CONFIG(clipboard)
    QScopedPointer<QPlatformClipboard> mClipboard;
#endif
#if QT_CONFIG(draganddrop)
    QScopedPointer<QPlatformDrag> mDrag;
#endif
    QScopedPointer<QPlatformNativeInterface> mNativeInterface;
#if QT_CONFIG(accessibility)
    mutable QScopedPointer<QPlatformAccessibility> mAccessibility;
#endif
    QScopedPointer<QWaylandPlatformServices> mPlatformServices;
    QMutex mClientBufferInitLock;
    bool mClientBufferIntegrationInitialized = false;
    bool mServerBufferIntegrationInitialized = false;
    bool mShellIntegrationInitialized = false;

    static QWaylandIntegration *sInstance;

    friend class QWaylandDisplay;
};

}

QT_END_NAMESPACE

#endif
