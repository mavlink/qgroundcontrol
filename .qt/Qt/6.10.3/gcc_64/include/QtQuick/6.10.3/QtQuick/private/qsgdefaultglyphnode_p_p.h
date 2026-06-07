// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSGDEFAULTGLYPHNODE_P_P_H
#define QSGDEFAULTGLYPHNODE_P_P_H

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

#include <qcolor.h>
#include <QtQuick/qsgmaterial.h>
#include <QtQuick/qsgtexture.h>
#include <QtQuick/qsggeometry.h>
#include <qshareddata.h>
#include <QtQuick/private/qsgplaintexture_p.h>
#include <QtQuick/private/qsgrhitextureglyphcache_p.h>
#include <qrawfont.h>
#include <qmargins.h>

QT_BEGIN_NAMESPACE

class QFontEngine;
class Geometry;
class QSGRenderContext;
class QSGDefaultRenderContext;

class QSGTextMaskMaterial: public QSGMaterial
{
public:
    QSGTextMaskMaterial(QSGRenderContext *rc, const QVector4D &color, const QRawFont &font, QFontEngine::GlyphFormat glyphFormat = QFontEngine::Format_None);
    virtual ~QSGTextMaskMaterial();

    QSGMaterialType *type() const override;
    QSGMaterialShader *createShader(QSGRendererInterface::RenderMode renderMode) const override;
    int compare(const QSGMaterial *other) const override;

    void setColor(const QColor &c) {
        const auto rgbC = c.toRgb();
        setColor(QVector4D(rgbC.redF(), rgbC.greenF(), rgbC.blueF(), rgbC.alphaF()));
    }
    void setColor(const QVector4D &color);
    const QVector4D &color() const { return m_color; }

    QSGTexture *texture() const { return m_texture; }

    bool ensureUpToDate();

    QTextureGlyphCache *glyphCache() const;
    QSGRhiTextureGlyphCache *rhiGlyphCache() const;

    void populate(const QPointF &position,
                  const QVector<quint32> &glyphIndexes, const QVector<QPointF> &glyphPositions,
                  QSGGeometry *geometry, QRectF *boundingRect, QPointF *baseLine,
                  const QMargins &margins = QMargins(0, 0, 0, 0));

private:
    void init(QFontEngine::GlyphFormat glyphFormat);
    void updateCache(QFontEngine::GlyphFormat glyphFormat);

    QSGDefaultRenderContext *m_rc;
    QSGPlainTexture *m_texture;
    QExplicitlySharedDataPointer<QFontEngineGlyphCache> m_glyphCache;
    QRawFont m_font;
    QFontEngine *m_retainedFontEngine = nullptr;
    QRhi *m_rhi;
    QVector4D m_color;
    QSize m_size;
};

class QSGStyledTextMaterial : public QSGTextMaskMaterial
{
public:
    QSGStyledTextMaterial(QSGRenderContext *rc, const QRawFont &font);
    virtual ~QSGStyledTextMaterial() { }

    void setStyleShift(const QVector2D &shift) { m_styleShift = shift; }
    const QVector2D &styleShift() const { return m_styleShift; }

    void setStyleColor(const QColor &c) {
        const auto rgbC = c.toRgb();
        m_styleColor = QVector4D(rgbC.redF(), rgbC.greenF(), rgbC.blueF(), rgbC.alphaF());
    }
    void setStyleColor(const QVector4D &color) { m_styleColor = color; }
    const QVector4D &styleColor() const { return m_styleColor; }

    QSGMaterialType *type() const override;
    QSGMaterialShader *createShader(QSGRendererInterface::RenderMode renderMode) const override;
    int compare(const QSGMaterial *other) const override;

private:
    QVector2D m_styleShift;
    QVector4D m_styleColor;
};

class QSGOutlinedTextMaterial : public QSGStyledTextMaterial
{
public:
    QSGOutlinedTextMaterial(QSGRenderContext *rc, const QRawFont &font);
    ~QSGOutlinedTextMaterial() { }

    QSGMaterialType *type() const override;
    QSGMaterialShader *createShader(QSGRendererInterface::RenderMode renderMode) const override;
};

QT_END_NAMESPACE

#endif
