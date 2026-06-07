// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKRENDERCONTROL_P_H
#define QQUICKRENDERCONTROL_P_H

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

#include "qquickrendercontrol.h"
#include <QtQuick/private/qsgcontext_p.h>

QT_BEGIN_NAMESPACE

class QRhi;
class QRhiCommandBuffer;
class QOffscreenSurface;
class QQuickGraphicsConfiguration;

class Q_QUICK_EXPORT QQuickRenderControlPrivate : public QObjectPrivate
{
public:
    Q_DECLARE_PUBLIC(QQuickRenderControl)

    enum FrameStatus {
        NotRecordingFrame,
        RecordingFrame,
        DeviceLostInBeginFrame,
        ErrorInBeginFrame
    };

    QQuickRenderControlPrivate(QQuickRenderControl *renderControl);

    static QQuickRenderControlPrivate *get(QQuickRenderControl *renderControl) {
        return renderControl->d_func();
    }

    static bool isRenderWindowFor(QQuickWindow *quickWin, const QWindow *renderWin);
    virtual bool isRenderWindow(const QWindow *w);

    static void cleanup();

    void windowDestroyed();

    void update();
    void maybeUpdate();

    bool initRhi();
    void resetRhi(const QQuickGraphicsConfiguration &config);

    QImage grab();

    QQuickRenderControl *q;
    bool initialized;
    QQuickWindow *window;
    static QSGContext *sg;
    QSGRenderContext *rc;
    QRhi *rhi;
    bool ownRhi;
    QRhiCommandBuffer *cb;
    QOffscreenSurface *offscreenSurface;
    int sampleCount;
    FrameStatus frameStatus;
};

QT_END_NAMESPACE

#endif // QQUICKRENDERCONTROL_P_H
