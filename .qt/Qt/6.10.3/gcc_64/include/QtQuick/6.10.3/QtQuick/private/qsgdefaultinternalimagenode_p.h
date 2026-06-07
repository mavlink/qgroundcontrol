// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only


#ifndef QSGDEFAULTINTERNALIMAGENODE_P_H
#define QSGDEFAULTINTERNALIMAGENODE_P_H

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

#include <private/qsgadaptationlayer_p.h>
#include <private/qsgbasicinternalimagenode_p.h>
#include <QtQuick/qsgtexturematerial.h>

QT_BEGIN_NAMESPACE

class QSGDefaultRenderContext;

class Q_QUICK_EXPORT QSGSmoothTextureMaterial : public QSGTextureMaterial
{
public:
    QSGSmoothTextureMaterial();

    void setTexture(QSGTexture *texture);

protected:
    QSGMaterialType *type() const override;
    QSGMaterialShader *createShader(QSGRendererInterface::RenderMode renderMode) const override;
};

class Q_QUICK_EXPORT QSGDefaultInternalImageNode : public QSGBasicInternalImageNode
{
public:
    QSGDefaultInternalImageNode(QSGDefaultRenderContext *rc);

    void setMipmapFiltering(QSGTexture::Filtering filtering) override;
    void setFiltering(QSGTexture::Filtering filtering) override;
    void setHorizontalWrapMode(QSGTexture::WrapMode wrapMode) override;
    void setVerticalWrapMode(QSGTexture::WrapMode wrapMode) override;

    void updateMaterialAntialiasing() override;
    void setMaterialTexture(QSGTexture *texture) override;
    QSGTexture *materialTexture() const override;
    bool updateMaterialBlending() override;
    bool supportsWrap(const QSize &size) const override;

private:
    QSGDefaultRenderContext *m_rc;
    QSGOpaqueTextureMaterial m_material;
    QSGTextureMaterial m_materialO;
    QSGSmoothTextureMaterial m_smoothMaterial;
};

QT_END_NAMESPACE

#endif
