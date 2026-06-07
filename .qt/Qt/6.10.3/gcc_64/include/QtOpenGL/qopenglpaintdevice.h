// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOPENGLPAINTDEVICE_H
#define QOPENGLPAINTDEVICE_H

#include <QtOpenGL/qtopenglglobal.h>

#ifndef QT_NO_OPENGL

#include <QtGui/qpaintdevice.h>
#include <QtGui/qopengl.h>
#include <QtGui/qopenglcontext.h>

QT_BEGIN_NAMESPACE

class QOpenGLPaintDevicePrivate;

class Q_OPENGL_EXPORT QOpenGLPaintDevice : public QPaintDevice
{
    Q_DECLARE_PRIVATE(QOpenGLPaintDevice)
public:
    QOpenGLPaintDevice();
    explicit QOpenGLPaintDevice(const QSize &size);
    QOpenGLPaintDevice(int width, int height);
    ~QOpenGLPaintDevice();

    int devType() const override { return QInternal::OpenGL; }
    QPaintEngine *paintEngine() const override;

    QOpenGLContext *context() const;
    QSize size() const;
    void setSize(const QSize &size);
    void setDevicePixelRatio(qreal devicePixelRatio);

    qreal dotsPerMeterX() const;
    qreal dotsPerMeterY() const;

    void setDotsPerMeterX(qreal);
    void setDotsPerMeterY(qreal);

    void setPaintFlipped(bool flipped);
    bool paintFlipped() const;

    virtual void ensureActiveTarget();

protected:
    QOpenGLPaintDevice(QOpenGLPaintDevicePrivate &dd);
    int metric(QPaintDevice::PaintDeviceMetric metric) const override;

    Q_DISABLE_COPY(QOpenGLPaintDevice)
    QScopedPointer<QOpenGLPaintDevicePrivate> d_ptr;
};

QT_END_NAMESPACE

#endif // QT_NO_OPENGL

#endif // QOPENGLPAINTDEVICE_H
