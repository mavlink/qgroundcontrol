// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSGDEFAULTIMAGENODE_P_H
#define QSGDEFAULTIMAGENODE_P_H

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

#include <QtQuick/private/qtquickglobal_p.h>
#include <QtQuick/qsgimagenode.h>
#include <QtQuick/qsggeometry.h>
#include <QtQuick/qsgtexturematerial.h>

QT_BEGIN_NAMESPACE

class Q_QUICK_EXPORT QSGDefaultImageNode : public QSGImageNode
{
public:
    QSGDefaultImageNode();
    ~QSGDefaultImageNode();

    void setRect(const QRectF &rect) override;
    QRectF rect() const override;

    void setSourceRect(const QRectF &r) override;
    QRectF sourceRect() const override;

    void setTexture(QSGTexture *texture) override;
    QSGTexture *texture() const override;

    void setFiltering(QSGTexture::Filtering filtering) override;
    QSGTexture::Filtering filtering() const override;

    void setMipmapFiltering(QSGTexture::Filtering filtering) override;
    QSGTexture::Filtering mipmapFiltering() const override;

    void setAnisotropyLevel(QSGTexture::AnisotropyLevel level) override;
    QSGTexture::AnisotropyLevel anisotropyLevel() const override;

    void setTextureCoordinatesTransform(TextureCoordinatesTransformMode mode) override;
    TextureCoordinatesTransformMode textureCoordinatesTransform() const override;

    void setOwnsTexture(bool owns) override;
    bool ownsTexture() const override;

private:
    QSGGeometry m_geometry;
    QSGOpaqueTextureMaterial m_opaque_material;
    QSGTextureMaterial m_material;
    QRectF m_rect;
    QRectF m_sourceRect;
    QSize m_textureSize;
    TextureCoordinatesTransformMode m_texCoordMode;
    uint m_isAtlasTexture : 1;
    uint m_ownsTexture : 1;
};

QT_END_NAMESPACE

#endif // QSGDEFAULTIMAGENODE_P_H
