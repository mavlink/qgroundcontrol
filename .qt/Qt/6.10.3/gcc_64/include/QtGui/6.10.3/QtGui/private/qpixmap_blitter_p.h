// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QPIXMAP_BLITTER_P_H
#define QPIXMAP_BLITTER_P_H

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

#include <QtGui/private/qtguiglobal_p.h>
#include <qpa/qplatformpixmap.h>
#include <private/qpaintengine_blitter_p.h>

#ifndef QT_NO_BLITTABLE
QT_BEGIN_NAMESPACE

class Q_GUI_EXPORT  QBlittablePlatformPixmap : public QPlatformPixmap
{
//     Q_DECLARE_PRIVATE(QBlittablePlatformPixmap)
public:
    QBlittablePlatformPixmap();
    ~QBlittablePlatformPixmap();

    virtual QBlittable *createBlittable(const QSize &size, bool alpha) const = 0;
    QBlittable *blittable() const;
    void setBlittable(QBlittable *blittable);

    void resize(int width, int height) override;
    int metric(QPaintDevice::PaintDeviceMetric metric) const override;
    void fill(const QColor &color) override;
    QImage *buffer() override;
    QImage toImage() const override;
    bool hasAlphaChannel() const override;
    void fromImage(const QImage &image, Qt::ImageConversionFlags flags) override;
    qreal devicePixelRatio() const override;
    void setDevicePixelRatio(qreal scaleFactor) override;

    QPaintEngine *paintEngine() const override;

    void markRasterOverlay(const QRectF &);
    void markRasterOverlay(const QPointF &, const QTextItem &);
    void markRasterOverlay(const QVectorPath &);
    void markRasterOverlay(const QPainterPath &);
    void markRasterOverlay(const QRect *rects, int rectCount);
    void markRasterOverlay(const QRectF *rects, int rectCount);
    void markRasterOverlay(const QPointF *points, int pointCount);
    void markRasterOverlay(const QPoint *points, int pointCount);
    void unmarkRasterOverlay(const QRectF &);

#ifdef QT_BLITTER_RASTEROVERLAY
    void mergeOverlay();
    void unmergeOverlay();
    QImage *overlay();

#endif //QT_BLITTER_RASTEROVERLAY
protected:
    QScopedPointer<QBlitterPaintEngine> m_engine;
    QScopedPointer<QBlittable> m_blittable;
    bool m_alpha;
    qreal m_devicePixelRatio;

#ifdef QT_BLITTER_RASTEROVERLAY
    QImage *m_rasterOverlay;
    QImage *m_unmergedCopy;
    QColor m_overlayColor;

    void markRasterOverlayImpl(const QRectF &);
    void unmarkRasterOverlayImpl(const QRectF &);
    QRectF clipAndTransformRect(const QRectF &) const;
#endif //QT_BLITTER_RASTEROVERLAY

};

inline void QBlittablePlatformPixmap::markRasterOverlay(const QRectF &rect)
{
#ifdef QT_BLITTER_RASTEROVERLAY
    markRasterOverlayImpl(rect);
#else
   Q_UNUSED(rect);
#endif
}

inline void QBlittablePlatformPixmap::markRasterOverlay(const QVectorPath &path)
{
#ifdef QT_BLITTER_RASTEROVERLAY
    markRasterOverlayImpl(path.convertToPainterPath().boundingRect());
#else
    Q_UNUSED(path);
#endif
}

inline void QBlittablePlatformPixmap::markRasterOverlay(const QPointF &pos, const QTextItem &ti)
{
#ifdef QT_BLITTER_RASTEROVERLAY
    QFontMetricsF fm(ti.font());
    QRectF rect = fm.tightBoundingRect(ti.text());
    rect.moveBottomLeft(pos);
    markRasterOverlay(rect);
#else
    Q_UNUSED(pos);
    Q_UNUSED(ti);
#endif
}

inline void QBlittablePlatformPixmap::markRasterOverlay(const QRect *rects, int rectCount)
{
#ifdef QT_BLITTER_RASTEROVERLAY
    for (int i = 0; i < rectCount; i++) {
        markRasterOverlay(rects[i]);
    }
#else
    Q_UNUSED(rects);
    Q_UNUSED(rectCount);
#endif
}
inline void QBlittablePlatformPixmap::markRasterOverlay(const QRectF *rects, int rectCount)
{
#ifdef QT_BLITTER_RASTEROVERLAY
    for (int i = 0; i < rectCount; i++) {
        markRasterOverlay(rects[i]);
    }
#else
    Q_UNUSED(rects);
    Q_UNUSED(rectCount);
#endif
}

inline void QBlittablePlatformPixmap::markRasterOverlay(const QPointF *points, int pointCount)
{
#ifdef QT_BLITTER_RASTEROVERLAY
#error "not ported yet"
#else
    Q_UNUSED(points);
    Q_UNUSED(pointCount);
#endif
}

inline void QBlittablePlatformPixmap::markRasterOverlay(const QPoint *points, int pointCount)
{
#ifdef QT_BLITTER_RASTEROVERLAY
#error "not ported yet"
#else
    Q_UNUSED(points);
    Q_UNUSED(pointCount);
#endif
}

inline void QBlittablePlatformPixmap::markRasterOverlay(const QPainterPath& path)
{
#ifdef QT_BLITTER_RASTEROVERLAY
#error "not ported yet"
#else
    Q_UNUSED(path);
#endif
}

inline void QBlittablePlatformPixmap::unmarkRasterOverlay(const QRectF &rect)
{
#ifdef QT_BLITTER_RASTEROVERLAY
    unmarkRasterOverlayImpl(rect);
#else
    Q_UNUSED(rect);
#endif
}

QT_END_NAMESPACE
#endif // QT_NO_BLITTABLE
#endif // QPIXMAP_BLITTER_P_H
