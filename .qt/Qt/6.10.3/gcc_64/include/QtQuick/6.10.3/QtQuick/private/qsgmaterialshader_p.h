// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSGMATERIALSHADER_P_H
#define QSGMATERIALSHADER_P_H

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

#include <private/qtquickglobal_p.h>
#include "qsgmaterialshader.h"
#include "qsgmaterial.h"
#include <rhi/qrhi.h>
#include <rhi/qshader.h>

QT_BEGIN_NAMESPACE

class QRhiSampler;

class Q_QUICK_EXPORT QSGMaterialShaderPrivate
{
public:
    Q_DECLARE_PUBLIC(QSGMaterialShader)

    QSGMaterialShaderPrivate(QSGMaterialShader *q) : q_ptr(q) { }
    static QSGMaterialShaderPrivate *get(QSGMaterialShader *s) { return s->d_func(); }
    static const QSGMaterialShaderPrivate *get(const QSGMaterialShader *s) { return s->d_func(); }

    void clearCachedRendererData();
    void prepare(QShader::Variant vertexShaderVariant);

    QShader shader(QShader::Stage stage) const { return shaders[stage].shader; }

    static QShader loadShader(const QString &filename);

    QSGMaterialShader *q_ptr;
    QHash<QShader::Stage, QString> shaderFileNames;
    QSGMaterialShader::Flags flags;

    struct ShaderStageData {
        ShaderStageData() { } // so shader.isValid() == false
        ShaderStageData(const QShader &shader) : shader(shader) { }
        QShader shader;
        QShader::Variant shaderVariant = QShader::StandardShader;
        QVector<int> vertexInputLocations; // excluding rewriter-inserted ones
        int qt_order_attrib_location = -1; // rewriter-inserted
    };
    QHash<QShader::Stage, ShaderStageData> shaders;

    static const int MAX_SHADER_RESOURCE_BINDINGS = 32;

    int ubufBinding = -1;
    int ubufSize = 0;
    QRhiShaderResourceBinding::StageFlags ubufStages;
    QRhiShaderResourceBinding::StageFlags combinedImageSamplerBindings[MAX_SHADER_RESOURCE_BINDINGS];
    int combinedImageSamplerCount[MAX_SHADER_RESOURCE_BINDINGS];

    ShaderStageData *vertexShader = nullptr;
    ShaderStageData *fragmentShader = nullptr;

    QByteArray masterUniformData;

    QVarLengthArray<QSGTexture *, 4> textureBindingTable[MAX_SHADER_RESOURCE_BINDINGS];
    QVarLengthArray<QRhiSampler *, 4> samplerBindingTable[MAX_SHADER_RESOURCE_BINDINGS];
};

Q_DECLARE_TYPEINFO(QSGMaterialShaderPrivate::ShaderStageData, Q_RELOCATABLE_TYPE);

QT_END_NAMESPACE

#endif
