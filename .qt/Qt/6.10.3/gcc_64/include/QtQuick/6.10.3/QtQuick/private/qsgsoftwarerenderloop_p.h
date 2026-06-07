// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSGSOFTWARERENDERLOOP_H
#define QSGSOFTWARERENDERLOOP_H

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

#include <private/qsgrenderloop_p.h>

QT_BEGIN_NAMESPACE

class QBackingStore;

class QSGSoftwareRenderLoop : public QSGRenderLoop
{
    Q_OBJECT
public:
    QSGSoftwareRenderLoop();
    ~QSGSoftwareRenderLoop();

    void show(QQuickWindow *window) override;
    void hide(QQuickWindow *window) override;

    void windowDestroyed(QQuickWindow *window) override;

    void renderWindow(QQuickWindow *window, bool isNewExpose = false);
    void exposureChanged(QQuickWindow *window) override;
    QImage grab(QQuickWindow *window) override;

    void maybeUpdate(QQuickWindow *window) override;
    void update(QQuickWindow *window) override { maybeUpdate(window); } // identical for this implementation.
    void handleUpdateRequest(QQuickWindow *) override;

    void releaseResources(QQuickWindow *) override { }

    QSurface::SurfaceType windowSurfaceType() const override;

    QAnimationDriver *animationDriver() const override { return 0; }

    QSGContext *sceneGraphContext() const override;
    QSGRenderContext *createRenderContext(QSGContext *) const override { return rc; }

    struct WindowData {
        bool updatePending : 1;
        bool grabOnly : 1;
    };

    QHash<QQuickWindow *, WindowData> m_windows;
    QHash<QQuickWindow *, QBackingStore *> m_backingStores;

    QSGContext *sg;
    QSGRenderContext *rc;

    QImage grabContent;
};

QT_END_NAMESPACE

#endif // QSGSOFTWARERENDERLOOP_H
