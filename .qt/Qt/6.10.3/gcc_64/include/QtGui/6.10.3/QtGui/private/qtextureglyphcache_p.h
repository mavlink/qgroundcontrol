// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QTEXTUREGLYPHCACHE_P_H
#define QTEXTUREGLYPHCACHE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of other Qt classes.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtGui/private/qtguiglobal_p.h>
#include <qhash.h>
#include <qimage.h>
#include <qobject.h>
#include <qtransform.h>

#include <private/qfontengineglyphcache_p.h>

#ifndef QT_DEFAULT_TEXTURE_GLYPH_CACHE_WIDTH
#define QT_DEFAULT_TEXTURE_GLYPH_CACHE_WIDTH 256
#endif

struct glyph_metrics_t;
typedef unsigned int glyph_t;


QT_BEGIN_NAMESPACE

class QTextItemInt;

class Q_GUI_EXPORT QTextureGlyphCache : public QFontEngineGlyphCache
{
public:
    QTextureGlyphCache(QFontEngine::GlyphFormat format, const QTransform &matrix, const QColor &color = QColor())
        : QFontEngineGlyphCache(format, matrix, color), m_current_fontengine(nullptr),
                                               m_w(0), m_h(0), m_cx(0), m_cy(0), m_currentRowHeight(0)
        { }

    ~QTextureGlyphCache();

    struct GlyphAndSubPixelPosition
    {
        GlyphAndSubPixelPosition(glyph_t g, const QFixedPoint &spp)
            : glyph(g), subPixelPosition(spp) {}

        bool operator==(const GlyphAndSubPixelPosition &other) const
        {
            return glyph == other.glyph && subPixelPosition == other.subPixelPosition;
        }

        glyph_t glyph;
        QFixedPoint subPixelPosition;
    };

    struct Coord {
        int x;
        int y;
        int w;
        int h;

        int baseLineX;
        int baseLineY;

        bool isNull() const
        {
            return w == 0 || h == 0;
        }
    };

    bool populate(QFontEngine *fontEngine,
                  qsizetype numGlyphs,
                  const glyph_t *glyphs,
                  const QFixedPoint *positions,
                  QPainter::RenderHints renderHints = QPainter::RenderHints(),
                  bool includeGlyphCacheScale = false);
    bool hasPendingGlyphs() const { return !m_pendingGlyphs.isEmpty(); }
    void fillInPendingGlyphs();

    virtual void createTextureData(int width, int height) = 0;
    virtual void resizeTextureData(int width, int height) = 0;
    virtual int glyphPadding() const { return 0; }

    virtual void beginFillTexture() { }
    virtual void fillTexture(const Coord &coord,
                             glyph_t glyph,
                             const QFixedPoint &subPixelPosition) = 0;
    virtual void endFillTexture() { }

    inline void createCache(int width, int height) {
        m_w = width;
        m_h = height;
        createTextureData(width, height);
    }

    inline void resizeCache(int width, int height)
    {
        resizeTextureData(width, height);
        m_w = width;
        m_h = height;
    }

    inline bool isNull() const { return m_h == 0; }

    QHash<GlyphAndSubPixelPosition, Coord> coords;
    virtual int maxTextureWidth() const { return QT_DEFAULT_TEXTURE_GLYPH_CACHE_WIDTH; }
    virtual int maxTextureHeight() const { return -1; }

    QImage textureMapForGlyph(glyph_t g, const QFixedPoint &subPixelPosition) const;

protected:
    int calculateSubPixelPositionCount(glyph_t) const;

    QFontEngine *m_current_fontengine;
    QHash<GlyphAndSubPixelPosition, Coord> m_pendingGlyphs;

    int m_w; // image width
    int m_h; // image height
    int m_cx; // current x
    int m_cy; // current y
    int m_currentRowHeight; // Height of last row
};

inline size_t qHash(const QTextureGlyphCache::GlyphAndSubPixelPosition &g, size_t seed = 0)
{
    return qHashMulti(seed,
                      g.glyph,
                      g.subPixelPosition.x.value(),
                      g.subPixelPosition.y.value());
}


class Q_GUI_EXPORT QImageTextureGlyphCache : public QTextureGlyphCache
{
public:
    QImageTextureGlyphCache(QFontEngine::GlyphFormat format, const QTransform &matrix, const QColor &color = QColor())
        : QTextureGlyphCache(format, matrix, color) { }
    ~QImageTextureGlyphCache();

    virtual void createTextureData(int width, int height) override;
    virtual void resizeTextureData(int width, int height) override;
    virtual void fillTexture(const Coord &c,
                             glyph_t glyph,
                             const QFixedPoint &subPixelPosition) override;

    inline const QImage &image() const { return m_image; }

private:
    QImage m_image;
};

QT_END_NAMESPACE

#endif
