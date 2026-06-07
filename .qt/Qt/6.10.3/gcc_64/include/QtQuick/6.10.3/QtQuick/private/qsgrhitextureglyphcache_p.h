// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSGRHITEXTUREGLYPHCACHE_P_H
#define QSGRHITEXTUREGLYPHCACHE_P_H

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

#include <QtGui/private/qtextureglyphcache_p.h>
#include <rhi/qrhi.h>

QT_BEGIN_NAMESPACE

class QSGDefaultRenderContext;

class QSGRhiTextureGlyphCache : public QImageTextureGlyphCache
{
public:
    QSGRhiTextureGlyphCache(QSGDefaultRenderContext *rc,
                            QFontEngine::GlyphFormat format, const QTransform &matrix,
                            const QColor &color = QColor());
    ~QSGRhiTextureGlyphCache();

    void createTextureData(int width, int height) override;
    void resizeTextureData(int width, int height) override;
    void beginFillTexture() override;
    void fillTexture(const Coord &c, glyph_t glyph, const QFixedPoint &subPixelPosition) override;
    void endFillTexture() override;
    int glyphPadding() const override;
    int maxTextureWidth() const override;
    int maxTextureHeight() const override;

    QRhiTexture *texture() const { return m_texture; }
    void commitResourceUpdates(QRhiResourceUpdateBatch *mergeInto);

    // Clamp the default -1 width and height to 0 for compatibility with
    // QOpenGLTextureGlyphCache.
    int width() const { return qMax(0, m_size.width()); }
    int height() const { return qMax(0, m_size.height()); }

    bool eightBitFormatIsAlphaSwizzled() const;

private:
    void prepareGlyphImage(QImage *img);
    QRhiTexture *createEmptyTexture(QRhiTexture::Format format);

    QSGDefaultRenderContext *m_rc;
    QRhi *m_rhi;
    bool m_resizeWithTextureCopy;
    QRhiTexture *m_texture = nullptr;
    QSize m_size;
    bool m_bgra = false;
    QVarLengthArray<QRhiTextureUploadEntry, 16> m_uploads;
};

QT_END_NAMESPACE

#endif // QSGRHITEXTUREGLYPHCACHE_P_H
