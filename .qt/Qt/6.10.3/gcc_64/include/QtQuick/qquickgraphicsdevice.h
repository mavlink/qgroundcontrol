// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKGRAPHICSDEVICE_H
#define QQUICKGRAPHICSDEVICE_H

#include <QtQuick/qtquickglobal.h>

#if QT_CONFIG(vulkan)
#include <QtGui/qvulkaninstance.h>
#endif

#if QT_CONFIG(metal) || defined(Q_QDOC)
Q_FORWARD_DECLARE_OBJC_CLASS(MTLDevice);
Q_FORWARD_DECLARE_OBJC_CLASS(MTLCommandQueue);
#endif

QT_BEGIN_NAMESPACE

class QQuickGraphicsDevicePrivate;
class QOpenGLContext;
class QRhi;
class QRhiAdapter;

class Q_QUICK_EXPORT QQuickGraphicsDevice
{
public:
    QQuickGraphicsDevice();
    ~QQuickGraphicsDevice();
    QQuickGraphicsDevice(const QQuickGraphicsDevice &other);
    QQuickGraphicsDevice &operator=(const QQuickGraphicsDevice &other);

    bool isNull() const;

#if QT_CONFIG(opengl) || defined(Q_QDOC)
    static QQuickGraphicsDevice fromOpenGLContext(QOpenGLContext *context);
#endif

#if defined(Q_OS_WIN) || defined(Q_QDOC)
    static QQuickGraphicsDevice fromAdapter(quint32 adapterLuidLow, qint32 adapterLuidHigh, int featureLevel = 0);
    static QQuickGraphicsDevice fromDeviceAndContext(void *device, void *context);
#endif

#if QT_CONFIG(metal) || defined(Q_QDOC)
    static QQuickGraphicsDevice fromDeviceAndCommandQueue(MTLDevice *device, MTLCommandQueue *commandQueue);
#endif

#if QT_CONFIG(vulkan) || defined(Q_QDOC)
    static QQuickGraphicsDevice fromPhysicalDevice(VkPhysicalDevice physicalDevice);
    static QQuickGraphicsDevice fromDeviceObjects(VkPhysicalDevice physicalDevice, VkDevice device, int queueFamilyIndex, int queueIndex = 0);
#endif

    static QQuickGraphicsDevice fromRhi(QRhi *rhi);
    static QQuickGraphicsDevice fromRhiAdapter(QRhiAdapter *adapter);

private:
    void detach();
    QQuickGraphicsDevicePrivate *d;
    friend class QQuickGraphicsDevicePrivate;
};

QT_END_NAMESPACE

#endif // QQUICKGRAPHICSDEVICE_H
