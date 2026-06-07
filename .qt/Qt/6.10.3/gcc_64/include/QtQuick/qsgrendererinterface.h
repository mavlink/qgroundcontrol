// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSGRENDERERINTERFACE_H
#define QSGRENDERERINTERFACE_H

#include <QtQuick/qsgnode.h>

QT_BEGIN_NAMESPACE

class QQuickWindow;

class Q_QUICK_EXPORT QSGRendererInterface
{
public:
    enum GraphicsApi {
        Unknown,
        Software,
        OpenVG,
        OpenGL,
        Direct3D11,
        Vulkan,
        Metal,
        Null,
        Direct3D12,

        OpenGLRhi = OpenGL,
        Direct3D11Rhi = Direct3D11,
        VulkanRhi = Vulkan,
        MetalRhi = Metal,
        NullRhi = Null
    };

    enum Resource {
        DeviceResource,
        CommandQueueResource,
        CommandListResource,
        PainterResource,
        RhiResource,
        RhiSwapchainResource,
        RhiRedirectCommandBuffer,
        RhiRedirectRenderTarget,
        PhysicalDeviceResource,
        OpenGLContextResource,
        DeviceContextResource,
        CommandEncoderResource,
        VulkanInstanceResource,
        RenderPassResource,
        RedirectPaintDevice,
        GraphicsQueueFamilyIndexResource,
        GraphicsQueueIndexResource,
    };

    enum ShaderType {
        UnknownShadingLanguage,
        GLSL,
        HLSL,
        RhiShader
    };

    enum ShaderCompilationType {
        RuntimeCompilation = 0x01,
        OfflineCompilation = 0x02
    };
    Q_DECLARE_FLAGS(ShaderCompilationTypes, ShaderCompilationType)

    enum ShaderSourceType {
        ShaderSourceString = 0x01,
        ShaderSourceFile = 0x02,
        ShaderByteCode = 0x04
    };
    Q_DECLARE_FLAGS(ShaderSourceTypes, ShaderSourceType)

    enum RenderMode {
        RenderMode2D,
        RenderMode2DNoDepthBuffer,
        RenderMode3D
    };

    virtual ~QSGRendererInterface();

    virtual GraphicsApi graphicsApi() const = 0;

    virtual void *getResource(QQuickWindow *window, Resource resource) const;
    virtual void *getResource(QQuickWindow *window, const char *resource) const;

    virtual ShaderType shaderType() const = 0;
    virtual ShaderCompilationTypes shaderCompilationType() const = 0;
    virtual ShaderSourceTypes shaderSourceType() const = 0;

    static bool isApiRhiBased(GraphicsApi api);
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QSGRendererInterface::ShaderCompilationTypes)
Q_DECLARE_OPERATORS_FOR_FLAGS(QSGRendererInterface::ShaderSourceTypes)

QT_END_NAMESPACE

#endif
