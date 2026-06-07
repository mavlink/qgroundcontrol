// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSGDISTANCEFIELDGLYPHNODE_P_P_H
#define QSGDISTANCEFIELDGLYPHNODE_P_P_H

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

#include <QtQuick/qsgmaterial.h>
#include <QtQuick/private/qsgplaintexture_p.h>
#include "qsgdistancefieldglyphnode_p.h"
#include "qsgadaptationlayer_p.h"

QT_BEGIN_NAMESPACE

class QSGPlainTexture;

class Q_QUICK_EXPORT QSGDistanceFieldTextMaterial: public QSGMaterial
{
public:
    QSGDistanceFieldTextMaterial();
    ~QSGDistanceFieldTextMaterial();

    QSGMaterialType *type() const override;
    QSGMaterialShader *createShader(QSGRendererInterface::RenderMode renderMode) const override;
    int compare(const QSGMaterial *other) const override;

    virtual void setColor(const QColor &color);
    const QVector4D &color() const { return m_color; }

    void setGlyphCache(QSGDistanceFieldGlyphCache *a) { m_glyph_cache = a; }
    QSGDistanceFieldGlyphCache *glyphCache() const { return m_glyph_cache; }

    void setTexture(const QSGDistanceFieldGlyphCache::Texture * tex) { m_texture = tex; }
    const QSGDistanceFieldGlyphCache::Texture * texture() const { return m_texture; }

    void setFontScale(qreal fontScale) { m_fontScale = fontScale; }
    qreal fontScale() const { return m_fontScale; }

    QSize textureSize() const { return m_size; }

    bool updateTextureSize();
    bool updateTextureSizeAndWrapper();
    QSGTexture *wrapperTexture() const { return m_sgTexture; }

protected:
    QSize m_size;
    QVector4D m_color;
    QSGDistanceFieldGlyphCache *m_glyph_cache;
    const QSGDistanceFieldGlyphCache::Texture *m_texture;
    qreal m_fontScale;
    QSGPlainTexture *m_sgTexture;
};

class Q_QUICK_EXPORT QSGDistanceFieldStyledTextMaterial : public QSGDistanceFieldTextMaterial
{
public:
    QSGDistanceFieldStyledTextMaterial();
    ~QSGDistanceFieldStyledTextMaterial();

    QSGMaterialType *type() const override = 0;
    QSGMaterialShader *createShader(QSGRendererInterface::RenderMode renderMode) const override = 0;
    int compare(const QSGMaterial *other) const override;

    void setStyleColor(const QColor &color);
    const QVector4D &styleColor() const { return m_styleColor; }

protected:
    QVector4D m_styleColor;
};

class Q_QUICK_EXPORT QSGDistanceFieldOutlineTextMaterial : public QSGDistanceFieldStyledTextMaterial
{
public:
    QSGDistanceFieldOutlineTextMaterial();
    ~QSGDistanceFieldOutlineTextMaterial();

    QSGMaterialType *type() const override;
    QSGMaterialShader *createShader(QSGRendererInterface::RenderMode renderMode) const override;
};

class Q_QUICK_EXPORT QSGDistanceFieldShiftedStyleTextMaterial : public QSGDistanceFieldStyledTextMaterial
{
public:
    QSGDistanceFieldShiftedStyleTextMaterial();
    ~QSGDistanceFieldShiftedStyleTextMaterial();

    QSGMaterialType *type() const override;
    QSGMaterialShader *createShader(QSGRendererInterface::RenderMode renderMode) const override;
    int compare(const QSGMaterial *other) const override;

    void setShift(const QPointF &shift) { m_shift = shift; }
    const QPointF &shift() const { return m_shift; }

protected:
    QPointF m_shift;
};

class Q_QUICK_EXPORT QSGHiQSubPixelDistanceFieldTextMaterial : public QSGDistanceFieldTextMaterial
{
public:
    QSGMaterialType *type() const override;
    QSGMaterialShader *createShader(QSGRendererInterface::RenderMode renderMode) const override;
    void setColor(const QColor &color) override {
        const auto rgbColor = color.toRgb();
        m_color = QVector4D(rgbColor.redF(), rgbColor.greenF(), rgbColor.blueF(), rgbColor.alphaF());
    }
};

class Q_QUICK_EXPORT QSGLoQSubPixelDistanceFieldTextMaterial : public QSGDistanceFieldTextMaterial
{
public:
    QSGMaterialType *type() const override;
    QSGMaterialShader *createShader(QSGRendererInterface::RenderMode renderMode) const override;
    void setColor(const QColor &color) override {
        const auto rgbColor = color.toRgb();
        m_color = QVector4D(rgbColor.redF(), rgbColor.greenF(), rgbColor.blueF(), rgbColor.alphaF());
    }
};

QT_END_NAMESPACE

#endif
