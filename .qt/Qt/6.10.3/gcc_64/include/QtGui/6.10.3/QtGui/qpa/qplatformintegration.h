// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QPLATFORMINTEGRATION_H
#define QPLATFORMINTEGRATION_H

//
//  W A R N I N G
//  -------------
//
// This file is part of the QPA API and is not meant to be used
// in applications. Usage of this API may make your code
// source and binary incompatible with future versions of Qt.
//

#include <QtGui/qtguiglobal.h>
#include <QtGui/qwindowdefs.h>
#include <qpa/qplatformscreen.h>
#include <QtGui/qsurfaceformat.h>
#include <QtGui/qopenglcontext.h>

QT_BEGIN_NAMESPACE


class QPlatformWindow;
class QWindow;
class QPlatformBackingStore;
class QPlatformFontDatabase;
class QPlatformClipboard;
class QPlatformNativeInterface;
class QPlatformDrag;
class QPlatformOpenGLContext;
class QGuiGLFormat;
class QAbstractEventDispatcher;
class QPlatformInputContext;
class QPlatformKeyMapper;
class QPlatformAccessibility;
class QPlatformTheme;
class QPlatformDialogHelper;
class QPlatformSharedGraphicsCache;
class QPlatformServices;
class QPlatformSessionManager;
class QKeyEvent;
class QPlatformOffscreenSurface;
class QOffscreenSurface;
class QPlatformVulkanInstance;
class QVulkanInstance;

namespace QNativeInterface::Private {

template <typename R, typename I, auto func, typename... Args>
struct QInterfaceProxyImp
{
    template <typename T>
    static R apply(T *obj, Args... args)
    {
        if (auto *iface = dynamic_cast<I*>(obj))
            return (iface->*func)(args...);
        else
            return R();
    }
};

template <auto func>
struct QInterfaceProxy;
template <typename R, typename I, typename... Args, R(I::*func)(Args...)>
struct QInterfaceProxy<func> : public QInterfaceProxyImp<R, I, func, Args...> {};
template <typename R, typename I, typename... Args, R(I::*func)(Args...) const>
struct QInterfaceProxy<func> : public QInterfaceProxyImp<R, I, func, Args...> {};

} // QNativeInterface::Private

class Q_GUI_EXPORT QPlatformIntegration
{
public:
    Q_DISABLE_COPY_MOVE(QPlatformIntegration)

    enum Capability {
        ThreadedPixmaps = 1,
        OpenGL,
        ThreadedOpenGL,
        SharedGraphicsCache,
        BufferQueueingOpenGL,
        WindowMasks,
        MultipleWindows,
        ApplicationState,
        ForeignWindows,
        NonFullScreenWindows,
        NativeWidgets,
        WindowManagement,
        WindowActivation, // whether requestActivate is supported
        SyncState,
        RasterGLSurface,
        AllGLFunctionsQueryable,
        ApplicationIcon,
        SwitchableWidgetComposition,
        TopStackedNativeChildWindows,
        OpenGLOnRasterSurface,
        MaximizeUsingFullscreenGeometry,
        PaintEvents,
        RhiBasedRendering,
        ScreenWindowGrabbing, // whether QScreen::grabWindow() is supported
        BackingStoreStaticContents,
        OffscreenSurface
    };

    virtual ~QPlatformIntegration() { }

    virtual bool hasCapability(Capability cap) const;

    virtual QPlatformPixmap *createPlatformPixmap(QPlatformPixmap::PixelType type) const;
    virtual QPlatformWindow *createPlatformWindow(QWindow *window) const = 0;
    virtual QPlatformWindow *createForeignWindow(QWindow *, WId) const { return nullptr; }
    virtual QPlatformBackingStore *createPlatformBackingStore(QWindow *window) const = 0;
#ifndef QT_NO_OPENGL
    virtual QPlatformOpenGLContext *createPlatformOpenGLContext(QOpenGLContext *context) const;
#endif
    virtual QPlatformSharedGraphicsCache *createPlatformSharedGraphicsCache(const char *cacheId) const;
    virtual QPaintEngine *createImagePaintEngine(QPaintDevice *paintDevice) const;

// Event dispatcher:
    virtual QAbstractEventDispatcher *createEventDispatcher() const = 0;
    virtual void initialize();
    virtual void destroy();

//Deeper window system integrations
    virtual QPlatformFontDatabase *fontDatabase() const;
#ifndef QT_NO_CLIPBOARD
    virtual QPlatformClipboard *clipboard() const;
#endif
#if QT_CONFIG(draganddrop)
    virtual QPlatformDrag *drag() const;
#endif
    virtual QPlatformInputContext *inputContext() const;
#if QT_CONFIG(accessibility)
    virtual QPlatformAccessibility *accessibility() const;
#endif

    // Access native handles. The window handle is already available from Wid;
    virtual QPlatformNativeInterface *nativeInterface() const;

    virtual QPlatformServices *services() const;

    enum StyleHint {
        CursorFlashTime,
        KeyboardInputInterval,
        MouseDoubleClickInterval,
        StartDragDistance,
        StartDragTime,
        KeyboardAutoRepeatRate,
        ShowIsFullScreen,
        PasswordMaskDelay,
        FontSmoothingGamma,
        StartDragVelocity,
        UseRtlExtensions,
        PasswordMaskCharacter,
        SetFocusOnTouchRelease,
        ShowIsMaximized,
        MousePressAndHoldInterval,
        TabFocusBehavior,
        ReplayMousePressOutsidePopup,
        ItemViewActivateItemOnSingleClick,
        UiEffects,
        WheelScrollLines,
        ShowShortcutsInContextMenus,
        MouseQuickSelectionThreshold,
        MouseDoubleClickDistance,
        FlickStartDistance,
        FlickMaximumVelocity,
        FlickDeceleration,
        UnderlineShortcut,
    };

    virtual QVariant styleHint(StyleHint hint) const;
    virtual Qt::WindowState defaultWindowState(Qt::WindowFlags) const;

protected:
    virtual Qt::KeyboardModifiers queryKeyboardModifiers() const;
    virtual QList<int> possibleKeys(const QKeyEvent *) const;
    friend class QPlatformKeyMapper;
public:
    virtual QPlatformKeyMapper *keyMapper() const;

    virtual QStringList themeNames() const;
    virtual QPlatformTheme *createPlatformTheme(const QString &name) const;

    virtual QPlatformOffscreenSurface *createPlatformOffscreenSurface(QOffscreenSurface *surface) const;

#ifndef QT_NO_SESSIONMANAGER
    virtual QPlatformSessionManager *createPlatformSessionManager(const QString &id, const QString &key) const;
#endif

    virtual void sync();

#ifndef QT_NO_OPENGL
    virtual QOpenGLContext::OpenGLModuleType openGLModuleType();
#endif
    virtual void setApplicationIcon(const QIcon &icon) const;
    virtual void setApplicationBadge(qint64 number);

    virtual void beep() const;
    virtual void quit() const;

#if QT_CONFIG(vulkan) || defined(Q_QDOC)
    virtual QPlatformVulkanInstance *createPlatformVulkanInstance(QVulkanInstance *instance) const;
#endif

    template <auto func, typename... Args>
    auto call(Args... args)
    {
        using namespace QNativeInterface::Private;
        return QInterfaceProxy<func>::apply(this, args...);
    }

protected:
    QPlatformIntegration() = default;
};

QT_END_NAMESPACE

#endif // QPLATFORMINTEGRATION_H
