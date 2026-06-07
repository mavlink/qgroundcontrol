// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSGMATERIALSHADER_H
#define QSGMATERIALSHADER_H

#include <QtQuick/qtquickglobal.h>
#include <QtCore/QRect>
#include <QtGui/QMatrix4x4>
#include <QtGui/QColor>
#include <QtQuick/qsgmaterialtype.h>

QT_BEGIN_NAMESPACE

class QSGMaterial;
class QSGMaterialShaderPrivate;
class QSGTexture;
class QRhiResourceUpdateBatch;
class QRhi;
class QShader;

class Q_QUICK_EXPORT QSGMaterialShader
{
public:
    class Q_QUICK_EXPORT RenderState {
    public:
        enum DirtyState
        {
            DirtyMatrix             = 0x0001,
            DirtyOpacity            = 0x0002,
            DirtyCachedMaterialData = 0x0004,
            DirtyAll                = 0xFFFF
        };
        Q_DECLARE_FLAGS(DirtyStates, DirtyState)

        inline DirtyStates dirtyStates() const { return m_dirty; }

        inline bool isMatrixDirty() const { return bool(m_dirty & QSGMaterialShader::RenderState::DirtyMatrix); }
        inline bool isOpacityDirty() const { return bool(m_dirty & QSGMaterialShader::RenderState::DirtyOpacity); }

        float opacity() const;
        QMatrix4x4 combinedMatrix() const;
        QMatrix4x4 combinedMatrix(qsizetype index) const;
        QMatrix4x4 modelViewMatrix() const;
        QMatrix4x4 projectionMatrix() const;
        QMatrix4x4 projectionMatrix(qsizetype index) const;
        qsizetype projectionMatrixCount() const;
        QRect viewportRect() const;
        QRect deviceRect() const;
        float determinant() const;
        float devicePixelRatio() const;

        QByteArray *uniformData();
        QRhiResourceUpdateBatch *resourceUpdateBatch();
        QRhi *rhi();

    private:
        friend class QSGRenderer;
        DirtyStates m_dirty;
        const void *m_data;
    };

    struct GraphicsPipelineState {
        enum BlendFactor {
            Zero,
            One,
            SrcColor,
            OneMinusSrcColor,
            DstColor,
            OneMinusDstColor,
            SrcAlpha,
            OneMinusSrcAlpha,
            DstAlpha,
            OneMinusDstAlpha,
            ConstantColor,
            OneMinusConstantColor,
            ConstantAlpha,
            OneMinusConstantAlpha,
            SrcAlphaSaturate,
            Src1Color,
            OneMinusSrc1Color,
            Src1Alpha,
            OneMinusSrc1Alpha
        };

        enum class BlendOp {
            Add,
            Subtract,
            ReverseSubtract,
            Min,
            Max,
        };

        enum ColorMaskComponent {
            R = 1 << 0,
            G = 1 << 1,
            B = 1 << 2,
            A = 1 << 3
        };
        Q_DECLARE_FLAGS(ColorMask, ColorMaskComponent)

        enum CullMode {
            CullNone,
            CullFront,
            CullBack
        };

        enum PolygonMode {
            Fill,
            Line,
        };

        bool blendEnable;
        BlendFactor srcColor;
        BlendFactor dstColor;
        ColorMask colorWrite;
        QColor blendConstant;
        CullMode cullMode;
        PolygonMode polygonMode;
        bool separateBlendFactors;
        BlendFactor srcAlpha;
        BlendFactor dstAlpha;
        BlendOp opColor;
        BlendOp opAlpha;

        // This struct is extensible while keeping BC since apps only ever get
        // a ptr to the struct, it is not created by them.
    };

    enum Flag {
        UpdatesGraphicsPipelineState = 0x0001
    };
    Q_DECLARE_FLAGS(Flags, Flag)

    enum Stage {
        VertexStage,
        FragmentStage,
    };

    QSGMaterialShader();
    virtual ~QSGMaterialShader();

    virtual bool updateUniformData(RenderState &state,
                                   QSGMaterial *newMaterial, QSGMaterial *oldMaterial);

    virtual void updateSampledImage(RenderState &state, int binding, QSGTexture **texture,
                                    QSGMaterial *newMaterial, QSGMaterial *oldMaterial);

    virtual bool updateGraphicsPipelineState(RenderState &state, GraphicsPipelineState *ps,
                                             QSGMaterial *newMaterial, QSGMaterial *oldMaterial);

    Flags flags() const;
    void setFlag(Flags flags, bool on = true);
    void setFlags(Flags flags);

    int combinedImageSamplerCount(int binding) const;

protected:
    Q_DECLARE_PRIVATE(QSGMaterialShader)
    QSGMaterialShader(QSGMaterialShaderPrivate &dd);

    // filename is for a file containing a serialized QShader.
    void setShaderFileName(Stage stage, const QString &filename);
    void setShaderFileName(Stage stage, const QString &filename, int viewCount);

    void setShader(Stage stage, const QShader &shader);

private:
    QScopedPointer<QSGMaterialShaderPrivate> d_ptr;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QSGMaterialShader::RenderState::DirtyStates)
Q_DECLARE_OPERATORS_FOR_FLAGS(QSGMaterialShader::GraphicsPipelineState::ColorMask)
Q_DECLARE_OPERATORS_FOR_FLAGS(QSGMaterialShader::Flags)

QT_END_NAMESPACE

#endif
