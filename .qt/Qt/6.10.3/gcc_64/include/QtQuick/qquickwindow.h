// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKWINDOW_H
#define QQUICKWINDOW_H

#include <QtQuick/qtquickglobal.h>
#include <QtQuick/qsgrendererinterface.h>

#include <QtCore/qmetatype.h>
#include <QtGui/qwindow.h>
#include <QtGui/qevent.h>
#include <QtQml/qqml.h>
#include <QtQml/qqmldebug.h>
#include <QtQml/qqmlinfo.h>

QT_BEGIN_NAMESPACE

class QRunnable;
class QQuickItem;
class QSGTexture;
class QInputMethodEvent;
class QQuickWindowPrivate;
class QQuickWindowAttached;
class QQmlIncubationController;
class QInputMethodEvent;
class QQuickCloseEvent;
class QQuickRenderControl;
class QSGRectangleNode;
class QSGImageNode;
class QSGNinePatchNode;
class QQuickPalette;
class QQuickRenderTarget;
class QQuickGraphicsDevice;
class QQuickGraphicsConfiguration;
class QRhi;
class QRhiSwapChain;
class QRhiTexture;
class QSGTextNode;

class Q_QUICK_EXPORT QQuickWindow : public QWindow
{
    Q_OBJECT
    Q_PRIVATE_PROPERTY(QQuickWindow::d_func(), QQmlListProperty<QObject> data READ data DESIGNABLE false)
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)
    Q_PROPERTY(QQuickItem* contentItem READ contentItem CONSTANT)
    Q_PROPERTY(QQuickItem* activeFocusItem READ activeFocusItem NOTIFY activeFocusItemChanged REVISION(2, 1))
    Q_PRIVATE_PROPERTY(QQuickWindow::d_func(), QQuickPalette *palette READ palette WRITE setPalette
        RESET resetPalette NOTIFY paletteChanged REVISION(6, 2))
    QDOC_PROPERTY(QWindow* transientParent READ transientParent WRITE setTransientParent NOTIFY transientParentChanged)
    Q_CLASSINFO("DefaultProperty", "data")
    Q_DECLARE_PRIVATE(QQuickWindow)

    QML_NAMED_ELEMENT(Window)
    QML_ADDED_IN_VERSION(2, 0)
    QML_REMOVED_IN_VERSION(2, 1)
public:
    enum CreateTextureOption {
        TextureHasAlphaChannel  = 0x0001,
        TextureHasMipmaps       = 0x0002,
        TextureOwnsGLTexture    = 0x0004,
        TextureCanUseAtlas      = 0x0008,
        TextureIsOpaque         = 0x0010
    };

    enum RenderStage {
        BeforeSynchronizingStage,
        AfterSynchronizingStage,
        BeforeRenderingStage,
        AfterRenderingStage,
        AfterSwapStage,
        NoStage
    };

    Q_DECLARE_FLAGS(CreateTextureOptions, CreateTextureOption)
    Q_FLAG(CreateTextureOptions)

    enum SceneGraphError {
        ContextNotAvailable = 1
    };
    Q_ENUM(SceneGraphError)

    enum TextRenderType {
        QtTextRendering,
        NativeTextRendering,
        CurveTextRendering
    };
    Q_ENUM(TextRenderType)

    explicit QQuickWindow(QWindow *parent = nullptr);
    explicit QQuickWindow(QQuickRenderControl *renderControl);

    ~QQuickWindow() override;

    QQuickItem *contentItem() const;

    QQuickItem *activeFocusItem() const;
    QObject *focusObject() const override;

    QQuickItem *mouseGrabberItem() const;

    QImage grabWindow();

    void setRenderTarget(const QQuickRenderTarget &target);
    QQuickRenderTarget renderTarget() const;

    struct GraphicsStateInfo {
        int currentFrameSlot;
        int framesInFlight;
    };
    const GraphicsStateInfo &graphicsStateInfo();
    void beginExternalCommands();
    void endExternalCommands();
    QQmlIncubationController *incubationController() const;

#if QT_CONFIG(accessibility)
    QAccessibleInterface *accessibleRoot() const override;
#endif

    // Scene graph specific functions
    QSGTexture *createTextureFromImage(const QImage &image) const;
    QSGTexture *createTextureFromImage(const QImage &image, CreateTextureOptions options) const;
    QSGTexture *createTextureFromRhiTexture(QRhiTexture *texture, CreateTextureOptions options = {}) const;

    void setColor(const QColor &color);
    QColor color() const;

    static bool hasDefaultAlphaBuffer();
    static void setDefaultAlphaBuffer(bool useAlpha);

    void setPersistentGraphics(bool persistent);
    bool isPersistentGraphics() const;

    void setPersistentSceneGraph(bool persistent);
    bool isPersistentSceneGraph() const;

    bool isSceneGraphInitialized() const;

    void scheduleRenderJob(QRunnable *job, RenderStage schedule);

    qreal effectiveDevicePixelRatio() const;

    QSGRendererInterface *rendererInterface() const;

    static void setGraphicsApi(QSGRendererInterface::GraphicsApi api);
    static QSGRendererInterface::GraphicsApi graphicsApi();

    static void setSceneGraphBackend(const QString &backend);
    static QString sceneGraphBackend();

    void setGraphicsDevice(const QQuickGraphicsDevice &device);
    QQuickGraphicsDevice graphicsDevice() const;

    void setGraphicsConfiguration(const QQuickGraphicsConfiguration &config);
    QQuickGraphicsConfiguration graphicsConfiguration() const;

    QSGRectangleNode *createRectangleNode() const;
    QSGImageNode *createImageNode() const;
    QSGNinePatchNode *createNinePatchNode() const;
    QSGTextNode *createTextNode() const;

    static TextRenderType textRenderType();
    static void setTextRenderType(TextRenderType renderType);

    QRhi *rhi() const;
    QRhiSwapChain *swapChain() const;

Q_SIGNALS:
    void frameSwapped();
    void sceneGraphInitialized();
    void sceneGraphInvalidated();
    void beforeSynchronizing();
    Q_REVISION(2, 2) void afterSynchronizing();
    void beforeRendering();
    void afterRendering();
    Q_REVISION(2, 2) void afterAnimating();
    Q_REVISION(2, 2) void sceneGraphAboutToStop();

    Q_REVISION(2, 1) void closing(QQuickCloseEvent *close);
    void colorChanged(const QColor &);
    Q_REVISION(2, 1) void activeFocusItemChanged();
    Q_REVISION(2, 2) void sceneGraphError(QQuickWindow::SceneGraphError error, const QString &message);

    Q_REVISION(2, 14) void beforeRenderPassRecording();
    Q_REVISION(2, 14) void afterRenderPassRecording();

    Q_REVISION(6, 0) void paletteChanged();
    Q_REVISION(6, 0) void paletteCreated();

    Q_REVISION(6, 0) void beforeFrameBegin();
    Q_REVISION(6, 0) void afterFrameEnd();

public Q_SLOTS:
    void update();
    void releaseResources();

protected:
    QQuickWindow(QQuickWindowPrivate &dd, QWindow *parent = nullptr);
    QQuickWindow(QQuickWindowPrivate &dd, QQuickRenderControl *control);

    void exposeEvent(QExposeEvent *) override;
    void resizeEvent(QResizeEvent *) override;

    void showEvent(QShowEvent *) override;
    void hideEvent(QHideEvent *) override;
    void closeEvent(QCloseEvent *) override;

    void focusInEvent(QFocusEvent *) override;
    void focusOutEvent(QFocusEvent *) override;

    bool event(QEvent *) override;

    // These overrides are no longer normal entry points for
    // input events, but kept in case legacy code calls them.
    void keyPressEvent(QKeyEvent *) override;
    void keyReleaseEvent(QKeyEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
    void mouseReleaseEvent(QMouseEvent *) override;
    void mouseDoubleClickEvent(QMouseEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;
#if QT_CONFIG(wheelevent)
    void wheelEvent(QWheelEvent *) override;
#endif
#if QT_CONFIG(tabletevent)
    void tabletEvent(QTabletEvent *) override;
#endif

private Q_SLOTS:
    void maybeUpdate();
    void cleanupSceneGraph();
    void physicalDpiChanged();
    void handleScreenChanged(QScreen *screen);
    void runJobsAfterSwap();
    void handleApplicationStateChanged(Qt::ApplicationState state);
    void handleFontDatabaseChanged();
private:
#ifndef QT_NO_DEBUG_STREAM
    inline friend QQmlInfo operator<<(QQmlInfo info, const QQuickWindow *window)
    {
        info.QDebug::operator<<(window);
        return info;
    }
#endif

    friend class QQuickItem;
    friend class QQuickItemPrivate;
    friend class QQuickWidget;
    friend class QQuickRenderControl;
    friend class QQuickAnimatorController;
    friend class QQuickWidgetPrivate;
    friend class QQuickDeliveryAgentPrivate;
    Q_DISABLE_COPY(QQuickWindow)
};

#ifndef QT_NO_DEBUG_STREAM
QDebug Q_QUICK_EXPORT operator<<(QDebug debug, const QQuickWindow *item);

inline QQmlInfo operator<<(QQmlInfo info, const QWindow *window)
{
    info.QDebug::operator<<(window);
    return info;
}
#endif

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QQuickWindow *)

#endif // QQUICKWINDOW_H

