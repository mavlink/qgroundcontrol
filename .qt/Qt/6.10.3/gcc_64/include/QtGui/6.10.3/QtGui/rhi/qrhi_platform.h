// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QRHIPLATFORM_H
#define QRHIPLATFORM_H

//
//  W A R N I N G
//  -------------
//
// This file is part of the RHI API, with limited compatibility guarantees.
// Usage of this API may make your code source and binary incompatible with
// future versions of Qt.
//

#include <rhi/qrhi.h>

#if QT_CONFIG(opengl)
#include <QtGui/qsurfaceformat.h>
#endif

#if QT_CONFIG(vulkan)
#include <QtGui/qvulkaninstance.h>
#endif

#if QT_CONFIG(metal) || defined(Q_QDOC)
Q_FORWARD_DECLARE_OBJC_CLASS(MTLDevice);
Q_FORWARD_DECLARE_OBJC_CLASS(MTLCommandQueue);
Q_FORWARD_DECLARE_OBJC_CLASS(MTLCommandBuffer);
Q_FORWARD_DECLARE_OBJC_CLASS(MTLRenderCommandEncoder);
#endif

QT_BEGIN_NAMESPACE

struct Q_GUI_EXPORT QRhiNullInitParams : public QRhiInitParams
{
};

struct Q_GUI_EXPORT QRhiNullNativeHandles : public QRhiNativeHandles
{
};

#if QT_CONFIG(opengl) || defined(Q_QDOC)

class QOpenGLContext;
class QOffscreenSurface;
class QSurface;
class QWindow;

struct Q_GUI_EXPORT QRhiGles2InitParams : public QRhiInitParams
{
    QRhiGles2InitParams();

    QSurfaceFormat format;
    QSurface *fallbackSurface = nullptr;
    QWindow *window = nullptr;
    QOpenGLContext *shareContext = nullptr;

    static QOffscreenSurface *newFallbackSurface(const QSurfaceFormat &format = QSurfaceFormat::defaultFormat());
};

struct Q_GUI_EXPORT QRhiGles2NativeHandles : public QRhiNativeHandles
{
    QOpenGLContext *context = nullptr;
};

#endif // opengl/qdoc

#if (QT_CONFIG(vulkan) && __has_include(<vulkan/vulkan.h>)) || defined(Q_QDOC)

struct Q_GUI_EXPORT QRhiVulkanInitParams : public QRhiInitParams
{
    QVulkanInstance *inst = nullptr;
    QWindow *window = nullptr;
    QByteArrayList deviceExtensions;

    static QByteArrayList preferredInstanceExtensions();
    static QByteArrayList preferredExtensionsForImportedDevice();
};

struct Q_GUI_EXPORT QRhiVulkanNativeHandles : public QRhiNativeHandles
{
    // to import a physical device (always required)
    VkPhysicalDevice physDev = VK_NULL_HANDLE;
    // to import a device and queue
    VkDevice dev = VK_NULL_HANDLE;
    quint32 gfxQueueFamilyIdx = 0;
    quint32 gfxQueueIdx = 0;
    // and optionally, the mem allocator
    void *vmemAllocator = nullptr;

    // only for querying (rhi->nativeHandles())
    VkQueue gfxQueue = VK_NULL_HANDLE;
    QVulkanInstance *inst = nullptr;
};

struct Q_GUI_EXPORT QRhiVulkanCommandBufferNativeHandles : public QRhiNativeHandles
{
    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
};

struct Q_GUI_EXPORT QRhiVulkanRenderPassNativeHandles : public QRhiNativeHandles
{
    VkRenderPass renderPass = VK_NULL_HANDLE;
};

struct Q_GUI_EXPORT QRhiVulkanQueueSubmitParams : public QRhiNativeHandles
{
    uint32_t waitSemaphoreCount;
    VkSemaphore *waitSemaphores;
    uint32_t signalSemaphoreCount;
    VkSemaphore *signalSemaphores;
    uint32_t presentWaitSemaphoreCount;
    VkSemaphore *presentWaitSemaphores;
};

#endif // vulkan/qdoc

#if defined(Q_OS_WIN) || defined(Q_QDOC)

// no d3d includes here, to prevent precompiled header mess due to COM, hence the void pointers

struct Q_GUI_EXPORT QRhiD3D11InitParams : public QRhiInitParams
{
    bool enableDebugLayer = false;
};

struct Q_GUI_EXPORT QRhiD3D11NativeHandles : public QRhiNativeHandles
{
    // to import a device and a context
    void *dev = nullptr;
    void *context = nullptr;
    // alternatively, to specify the device feature level and/or the adapter to use
    int featureLevel = 0;
    quint32 adapterLuidLow = 0;
    qint32 adapterLuidHigh = 0;
};

struct Q_GUI_EXPORT QRhiD3D12InitParams : public QRhiInitParams
{
    bool enableDebugLayer = false;
};

struct Q_GUI_EXPORT QRhiD3D12NativeHandles : public QRhiNativeHandles
{
    // to import a device
    void *dev = nullptr;
    int minimumFeatureLevel = 0;
    // to just specify the adapter to use, set these and leave dev set to null
    quint32 adapterLuidLow = 0;
    qint32 adapterLuidHigh = 0;
    // in addition, can specify the command queue to use
    void *commandQueue = nullptr;
};

struct Q_GUI_EXPORT QRhiD3D12CommandBufferNativeHandles : public QRhiNativeHandles
{
    void *commandList = nullptr; // ID3D12GraphicsCommandList1
};

#endif // WIN/QDOC

#if QT_CONFIG(metal) || defined(Q_QDOC)

struct Q_GUI_EXPORT QRhiMetalInitParams : public QRhiInitParams
{
};

struct Q_GUI_EXPORT QRhiMetalNativeHandles : public QRhiNativeHandles
{
    MTLDevice *dev = nullptr;
    MTLCommandQueue *cmdQueue = nullptr;
};

struct Q_GUI_EXPORT QRhiMetalCommandBufferNativeHandles : public QRhiNativeHandles
{
    MTLCommandBuffer *commandBuffer = nullptr;
    MTLRenderCommandEncoder *encoder = nullptr;
};

#endif // MACOS/IOS/QDOC

QT_END_NAMESPACE

#endif
