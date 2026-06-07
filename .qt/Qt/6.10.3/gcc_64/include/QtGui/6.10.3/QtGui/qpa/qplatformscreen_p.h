// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QPLATFORMSCREEN_P_H
#define QPLATFORMSCREEN_P_H

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

#include <QtGui/private/qtguiglobal_p.h>

#include <QtCore/qpointer.h>
#include <QtCore/qnativeinterface.h>

QT_BEGIN_NAMESPACE

class QScreen;

class QPlatformScreenPrivate
{
public:
    QPointer<QScreen> screen;
};

// ----------------- QNativeInterface -----------------

namespace QNativeInterface::Private {

#if QT_CONFIG(xcb) || defined(Q_QDOC)
struct Q_GUI_EXPORT QXcbScreen
{
    QT_DECLARE_NATIVE_INTERFACE(QXcbScreen, 1, QScreen)
    virtual int virtualDesktopNumber() const = 0;
};
#endif

#if QT_CONFIG(vsp2) || defined(Q_QDOC)
struct Q_GUI_EXPORT QVsp2Screen
{
    QT_DECLARE_NATIVE_INTERFACE(QVsp2Screen, 1, QScreen)
    virtual int addLayer(int dmabufFd, const QSize &size, const QPoint &position, uint drmPixelFormat, uint bytesPerLine) = 0;
    virtual void setLayerBuffer(int id, int dmabufFd) = 0;
    virtual void setLayerPosition(int id, const QPoint &position) = 0;
    virtual void setLayerAlpha(int id, qreal alpha) = 0;
    virtual bool removeLayer(int id) = 0;
    virtual void addBlendListener(void (*callback)()) = 0;
};
#endif

#if defined(Q_OS_WEBOS) || defined(Q_QDOC)
struct Q_GUI_EXPORT QWebOSScreen
{
    QT_DECLARE_NATIVE_INTERFACE(QWebOSScreen, 1, QScreen)
    virtual int addLayer(void *gbm_bo, const QRectF &geometry) = 0;
    virtual void setLayerBuffer(int id, void *gbm_bo) = 0;
    virtual void setLayerGeometry(int id, const QRectF &geometry) = 0;
    virtual void setLayerAlpha(int id, qreal alpha) = 0;
    virtual bool removeLayer(int id) = 0;
    virtual void addFlipListener(void (*callback)()) = 0;
};
#endif
} // QNativeInterface::Private

QT_END_NAMESPACE

#endif // QPLATFORMSCREEN_P_H
