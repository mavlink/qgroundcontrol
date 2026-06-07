// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOPENGL_PAINTDEVICE_P_H
#define QOPENGL_PAINTDEVICE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of the Qt OpenGL classes.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#include <qopenglpaintdevice.h>
#include <private/qglobal_p.h>

QT_BEGIN_NAMESPACE

class QOpenGLContext;
class QPaintEngine;

class Q_OPENGL_EXPORT QOpenGLPaintDevicePrivate
{
public:
    QOpenGLPaintDevicePrivate(const QSize &size);
    virtual ~QOpenGLPaintDevicePrivate();

    static QOpenGLPaintDevicePrivate *get(QOpenGLPaintDevice *dev) { return dev->d_func(); }

    virtual void beginPaint() { }
    virtual void endPaint() { }

public:
    QSize size;
    QOpenGLContext *ctx;

    qreal dpmx;
    qreal dpmy;
    qreal devicePixelRatio;

    bool flipped;

    QPaintEngine *engine;
};

QT_END_NAMESPACE

#endif // QOPENGL_PAINTDEVICE_P_H
