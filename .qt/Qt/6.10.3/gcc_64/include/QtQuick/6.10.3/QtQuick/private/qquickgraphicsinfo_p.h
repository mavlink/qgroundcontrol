// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKGRAPHICSINFO_P_H
#define QQUICKGRAPHICSINFO_P_H

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

#include <QtCore/qobject.h>
#include <QtCore/qpointer.h>
#include <QtQml/qqml.h>
#include <QtGui/qsurfaceformat.h>
#include <QtQuick/qsgrendererinterface.h>
#include <QtCore/private/qglobal_p.h>

QT_BEGIN_NAMESPACE

class QQuickItem;
class QQuickWindow;

class QQuickGraphicsInfo : public QObject
{
    Q_OBJECT
    Q_PROPERTY(GraphicsApi api READ api NOTIFY apiChanged FINAL)
    Q_PROPERTY(ShaderType shaderType READ shaderType NOTIFY shaderTypeChanged FINAL)
    Q_PROPERTY(ShaderCompilationType shaderCompilationType READ shaderCompilationType NOTIFY shaderCompilationTypeChanged FINAL)
    Q_PROPERTY(ShaderSourceType shaderSourceType READ shaderSourceType NOTIFY shaderSourceTypeChanged FINAL)

    Q_PROPERTY(int majorVersion READ majorVersion NOTIFY majorVersionChanged FINAL)
    Q_PROPERTY(int minorVersion READ minorVersion NOTIFY minorVersionChanged FINAL)
    Q_PROPERTY(OpenGLContextProfile profile READ profile NOTIFY profileChanged FINAL)
    Q_PROPERTY(RenderableType renderableType READ renderableType NOTIFY renderableTypeChanged FINAL)

    QML_NAMED_ELEMENT(GraphicsInfo)
    QML_ADDED_IN_VERSION(2, 8)
    QML_UNCREATABLE("GraphicsInfo is only available via attached properties.")
    QML_ATTACHED(QQuickGraphicsInfo)

public:
    enum GraphicsApi {
        Unknown = QSGRendererInterface::Unknown,
        Software = QSGRendererInterface::Software,
        OpenVG = QSGRendererInterface::OpenVG,
        OpenGL = QSGRendererInterface::OpenGL,
        Direct3D11 = QSGRendererInterface::Direct3D11,
        Vulkan = QSGRendererInterface::Vulkan,
        Metal = QSGRendererInterface::Metal,
        Null = QSGRendererInterface::Null,
        Direct3D12 = QSGRendererInterface::Direct3D12,

        OpenGLRhi = QSGRendererInterface::OpenGLRhi,
        Direct3D11Rhi = QSGRendererInterface::Direct3D11Rhi,
        VulkanRhi = QSGRendererInterface::VulkanRhi,
        MetalRhi = QSGRendererInterface::MetalRhi,
        NullRhi = QSGRendererInterface::NullRhi
    };
    Q_ENUM(GraphicsApi)

    enum ShaderType {
        UnknownShadingLanguage = QSGRendererInterface::UnknownShadingLanguage,
        GLSL = QSGRendererInterface::GLSL,
        HLSL = QSGRendererInterface::HLSL,
        RhiShader = QSGRendererInterface::RhiShader
    };
    Q_ENUM(ShaderType)

    enum ShaderCompilationType {
        RuntimeCompilation = QSGRendererInterface::RuntimeCompilation,
        OfflineCompilation = QSGRendererInterface::OfflineCompilation
    };
    Q_ENUM(ShaderCompilationType)

    enum ShaderSourceType {
        ShaderSourceString = QSGRendererInterface::ShaderSourceString,
        ShaderSourceFile = QSGRendererInterface::ShaderSourceFile,
        ShaderByteCode = QSGRendererInterface::ShaderByteCode
    };
    Q_ENUM(ShaderSourceType)

    enum OpenGLContextProfile {
        OpenGLNoProfile = QSurfaceFormat::NoProfile,
        OpenGLCoreProfile = QSurfaceFormat::CoreProfile,
        OpenGLCompatibilityProfile = QSurfaceFormat::CompatibilityProfile
    };
    Q_ENUM(OpenGLContextProfile)

    enum RenderableType {
        SurfaceFormatUnspecified = QSurfaceFormat::DefaultRenderableType,
        SurfaceFormatOpenGL = QSurfaceFormat::OpenGL,
        SurfaceFormatOpenGLES = QSurfaceFormat::OpenGLES
    };
    Q_ENUM(RenderableType)

    QQuickGraphicsInfo(QQuickItem *item = nullptr);

    static QQuickGraphicsInfo *qmlAttachedProperties(QObject *object);

    GraphicsApi api() const { return m_api; }
    ShaderType shaderType() const { return m_shaderType; }
    ShaderCompilationType shaderCompilationType() const { return m_shaderCompilationType; }
    ShaderSourceType shaderSourceType() const { return m_shaderSourceType; }

    int majorVersion() const { return m_majorVersion; }
    int minorVersion() const { return m_minorVersion; }
    OpenGLContextProfile profile() const { return m_profile; }
    RenderableType renderableType() const { return m_renderableType; }

Q_SIGNALS:
    void apiChanged();
    void shaderTypeChanged();
    void shaderCompilationTypeChanged();
    void shaderSourceTypeChanged();

    void majorVersionChanged();
    void minorVersionChanged();
    void profileChanged();
    void renderableTypeChanged();

private Q_SLOTS:
    void updateInfo();
    void setWindow(QQuickWindow *window);

private:
    QPointer<QQuickWindow> m_window;
    GraphicsApi m_api;
    ShaderType m_shaderType;
    ShaderCompilationType m_shaderCompilationType;
    ShaderSourceType m_shaderSourceType;
    int m_majorVersion;
    int m_minorVersion;
    OpenGLContextProfile m_profile;
    RenderableType m_renderableType;
};

QT_END_NAMESPACE

#endif // QQUICKGRAPHICSINFO_P_H
