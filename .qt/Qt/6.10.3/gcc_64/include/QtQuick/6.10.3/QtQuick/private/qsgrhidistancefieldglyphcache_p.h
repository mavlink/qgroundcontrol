// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSGRHIDISTANCEFIELDGLYPHCACHE_H
#define QSGRHIDISTANCEFIELDGLYPHCACHE_H

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

#include "qsgadaptationlayer_p.h"
#include <private/qsgareaallocator_p.h>
#include <rhi/qrhi.h>

QT_BEGIN_NAMESPACE

class QSGDefaultRenderContext;

class Q_QUICK_EXPORT QSGRhiDistanceFieldGlyphCache : public QSGDistanceFieldGlyphCache
{
public:
    QSGRhiDistanceFieldGlyphCache(QSGDefaultRenderContext *rc, const QRawFont &font, int renderTypeQuality);
    virtual ~QSGRhiDistanceFieldGlyphCache();

    void requestGlyphs(const QSet<glyph_t> &glyphs) override;
    void storeGlyphs(const QList<QDistanceField> &glyphs) override;
    void referenceGlyphs(const QSet<glyph_t> &glyphs) override;
    void releaseGlyphs(const QSet<glyph_t> &glyphs) override;

    bool useTextureResizeWorkaround() const;
    bool createFullSizeTextures() const;
    bool isActive() const override;
    int maxTextureSize() const;

    void setMaxTextureCount(int max) { m_maxTextureCount = max; }
    int maxTextureCount() const { return m_maxTextureCount; }

    void commitResourceUpdates(QRhiResourceUpdateBatch *mergeInto);

    bool eightBitFormatIsAlphaSwizzled() const override;
    bool screenSpaceDerivativesSupported() const override;

#if defined(QSG_DISTANCEFIELD_CACHE_DEBUG)
    void saveTexture(QRhiTexture *texture, const QString &nameBase) const override;
#endif

private:
    bool loadPregeneratedCache(const QRawFont &font);

    struct TextureInfo {
        QRhiTexture *texture;
        QSize size;
        QRect allocatedArea;
        QDistanceField image;
        int padding = -1;
        QVarLengthArray<QRhiTextureUploadEntry, 16> uploads;

        TextureInfo(const QRect &preallocRect = QRect()) : texture(nullptr), allocatedArea(preallocRect) { }
    };

    void createTexture(TextureInfo *texInfo, int width, int height, const void *pixels);
    void createTexture(TextureInfo *texInfo, int width, int height);
    void resizeTexture(TextureInfo *texInfo, int width, int height);

    TextureInfo *textureInfo(int index)
    {
        for (int i = m_textures.size(); i <= index; ++i) {
            if (createFullSizeTextures())
                m_textures.append(QRect(0, 0, maxTextureSize(), maxTextureSize()));
            else
                m_textures.append(TextureInfo());
        }

        return &m_textures[index];
    }

    QSGDefaultRenderContext *m_rc;
    QRhi *m_rhi;
    mutable int m_maxTextureSize = 0;
    int m_maxTextureCount = 3;
    QSGAreaAllocator *m_areaAllocator = nullptr;
    QList<TextureInfo> m_textures;
    QHash<glyph_t, TextureInfo *> m_glyphsTexture;
    QSet<glyph_t> m_unusedGlyphs;
    QSet<glyph_t> m_referencedGlyphs;
    QSet<QRhiTexture *> m_pendingDispose;
};

QT_END_NAMESPACE

#endif // QSGRHIDISTANCEFIELDGLYPHCACHE_H
