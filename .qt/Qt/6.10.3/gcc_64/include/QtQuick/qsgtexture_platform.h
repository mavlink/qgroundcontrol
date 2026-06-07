// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSGTEXTURE_PLATFORM_H
#define QSGTEXTURE_PLATFORM_H

#include <QtCore/qnativeinterface.h>
#include <QtQuick/qquickwindow.h>

#if QT_CONFIG(opengl)
#include <QtGui/qopengl.h>
#endif

#if QT_CONFIG(vulkan)
#include <QtGui/qvulkaninstance.h>
#endif

#if QT_CONFIG(metal) || defined(Q_QDOC)
#  if defined(__OBJC__) || defined(Q_QDOC)
     @protocol MTLTexture;
#    define QT_OBJC_PROTOCOL(protocol) id<protocol>
#  else
     typedef struct objc_object *id;
#    define QT_OBJC_PROTOCOL(protocol) id
#  endif
#endif

QT_BEGIN_NAMESPACE

namespace QNativeInterface {

#if QT_CONFIG(opengl) || defined(Q_QDOC)
struct Q_QUICK_EXPORT QSGOpenGLTexture
{
    QT_DECLARE_NATIVE_INTERFACE(QSGOpenGLTexture, 1, QSGTexture)
    virtual GLuint nativeTexture() const = 0;
    static QSGTexture *fromNative(GLuint textureId,
                                  QQuickWindow *window,
                                  const QSize &size,
                                  QQuickWindow::CreateTextureOptions options = {});
    static QSGTexture *fromNativeExternalOES(GLuint textureId,
                                             QQuickWindow *window,
                                             const QSize &size,
                                             QQuickWindow::CreateTextureOptions options = {});
};
#endif

#if defined(Q_OS_WIN) || defined(Q_QDOC)
struct Q_QUICK_EXPORT QSGD3D11Texture
{
    QT_DECLARE_NATIVE_INTERFACE(QSGD3D11Texture, 1, QSGTexture)
    virtual void *nativeTexture() const = 0;
    static QSGTexture *fromNative(void *texture,
                                  QQuickWindow *window,
                                  const QSize &size,
                                  QQuickWindow::CreateTextureOptions options = {});
};
struct Q_QUICK_EXPORT QSGD3D12Texture
{
    QT_DECLARE_NATIVE_INTERFACE(QSGD3D12Texture, 1, QSGTexture)
    virtual void *nativeTexture() const = 0;
    virtual int nativeResourceState() const = 0;
    static QSGTexture *fromNative(void *texture,
                                  int resourceState,
                                  QQuickWindow *window,
                                  const QSize &size,
                                  QQuickWindow::CreateTextureOptions options = {});
};
#endif

#if QT_CONFIG(metal) || defined(Q_QDOC)
struct Q_QUICK_EXPORT QSGMetalTexture
{
    QT_DECLARE_NATIVE_INTERFACE(QSGMetalTexture, 1, QSGTexture)
    virtual QT_OBJC_PROTOCOL(MTLTexture) nativeTexture() const = 0;
    static QSGTexture *fromNative(QT_OBJC_PROTOCOL(MTLTexture) texture,
                                  QQuickWindow *window,
                                  const QSize &size,
                                  QQuickWindow::CreateTextureOptions options = {});
};
#endif

#if QT_CONFIG(vulkan) || defined(Q_QDOC)
struct Q_QUICK_EXPORT QSGVulkanTexture
{
    QT_DECLARE_NATIVE_INTERFACE(QSGVulkanTexture, 1, QSGTexture)
    virtual VkImage nativeImage() const = 0;
    virtual VkImageLayout nativeImageLayout() const = 0;
    static QSGTexture *fromNative(VkImage image,
                                  VkImageLayout layout,
                                  QQuickWindow *window,
                                  const QSize &size,
                                  QQuickWindow::CreateTextureOptions options = {});
};
#endif

} // QNativeInterface

QT_END_NAMESPACE

#endif // QSGTEXTURE_PLATFORM_H
