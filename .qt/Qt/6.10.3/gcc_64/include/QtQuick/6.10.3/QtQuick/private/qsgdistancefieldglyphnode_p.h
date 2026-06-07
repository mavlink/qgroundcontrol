// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSGDISTANCEFIELDGLYPHNODE_P_H
#define QSGDISTANCEFIELDGLYPHNODE_P_H

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
#include <QtQuick/qsgtexture.h>

#include <QtQuick/private/qquicktext_p.h>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(lcSgText)

class QSGRenderContext;
class QSGDistanceFieldTextMaterial;

class QSGDistanceFieldGlyphNode : public QSGGlyphNode, public QSGDistanceFieldGlyphConsumer
{
public:
    QSGDistanceFieldGlyphNode(QSGRenderContext *context);
    ~QSGDistanceFieldGlyphNode();

    QPointF baseLine() const override { return m_baseLine; }
    void setGlyphs(const QPointF &position, const QGlyphRun &glyphs) override;
    void setColor(const QColor &color) override;

    void setPreferredAntialiasingMode(AntialiasingMode mode) override;
    void setRenderTypeQuality(int renderTypeQuality) override;

    void setStyle(QQuickText::TextStyle style) override;
    void setStyleColor(const QColor &color) override;

    void update() override;
    void preprocess() override;

    void invalidateGlyphs(const QVector<quint32> &glyphs) override;

    void updateGeometry();

private:
    enum DistanceFieldGlyphNodeType {
        RootGlyphNode,
        SubGlyphNode
    };

    void setGlyphNodeType(DistanceFieldGlyphNodeType type) { m_glyphNodeType = type; }
    void updateMaterial();

    DistanceFieldGlyphNodeType m_glyphNodeType;
    QColor m_color;
    QPointF m_baseLine;
    QSGRenderContext *m_context;
    QSGDistanceFieldTextMaterial *m_material;
    QPointF m_originalPosition;
    QPointF m_position;
    QGlyphRun m_glyphs;
    QSGDistanceFieldGlyphCache *m_glyph_cache;
    QSGGeometry m_geometry;
    QQuickText::TextStyle m_style;
    QColor m_styleColor;
    AntialiasingMode m_antialiasingMode;
    QRectF m_boundingRect;
    const QSGDistanceFieldGlyphCache::Texture *m_texture;
    int m_renderTypeQuality;

    struct GlyphInfo {
        QVector<quint32> indexes;
        QVector<QPointF> positions;
    };
    QSet<quint32> m_allGlyphIndexesLookup;
    // m_glyphs holds pointers to the GlyphInfo.indexes and positions arrays, so we need to hold on to them
    QHash<const QSGDistanceFieldGlyphCache::Texture *, GlyphInfo> m_glyphsInOtherTextures;

    uint m_dirtyGeometry: 1;
    uint m_dirtyMaterial: 1;
};

QT_END_NAMESPACE

#endif
