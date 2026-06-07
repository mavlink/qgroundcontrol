// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQUICKWIDGET_P_H
#define QQUICKWIDGET_P_H

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

#include "qquickwidget.h"
#include <private/qwidget_p.h>
#include <rhi/qrhi.h>
#include <private/qbackingstorerhisupport_p.h>

#include <QtCore/qurl.h>
#include <QtCore/qelapsedtimer.h>
#include <QtCore/qtimer.h>
#include <QtCore/qpointer.h>
#include <QtCore/QWeakPointer>

#include <QtQml/qqmlengine.h>

#include "private/qquickitemchangelistener_p.h"

QT_BEGIN_NAMESPACE

class QQmlContext;
class QQmlError;
class QQuickItem;
class QQmlComponent;
class QQuickRenderControl;

class QQuickWidgetPrivate
        : public QWidgetPrivate,
          public QSafeQuickItemChangeListener<QQuickWidgetPrivate>
{
    Q_DECLARE_PUBLIC(QQuickWidget)
public:
    static QQuickWidgetPrivate* get(QQuickWidget *view) { return view->d_func(); }
    static const QQuickWidgetPrivate* get(const QQuickWidget *view) { return view->d_func(); }

    QQuickWidgetPrivate();

    void executeHelper();
    void destroy();
    void execute();
    void execute(QAnyStringView uri, QAnyStringView typeName);
    void itemGeometryChanged(QQuickItem *item, QQuickGeometryChange change, const QRectF &oldGeometry) override;
    void initResize();
    void updateSize();
    void updatePosition();
    void updateFrambufferObjectSize();
    bool setRootObject(QObject *);
    void render(bool needsSync);
    void renderSceneGraph();
    void initializeWithRhi();
    void handleContextCreationFailure(const QSurfaceFormat &format);

    QPlatformBackingStoreRhiConfig rhiConfig() const override;
    TextureData texture() const override;
    QPlatformTextureList::Flags textureListFlags() override;
    QImage grabFramebuffer() override;

    void init(QQmlEngine* e = nullptr);
    void ensureBackingScene();
    void initOffscreenWindow();
    void ensureEngine() const;
    void handleWindowChange();
    void invalidateRenderControl();

    QSize rootObjectSize() const;

    QPointer<QQuickItem> root;

    QUrl source;

    mutable QPointer<QQmlEngine> engine;
    QQmlComponent *component;
    QBasicTimer resizetimer;
    QQuickWindow *offscreenWindow;
    QQuickRenderControl *renderControl;

    QRhi *rhi;
    QRhiTexture *outputTexture;
    QRhiRenderBuffer *depthStencil;
    QRhiRenderBuffer *msaaBuffer;
    QRhiTextureRenderTarget *rt;
    QRhiRenderPassDescriptor *rtRp;

    QQuickWidget::ResizeMode resizeMode;
    QSize initialSize;
    QElapsedTimer frameTimer;

    QBasicTimer updateTimer;
    bool eventPending;
    bool updatePending;
    bool fakeHidden;

    int requestedSamples;

    bool useSoftwareRenderer;
    QImage softwareImage;
    QRegion updateRegion;
    bool forceFullUpdate;
    bool deviceLost;

    QBackingStoreRhiSupport offscreenRenderer;

    QVariantMap initialProperties;
};

class QQuickWidgetOffscreenWindow: public QQuickWindow
{
    Q_OBJECT

public:
    QQuickWidgetOffscreenWindow(QQuickWindowPrivate &dd, QQuickRenderControl *control);
};

QT_END_NAMESPACE

#endif // QQuickWidget_P_H
