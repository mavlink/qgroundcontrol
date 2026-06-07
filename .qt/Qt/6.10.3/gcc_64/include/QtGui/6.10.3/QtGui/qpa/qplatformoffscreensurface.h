// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QPLATFORMOFFSCREENSURFACE_H
#define QPLATFORMOFFSCREENSURFACE_H

//
//  W A R N I N G
//  -------------
//
// This file is part of the QPA API and is not meant to be used
// in applications. Usage of this API may make your code
// source and binary incompatible with future versions of Qt.
//

#include "qplatformsurface.h"

#include <QtGui/qoffscreensurface.h>
#include <QtCore/qscopedpointer.h>

#include <QtCore/qnativeinterface.h>

QT_BEGIN_NAMESPACE

class QOffscreenSurface;
class QPlatformScreen;
class QPlatformOffscreenSurfacePrivate;

class Q_GUI_EXPORT QPlatformOffscreenSurface : public QPlatformSurface
{
    Q_DECLARE_PRIVATE(QPlatformOffscreenSurface)
public:
    explicit QPlatformOffscreenSurface(QOffscreenSurface *offscreenSurface);
    ~QPlatformOffscreenSurface() override;

    QOffscreenSurface *offscreenSurface() const;

    QPlatformScreen *screen() const override;

    virtual QSurfaceFormat format() const override;
    virtual bool isValid() const;

protected:
    QScopedPointer<QPlatformOffscreenSurfacePrivate> d_ptr;
    friend class QOffscreenSurfacePrivate;
private:
    Q_DISABLE_COPY(QPlatformOffscreenSurface)
};

namespace QNativeInterface::Private {

#if defined(Q_OS_ANDROID)
struct Q_GUI_EXPORT QAndroidOffScreenIntegration
{
    QT_DECLARE_NATIVE_INTERFACE(QAndroidOffScreenIntegration)
    virtual QOffscreenSurface *createOffscreenSurface(ANativeWindow *nativeSurface) const = 0;
};
#endif

} // QNativeInterface::Private


QT_END_NAMESPACE

#endif // QPLATFORMOFFSCREENSURFACE_H
