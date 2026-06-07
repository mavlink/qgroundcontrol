// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKWINDOW_P_H
#define QQUICKWINDOW_P_H

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

#include <QtQuick/private/qquickdeliveryagent_p_p.h>
#include <QtQuick/private/qquickevents_p_p.h>
#include <QtQuick/private/qsgcontext_p.h>
#include <QtQuick/private/qquickpaletteproviderprivatebase_p.h>
#include <QtQuick/private/qquickrendertarget_p.h>
#include <QtQuick/private/qquickgraphicsdevice_p.h>
#include <QtQuick/private/qquickgraphicsconfiguration_p.h>
#include <QtQuick/qquickitem.h>
#include <QtQuick/qquickwindow.h>

#include <QtCore/qthread.h>
#include <QtCore/qmutex.h>
#include <QtCore/qwaitcondition.h>
#include <QtCore/qrunnable.h>
#include <QtCore/qstack.h>

#include <QtGui/private/qevent_p.h>
#include <QtGui/private/qpointingdevice_p.h>
#include <QtGui/private/qwindow_p.h>
#include <QtGui/qevent.h>
#include <QtGui/qstylehints.h>
#include <QtGui/qguiapplication.h>

QT_BEGIN_NAMESPACE

class QOpenGLContext;
class QQuickAnimatorController;
class QQuickDragGrabber;
class QQuickItemPrivate;
class QPointingDevice;
class QQuickRenderControl;
class QQuickWindowIncubationController;
class QQuickWindowPrivate;
class QSGRenderLoop;
class QTouchEvent;
class QRhi;
class QRhiSwapChain;
class QRhiRenderBuffer;
class QRhiRenderPassDescriptor;
class QRhiTexture;

Q_DECLARE_LOGGING_CATEGORY(lcQuickWindow)

//Make it easy to identify and customize the root item if needed
class Q_QUICK_EXPORT QQuickRootItem : public QQuickItem
{
    Q_OBJECT
    QML_ANONYMOUS
    QML_ADDED_IN_VERSION(2, 0)
public:
    QQuickRootItem();

public Q_SLOTS:
    void setWidth(int w) {QQuickItem::setWidth(qreal(w));}
    void setHeight(int h) {QQuickItem::setHeight(qreal(h));}
};

struct QQuickWindowRenderTarget
{
    enum class ResetFlag {
        KeepImplicitBuffers = 0x01
    };
    Q_DECLARE_FLAGS(ResetFlags, ResetFlag)
    void reset(QRhi *rhi, ResetFlags flags = {});

    struct {
        QRhiRenderTarget *renderTarget = nullptr;
        bool owns = false;
        int multiViewCount = 1;
    } rt;
    struct {
        QRhiTexture *texture = nullptr;
        QRhiRenderBuffer *renderBuffer = nullptr;
        QRhiRenderPassDescriptor *rpDesc = nullptr;
    } res;
    struct ImplicitBuffers {
        QRhiRenderBuffer *depthStencil = nullptr;
        QRhiTexture *depthStencilTexture = nullptr;
        QRhiTexture *multisampleTexture = nullptr;
        void reset(QRhi *rhi);
    } implicitBuffers;
    struct {
        QPaintDevice *paintDevice = nullptr;
        bool owns = false;
    } sw;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QQuickWindowRenderTarget::ResetFlags)

class Q_QUICK_EXPORT QQuickWindowPrivate
    : public QWindowPrivate
    , public QQuickPaletteProviderPrivateBase<QQuickWindow, QQuickWindowPrivate>
{
public:
    Q_DECLARE_PUBLIC(QQuickWindow)

    enum CustomEvents {
        FullUpdateRequest = QEvent::User + 1,
        TriggerContextCreationFailure = QEvent::User + 2
    };

    static inline QQuickWindowPrivate *get(QQuickWindow *c) { return c->d_func(); }
    static inline const QQuickWindowPrivate *get(const QQuickWindow *c) { return c->d_func(); }

    QQuickWindowPrivate();
    ~QQuickWindowPrivate() override;

    void setPalette(QQuickPalette *p) override;
    void updateWindowPalette();
    void updateChildrenPalettes(const QPalette &parentPalette) override;

    void init(QQuickWindow *, QQuickRenderControl *control = nullptr);

    QQuickRootItem *contentItem;
    QSet<QQuickItem *> parentlessItems;
    QQmlListProperty<QObject> data();

    // primary delivery agent for the whole scene, used by default for events that arrive in this window;
    // but any subscene root can have a QQuickItemPrivate::ExtraData::subsceneDeliveryAgent
    QQuickDeliveryAgent *deliveryAgent = nullptr;
    QQuickDeliveryAgentPrivate *deliveryAgentPrivate() const
    { return deliveryAgent ? static_cast<QQuickDeliveryAgentPrivate *>(QQuickDeliveryAgentPrivate::get(deliveryAgent)) : nullptr; }

#if QT_CONFIG(cursor)
    QQuickItem *cursorItem = nullptr;
    QQuickPointerHandler *cursorHandler = nullptr;
    void updateCursor(const QPointF &scenePos, QQuickItem *rootItem = nullptr);
    std::pair<QQuickItem*, QQuickPointerHandler*> findCursorItemAndHandler(QQuickItem *item, const QPointF &scenePos) const;
#endif

    void clearFocusObject() override;
    void setFocusToTarget(FocusTarget, Qt::FocusReason) override;

    void maybeSynthesizeContextMenuEvent(QMouseEvent *event) override;

    void dirtyItem(QQuickItem *);
    void cleanup(QSGNode *);

    void ensureCustomRenderTarget();
    void setCustomCommandBuffer(QRhiCommandBuffer *cb);

    void polishItems();
    void forcePolish();
    void invalidateFontData(QQuickItem *item);
    void syncSceneGraph();
    void renderSceneGraph();

    bool isRenderable() const;

    bool emitError(QQuickWindow::SceneGraphError error, const QString &msg);

    enum TextureFromNativeTextureFlag {
        NativeTextureIsExternalOES = 0x01
    };
    Q_DECLARE_FLAGS(TextureFromNativeTextureFlags, TextureFromNativeTextureFlag)

    QSGTexture *createTextureFromNativeTexture(quint64 nativeObjectHandle,
                                               int nativeLayoutOrState,
                                               uint nativeFormat,
                                               const QSize &size,
                                               QQuickWindow::CreateTextureOptions options,
                                               TextureFromNativeTextureFlags flags = {}) const;
    QSGTexture *createTextureFromNativeTexture(quint64 nativeObjectHandle,
                                               int nativeLayoutOrState,
                                               const QSize &size,
                                               QQuickWindow::CreateTextureOptions options,
                                               TextureFromNativeTextureFlags flags = {}) const {
        return createTextureFromNativeTexture(nativeObjectHandle, nativeLayoutOrState, 0, size, options, flags);
    }

    QQuickItem::UpdatePaintNodeData updatePaintNodeData;

    QQuickItem *dirtyItemList;
    QList<QSGNode *> cleanupNodeList;

    QVector<QQuickItem *> itemsToPolish;

    qreal lastReportedItemDevicePixelRatio;
    QMetaObject::Connection physicalDpiChangedConnection;
    std::array<QMetaObject::Connection, 7> connections;

    void updateDirtyNodes();
    void cleanupNodes();
    void cleanupNodesOnShutdown();
    bool updateEffectiveOpacity(QQuickItem *);
    void updateEffectiveOpacityRoot(QQuickItem *, qreal);
    void updateDirtyNode(QQuickItem *);

    void fireFrameSwapped() { Q_EMIT q_func()->frameSwapped(); }
    void fireAboutToStop() { Q_EMIT q_func()->sceneGraphAboutToStop(); }

    bool needsChildWindowStackingOrderUpdate = false;
    void updateChildWindowStackingOrder(QQuickItem *item = nullptr);

    int multiViewCount();
    QRhiRenderTarget *activeCustomRhiRenderTarget();

    QSGRenderContext *context;
    QSGRenderer *renderer;
    QByteArray visualizationMode; // Default renderer supports "clip", "overdraw", "changes", "batches" and blank.

    QSGRenderLoop *windowManager;
    QQuickRenderControl *renderControl;
    QScopedPointer<QQuickAnimatorController> animationController;

    QColor clearColor;

    uint persistentGraphics : 1;
    uint persistentSceneGraph : 1;
    uint inDestructor : 1;

    // Storage for setRenderTarget(QQuickRenderTarget).
    // Gets baked into redirect.renderTarget by ensureCustomRenderTarget() when rendering the next frame.
    QQuickRenderTarget customRenderTarget;

    struct Redirect {
        QRhiCommandBuffer *commandBuffer = nullptr;
        QQuickWindowRenderTarget rt;
        bool renderTargetDirty = false;
    } redirect;

    QQuickGraphicsDevice customDeviceObjects;

    QQuickGraphicsConfiguration graphicsConfig;

    mutable QQuickWindowIncubationController *incubationController;

    static bool defaultAlphaBuffer;
    static QQuickWindow::TextRenderType textRenderType;

    // data property
    static void data_append(QQmlListProperty<QObject> *, QObject *);
    static qsizetype data_count(QQmlListProperty<QObject> *);
    static QObject *data_at(QQmlListProperty<QObject> *, qsizetype);
    static void data_clear(QQmlListProperty<QObject> *);
    static void data_removeLast(QQmlListProperty<QObject> *);

    static void rhiCreationFailureMessage(const QString &backendName,
                                          QString *translatedMessage,
                                          QString *untranslatedMessage);

    static void emitBeforeRenderPassRecording(void *ud);
    static void emitAfterRenderPassRecording(void *ud);

    QMutex renderJobMutex;
    QList<QRunnable *> beforeSynchronizingJobs;
    QList<QRunnable *> afterSynchronizingJobs;
    QList<QRunnable *> beforeRenderingJobs;
    QList<QRunnable *> afterRenderingJobs;
    QList<QRunnable *> afterSwapJobs;

    void runAndClearJobs(QList<QRunnable *> *jobs);
    QOpenGLContext *openglContext();

    QQuickWindow::GraphicsStateInfo rhiStateInfo;
    QRhi *rhi = nullptr;
    QRhiSwapChain *swapchain = nullptr;
    QRhiRenderBuffer *depthStencilForSwapchain = nullptr;
    QRhiRenderPassDescriptor *rpDescForSwapchain = nullptr;
    uint hasActiveSwapchain : 1;
    uint hasRenderableSwapchain : 1;
    uint swapchainJustBecameRenderable : 1;
    uint updatesEnabled : 1;
    bool pendingFontUpdate = false;
    bool windowEventDispatch = false;
    bool rmbContextMenuEventEnabled = false; // true after right-mouse press, false when menu opened
    QPointer<QQuickPalette> windowPaletteRef;

private:
    static void cleanupNodesOnShutdown(QQuickItem *);
};

class QQuickWindowQObjectCleanupJob : public QRunnable
{
public:
    QQuickWindowQObjectCleanupJob(QObject *o) : object(o) { }
    void run() override { delete object; }
    QObject *object;
    static void schedule(QQuickWindow *window, QObject *object) {
        Q_ASSERT(window);
        Q_ASSERT(object);
        window->scheduleRenderJob(new QQuickWindowQObjectCleanupJob(object), QQuickWindow::AfterSynchronizingStage);
    }
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QQuickWindowPrivate::TextureFromNativeTextureFlags)

QT_END_NAMESPACE

#endif // QQUICKWINDOW_P_H
