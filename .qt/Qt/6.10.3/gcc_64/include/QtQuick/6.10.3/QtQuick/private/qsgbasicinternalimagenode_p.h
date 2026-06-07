// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSGBASICINTERNALIMAGENODE_P_H
#define QSGBASICINTERNALIMAGENODE_P_H

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

QT_BEGIN_NAMESPACE

class Q_QUICK_EXPORT QSGBasicInternalImageNode : public QSGInternalImageNode
{
public:
    QSGBasicInternalImageNode();

    void setTargetRect(const QRectF &rect) override;
    void setInnerTargetRect(const QRectF &rect) override;
    void setInnerSourceRect(const QRectF &rect) override;
    void setSubSourceRect(const QRectF &rect) override;
    void setTexture(QSGTexture *texture) override;
    void setAntialiasing(bool antialiasing) override;
    void setMirror(bool mirrorHorizontally, bool mirrorVertically) override;
    void update() override;
    void preprocess() override;

    static QSGGeometry *updateGeometry(const QRectF &targetRect,
                                       const QRectF &innerTargetRect,
                                       const QRectF &sourceRect,
                                       const QRectF &innerSourceRect,
                                       const QRectF &subSourceRect,
                                       QSGGeometry *geometry,
                                       bool mirrorHorizontally = false,
                                       bool mirrorVertically = false,
                                       bool antialiasing = false);

protected:
    virtual void updateMaterialAntialiasing() = 0;
    virtual void setMaterialTexture(QSGTexture *texture) = 0;
    virtual QSGTexture *materialTexture() const = 0;
    virtual bool updateMaterialBlending() = 0;
    virtual bool supportsWrap(const QSize &size) const = 0;

    void updateGeometry();

    QRectF m_targetRect;
    QRectF m_innerTargetRect;
    QRectF m_innerSourceRect;
    QRectF m_subSourceRect;

    uint m_antialiasing : 1;
    uint m_mirrorHorizontally : 1;
    uint m_mirrorVertically : 1;
    uint m_dirtyGeometry : 1;

    QSGGeometry m_geometry;

    QSGDynamicTexture *m_dynamicTexture;
    QSize m_dynamicTextureSize;
    QRectF m_dynamicTextureSubRect;
};

QT_END_NAMESPACE

#endif
