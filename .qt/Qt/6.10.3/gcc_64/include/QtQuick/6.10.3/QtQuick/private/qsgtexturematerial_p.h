// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef TEXTUREMATERIAL_P_H
#define TEXTUREMATERIAL_P_H

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

#include "qsgtexturematerial.h"
#include <private/qtquickglobal_p.h>

QT_BEGIN_NAMESPACE

class Q_QUICK_EXPORT QSGOpaqueTextureMaterialRhiShader : public QSGMaterialShader
{
public:
    QSGOpaqueTextureMaterialRhiShader(int viewCount);

    bool updateUniformData(RenderState &state, QSGMaterial *newMaterial, QSGMaterial *oldMaterial) override;
    void updateSampledImage(RenderState &state, int binding, QSGTexture **texture, QSGMaterial *newMaterial, QSGMaterial *oldMaterial) override;
};

class QSGTextureMaterialRhiShader : public QSGOpaqueTextureMaterialRhiShader
{
public:
    QSGTextureMaterialRhiShader(int viewCount);

    bool updateUniformData(RenderState &state, QSGMaterial *newMaterial, QSGMaterial *oldMaterial) override;
};

QT_END_NAMESPACE

#endif // QSGTEXTUREMATERIAL_P_H
