// Copyright (C) 2015 Pier Luigi Fiorini <pierluigi.fiorini@gmail.com>
// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2016 Pelagicore AG
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QEGLFSKMSGBMSCREEN_H
#define QEGLFSKMSGBMSCREEN_H

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

#include <private/qeglfskmsscreen_p.h>
#include <QMutex>
#include <QWaitCondition>

#include <gbm.h>

QT_BEGIN_NAMESPACE

class QEglFSKmsGbmCursor;

class Q_EGLFS_EXPORT QEglFSKmsGbmScreen : public QEglFSKmsScreen
{
public:
    QEglFSKmsGbmScreen(QEglFSKmsDevice *device, const QKmsOutput &output, bool headless);
    ~QEglFSKmsGbmScreen();

    QPlatformCursor *cursor() const override;

    gbm_surface *createSurface(EGLConfig eglConfig);
    void resetSurface();

    void initCloning(QPlatformScreen *screenThisScreenClones,
                     const QList<QPlatformScreen *> &screensCloningThisScreen);

    void waitForFlip() override;

    virtual void flip();
    virtual void updateFlipStatus();

    virtual uint32_t gbmFlags() { return GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING; }

protected:
    void flipFinished();
    void ensureModeSet(uint32_t fb);
    void cloneDestFlipFinished(QEglFSKmsGbmScreen *cloneDestScreen);
    void waitForFlipWithEventReader(QEglFSKmsGbmScreen *screen);
    static void nonThreadedPageFlipHandler(int fd,
                                           unsigned int sequence,
                                           unsigned int tv_sec,
                                           unsigned int tv_usec,
                                           void *user_data);

    gbm_surface *m_gbm_surface;

    gbm_bo *m_gbm_bo_current;
    gbm_bo *m_gbm_bo_next;
    bool m_flipPending;

    QMutex m_flipMutex;
    QWaitCondition m_flipCond;
    static QMutex m_nonThreadedFlipMutex;

    QScopedPointer<QEglFSKmsGbmCursor> m_cursor;

    struct FrameBuffer {
        uint32_t fb = 0;
    };
    static void bufferDestroyedHandler(gbm_bo *bo, void *data);
    FrameBuffer *framebufferForBufferObject(gbm_bo *bo);

    QEglFSKmsGbmScreen *m_cloneSource;
    struct CloneDestination {
        QEglFSKmsGbmScreen *screen = nullptr;
        bool cloneFlipPending = false;
    };
    QList<CloneDestination> m_cloneDests;

    bool needsNewModeSetForNextFb = false;
};

QT_END_NAMESPACE

#endif // QEGLFSKMSGBMSCREEN_H
