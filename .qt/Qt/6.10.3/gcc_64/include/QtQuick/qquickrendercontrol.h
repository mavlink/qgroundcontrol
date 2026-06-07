// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKRENDERCONTROL_H
#define QQUICKRENDERCONTROL_H

#include <QtCore/qobject.h>
#include <QtQuick/qtquickglobal.h>
#include <QtGui/qimage.h>

QT_BEGIN_NAMESPACE

class QQuickWindow;
class QOpenGLContext;
class QQuickRenderControlPrivate;
class QThread;
class QRhi;
class QRhiCommandBuffer;

class Q_QUICK_EXPORT QQuickRenderControl : public QObject
{
    Q_OBJECT

public:
    explicit QQuickRenderControl(QObject *parent = nullptr);
    ~QQuickRenderControl() override;

    void prepareThread(QThread *targetThread);

    void setSamples(int sampleCount);
    int samples() const;

    bool initialize();

    void invalidate();

    void beginFrame();
    void endFrame();

    void polishItems();
    bool sync();
    void render();

    static QWindow *renderWindowFor(QQuickWindow *win, QPoint *offset = nullptr);
    virtual QWindow *renderWindow(QPoint *offset) { Q_UNUSED(offset); return nullptr; }

    QQuickWindow *window() const;

    QRhi *rhi() const;
    QRhiCommandBuffer *commandBuffer() const;

protected:
    explicit QQuickRenderControl(QQuickRenderControlPrivate &dd, QObject *parent = nullptr);

Q_SIGNALS:
    void renderRequested();
    void sceneChanged();

private:
    Q_DECLARE_PRIVATE(QQuickRenderControl)
};

QT_END_NAMESPACE

#endif // QQUICKRENDERCONTROL_H
