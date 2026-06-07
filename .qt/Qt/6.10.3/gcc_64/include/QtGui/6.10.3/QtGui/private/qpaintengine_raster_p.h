// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QPAINTENGINE_RASTER_P_H
#define QPAINTENGINE_RASTER_P_H

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
#include "private/qpaintengineex_p.h"
#include "QtGui/qpainterpath.h"
#include "private/qdatabuffer_p.h"
#include "private/qdrawhelper_p.h"
#include "private/qpaintengine_p.h"
#include "private/qrasterizer_p.h"
#include "private/qstroker_p.h"
#include "private/qpainter_p.h"
#include "private/qtextureglyphcache_p.h"
#include "private/qoutlinemapper_p.h"

#include <stdlib.h>

QT_BEGIN_NAMESPACE

class QOutlineMapper;
class QRasterPaintEnginePrivate;
class QRasterBuffer;
class QClipData;

class QRasterPaintEngineState : public QPainterState
{
public:
    QRasterPaintEngineState(const QRasterPaintEngineState &other);
    QRasterPaintEngineState();
    ~QRasterPaintEngineState();


    QPen lastPen;
    QSpanData penData;
    QStrokerOps *stroker;
    uint strokeFlags;

    QBrush lastBrush;
    QSpanData brushData;
    uint fillFlags;

    uint pixmapFlags;
    int intOpacity;

    qreal txscale;

    QClipData *clip;
//     QRect clipRect;
//     QRegion clipRegion;

//     QPainter::RenderHints hints;
//     QPainter::CompositionMode compositionMode;

    uint dirty;

    struct Flags {
        uint has_clip_ownership : 1;        // should delete the clip member..
        uint fast_pen : 1;                  // cosmetic 1-width pens, using midpoint drawlines
        uint non_complex_pen : 1;           // can use rasterizer, rather than stroker
        uint antialiased : 1;
        uint bilinear : 1;
        uint fast_text : 1;
        uint tx_noshear : 1;
        uint fast_images : 1;
        uint cosmetic_brush : 1;
    };

    union {
        Flags flags;
        uint flag_bits;
    };
};




/*******************************************************************************
 * QRasterPaintEngine
 */
class Q_GUI_EXPORT QRasterPaintEngine : public QPaintEngineEx
{
    Q_DECLARE_PRIVATE(QRasterPaintEngine)
public:

    QRasterPaintEngine(QPaintDevice *device);
    ~QRasterPaintEngine();
    bool begin(QPaintDevice *device) override;
    bool end() override;

    void penChanged() override;
    void brushChanged() override;
    void brushOriginChanged() override;
    void opacityChanged() override;
    void compositionModeChanged() override;
    void renderHintsChanged() override;
    void transformChanged() override;
    void clipEnabledChanged() override;

    void setState(QPainterState *s) override;
    QPainterState *createState(QPainterState *orig) const override;
    inline QRasterPaintEngineState *state() {
        return static_cast<QRasterPaintEngineState *>(QPaintEngineEx::state());
    }
    inline const QRasterPaintEngineState *state() const {
        return static_cast<const QRasterPaintEngineState *>(QPaintEngineEx::state());
    }

    void updateBrush(const QBrush &brush);
    void updatePen(const QPen &pen);

    void updateMatrix(const QTransform &matrix);

    virtual void fillPath(const QPainterPath &path, QSpanData *fillData);
    virtual void fillPolygon(const QPointF *points, int pointCount, PolygonDrawMode mode);

    void drawPolygon(const QPointF *points, int pointCount, PolygonDrawMode mode) override;
    void drawPolygon(const QPoint *points, int pointCount, PolygonDrawMode mode) override;

    void drawEllipse(const QRectF &rect) override;

    void fillRect(const QRectF &rect, const QBrush &brush) override;
    void fillRect(const QRectF &rect, const QColor &color) override;

    void drawRects(const QRect  *rects, int rectCount) override;
    void drawRects(const QRectF *rects, int rectCount) override;

    void drawPixmap(const QPointF &p, const QPixmap &pm) override;
    void drawPixmap(const QRectF &r, const QPixmap &pm, const QRectF &sr) override;
    void drawImage(const QPointF &p, const QImage &img) override;
    void drawImage(const QRectF &r, const QImage &pm, const QRectF &sr,
                   Qt::ImageConversionFlags flags = Qt::AutoColor) override;
    void drawTiledPixmap(const QRectF &r, const QPixmap &pm, const QPointF &sr) override;
    void drawTextItem(const QPointF &p, const QTextItem &textItem) override;

    void drawLines(const QLine *line, int lineCount) override;
    void drawLines(const QLineF *line, int lineCount) override;

    void drawPoints(const QPointF *points, int pointCount) override;
    void drawPoints(const QPoint *points, int pointCount) override;

    void stroke(const QVectorPath &path, const QPen &pen) override;
    void fill(const QVectorPath &path, const QBrush &brush) override;

    void clip(const QVectorPath &path, Qt::ClipOperation op) override;
    void clip(const QRect &rect, Qt::ClipOperation op) override;
    void clip(const QRegion &region, Qt::ClipOperation op) override;
    inline const QClipData *clipData() const;

    void drawStaticTextItem(QStaticTextItem *textItem) override;
    virtual bool drawCachedGlyphs(int numGlyphs, const glyph_t *glyphs, const QFixedPoint *positions,
                                  QFontEngine *fontEngine);

    enum ClipType {
        RectClip,
        ComplexClip
    };
    ClipType clipType() const;
    QRectF clipBoundingRect() const;

#ifdef Q_OS_WIN
    void setDC(HDC hdc);
    HDC getDC() const;
    void releaseDC(HDC hdc) const;
    static bool clearTypeFontsEnabled();
#endif

    QRasterBuffer *rasterBuffer();
    void alphaPenBlt(const void* src, int bpl, int depth, int rx,int ry,int w,int h, bool useGammaCorrection);

    Type type() const override { return Raster; }

    QPoint coordinateOffset() const override;

    bool requiresPretransformedGlyphPositions(QFontEngine *fontEngine, const QTransform &m) const override;
    bool shouldDrawCachedGlyphs(QFontEngine *fontEngine, const QTransform &m) const override;

protected:
    QRasterPaintEngine(QRasterPaintEnginePrivate &d, QPaintDevice *);
private:
    friend struct QSpanData;
    friend class QBlitterPaintEngine;
    friend class QBlitterPaintEnginePrivate;
    void init();

    void fillRect(const QRectF &rect, QSpanData *data);
    void drawBitmap(const QPointF &pos, const QImage &image, QSpanData *fill);

    bool setClipRectInDeviceCoords(const QRect &r, Qt::ClipOperation op);

    QRect toNormalizedFillRect(const QRectF &rect);

    inline void ensureBrush(const QBrush &brush) {
        if (!qbrush_fast_equals(state()->lastBrush, brush) || state()->fillFlags)
            updateBrush(brush);
    }
    inline void ensureBrush() { ensureBrush(state()->brush); }

    inline void ensurePen(const QPen &pen) {
        if (!qpen_fast_equals(state()->lastPen, pen) || (pen.style() != Qt::NoPen && state()->strokeFlags))
            updatePen(pen);
    }
    inline void ensurePen() { ensurePen(state()->pen); }

    void updateOutlineMapper();
    inline void ensureOutlineMapper();

    void updateRasterState();
    inline void ensureRasterState() {
        if (state()->dirty)
            updateRasterState();
    }
};


/*******************************************************************************
 * QRasterPaintEnginePrivate
 */
class QRasterPaintEnginePrivate : public QPaintEngineExPrivate
{
    Q_DECLARE_PUBLIC(QRasterPaintEngine)
public:
    QRasterPaintEnginePrivate();

    void rasterizeLine_dashed(QLineF line, qreal width,
                              int *dashIndex, qreal *dashOffset, bool *inDash);
    void rasterize(QT_FT_Outline *outline, ProcessSpans callback, QSpanData *spanData, QRasterBuffer *rasterBuffer);
    void rasterize(QT_FT_Outline *outline, ProcessSpans callback, void *userData, QRasterBuffer *rasterBuffer);
    void updateMatrixData(QSpanData *spanData, const QBrush &brush, const QTransform &brushMatrix);
    void updateClipping();

    void systemStateChanged() override;

    void drawImage(const QPointF &pt, const QImage &img, SrcOverBlendFunc func,
                   const QRect &clip, int alpha, const QRect &sr = QRect());
    void blitImage(const QPointF &pt, const QImage &img,
                   const QRect &clip, const QRect &sr = QRect());

    QTransform brushMatrix() const {
        Q_Q(const QRasterPaintEngine);
        const QRasterPaintEngineState *s = q->state();
        QTransform m(s->matrix);
        m.translate(s->brushOrigin.x(), s->brushOrigin.y());
        return m;
    }

    bool isUnclipped_normalized(const QRect &rect) const;
    bool isUnclipped(const QRect &rect, int penWidth) const;
    bool isUnclipped(const QRectF &rect, int penWidth) const;
    ProcessSpans getPenFunc(const QRectF &rect, const QSpanData *data) const;
    ProcessSpans getBrushFunc(const QRect &rect, const QSpanData *data) const;
    ProcessSpans getBrushFunc(const QRectF &rect, const QSpanData *data) const;

    inline const QClipData *clip() const;

    void initializeRasterizer(QSpanData *data);

    void recalculateFastImages();
    bool canUseFastImageBlending(QPainter::CompositionMode mode, const QImage &image) const;
    bool canUseImageBlitting(QPainter::CompositionMode mode, const QImage &image, const QPointF &pt, const QRectF &sr) const;

    QPaintDevice *device;
    QScopedPointer<QOutlineMapper> outlineMapper;
    QScopedPointer<QRasterBuffer>  rasterBuffer;

#if defined (Q_OS_WIN)
    HDC hdc;
#endif

    QRect deviceRect;
    QRect deviceRectUnclipped;

    QStroker basicStroker;
    QScopedPointer<QDashStroker> dashStroker;

    QScopedPointer<QT_FT_Raster> grayRaster;

    QDataBuffer<QLineF> cachedLines;
    QSpanData image_filler;
    QSpanData image_filler_xform;
    QSpanData solid_color_filler;


    QFontEngine::GlyphFormat glyphCacheFormat;

    QScopedPointer<QClipData> baseClip;

    int deviceDepth;

    uint mono_surface : 1;
    uint outlinemapper_xform_dirty : 1;

    QScopedPointer<QRasterizer> rasterizer;
};


class QClipData {
public:
    QClipData(int height);
    ~QClipData();

    int clipSpanHeight;
    struct ClipLine {
        int count;
        QT_FT_Span *spans;
    } *m_clipLines;

    void initialize();

    inline ClipLine *clipLines() {
        if (!m_clipLines)
            initialize();
        return m_clipLines;
    }

    inline QT_FT_Span *spans() {
        if (!m_spans)
            initialize();
        return m_spans;
    }

    int allocated;
    int count;
    QT_FT_Span *m_spans;
    int xmin, xmax, ymin, ymax;

    QRect clipRect;
    QRegion clipRegion;

    uint enabled : 1;
    uint hasRectClip : 1;
    uint hasRegionClip : 1;

    void appendSpan(int x, int length, int y, int coverage);
    void appendSpans(const QT_FT_Span *s, int num);

    // ### Should optimize and actually kill the QSpans if the rect is
    // ### a subset of The current region. Thus the "fast" clipspan
    // ### callback can be used
    void setClipRect(const QRect &rect);
    void setClipRegion(const QRegion &region);
    void fixup();
};

inline void QClipData::appendSpan(int x, int length, int y, int coverage)
{
    Q_ASSERT(m_spans); // initialize() has to be called prior to adding spans..

    if (count == allocated) {
        allocated *= 2;
        m_spans = static_cast<QT_FT_Span*>(q_check_ptr(realloc(m_spans, allocated * sizeof(QT_FT_Span))));
    }
    m_spans[count].x = x;
    m_spans[count].len = length;
    m_spans[count].y = y;
    m_spans[count].coverage = coverage;
    ++count;
}

inline void QClipData::appendSpans(const QT_FT_Span *s, int num)
{
    Q_ASSERT(m_spans);

    if (count + num > allocated) {
        do {
            allocated *= 2;
        } while (count + num > allocated);
        m_spans = static_cast<QT_FT_Span*>(q_check_ptr(realloc(m_spans, allocated * sizeof(QT_FT_Span))));
    }
    memcpy(m_spans+count, s, num*sizeof(QT_FT_Span));
    count += num;
}

/*******************************************************************************
 * QRasterBuffer
 */
class QRasterBuffer
{
public:
    QRasterBuffer() : m_width(0), m_height(0), m_buffer(nullptr) { init(); }

    ~QRasterBuffer();

    void init();

    QImage::Format prepare(QImage *image);

    uchar *scanLine(int y) { Q_ASSERT(y>=0); Q_ASSERT(y<m_height); return m_buffer + y * bytes_per_line; }

    int width() const { return m_width; }
    int height() const { return m_height; }
    qsizetype bytesPerLine() const { return bytes_per_line; }
    int bytesPerPixel() const { return bytes_per_pixel; }
    template<typename T>
    int stride() { return static_cast<int>(bytes_per_line / sizeof(T)); }

    uchar *buffer() const { return m_buffer; }

    bool monoDestinationWithClut;
    QRgb destColor0;
    QRgb destColor1;

    QPainter::CompositionMode compositionMode;
    QImage::Format format;
    QColorSpace colorSpace;
    QImage colorizeBitmap(const QImage &image, const QColor &color);

private:
    int m_width;
    int m_height;
    qsizetype bytes_per_line;
    int bytes_per_pixel;
    uchar *m_buffer;
};

inline void QRasterPaintEngine::ensureOutlineMapper() {
    if (d_func()->outlinemapper_xform_dirty)
        updateOutlineMapper();
}

inline const QClipData *QRasterPaintEnginePrivate::clip() const {
    Q_Q(const QRasterPaintEngine);
    if (q->state() && q->state()->clip && q->state()->clip->enabled)
        return q->state()->clip;
    return baseClip.data();
}

inline const QClipData *QRasterPaintEngine::clipData() const {
    Q_D(const QRasterPaintEngine);
    if (state() && state()->clip && state()->clip->enabled)
        return state()->clip;
    return d->baseClip.data();
}

QT_END_NAMESPACE
#endif // QPAINTENGINE_RASTER_P_H
