// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QPAINTENGINE_ALPHA_P_H
#define QPAINTENGINE_ALPHA_P_H

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

#include <QtPrintSupport/private/qtprintsupportglobal_p.h>

#ifndef QT_NO_PRINTER
#include "private/qpaintengine_p.h"
#include <QtPrintSupport/qtprintsupportglobal.h>

QT_BEGIN_NAMESPACE

class QAlphaPaintEnginePrivate;

class Q_PRINTSUPPORT_EXPORT QAlphaPaintEngine : public QPaintEngine
{
    Q_DECLARE_PRIVATE(QAlphaPaintEngine)
public:
    ~QAlphaPaintEngine();

    bool begin(QPaintDevice *pdev) override;
    bool end() override;

    void updateState(const QPaintEngineState &state)  override;

    void drawPath(const QPainterPath &path)  override;

    void drawPolygon(const QPointF *points, int pointCount, PolygonDrawMode mode) override;

    void drawPixmap(const QRectF &r, const QPixmap &pm, const QRectF &sr) override;
    void drawTextItem(const QPointF &p, const QTextItem &textItem) override;
    void drawTiledPixmap(const QRectF &r, const QPixmap &pixmap, const QPointF &s) override;

protected:
    QAlphaPaintEngine(QAlphaPaintEnginePrivate &data, PaintEngineFeatures devcaps = { });
    QRegion alphaClipping() const;
    bool continueCall() const;
    void flushAndInit(bool init = true);
    void cleanUp();
};

class QAlphaPaintEnginePrivate : public QPaintEnginePrivate
{
    Q_DECLARE_PUBLIC(QAlphaPaintEngine)
public:
    QAlphaPaintEnginePrivate();
    ~QAlphaPaintEnginePrivate();

    int m_pass;
    QPicture *m_pic;
    QPaintEngine *m_picengine;
    QPainter *m_picpainter;

    QPaintEngine::PaintEngineFeatures m_savedcaps;
    QPaintDevice *m_pdev;

    QRegion m_alphargn;
    QRegion m_cliprgn;
    mutable QRegion m_cachedDirtyRgn;
    mutable int m_numberOfCachedRects;
    QList<QRect> m_dirtyRects;

    bool m_hasalpha;
    bool m_alphaPen;
    bool m_alphaBrush;
    bool m_alphaOpacity;
    bool m_advancedPen;
    bool m_advancedBrush;
    bool m_complexTransform;
    bool m_emulateProjectiveTransforms;
    bool m_continueCall;

    QTransform m_transform;
    QPen m_pen;

    void addAlphaRect(const QRectF &rect);
    void addDirtyRect(const QRectF &rect) { m_dirtyRects.append(rect.toAlignedRect()); }
    bool canSeeTroughBackground(bool somethingInRectHasAlpha, const QRectF &rect) const;

    QRectF addPenWidth(const QPainterPath &path);
    void drawAlphaImage(const QRectF &rect);
    QRect toRect(const QRectF &rect) const;
    bool fullyContained(const QRectF &rect) const;

    void resetState(QPainter *p);
};

QT_END_NAMESPACE

#endif // QT_NO_PRINTER

#endif // QPAINTENGINE_ALPHA_P_H
