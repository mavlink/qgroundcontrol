// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QPAINTENGINE_PIC_P_H
#define QPAINTENGINE_PIC_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtGui/private/qtguiglobal_p.h>
#include <QtGui/qpaintengine.h>

#ifndef QT_NO_PICTURE

QT_BEGIN_NAMESPACE

class QPicturePaintEnginePrivate;
class QBuffer;

class QPicturePaintEngine : public QPaintEngine
{
    Q_DECLARE_PRIVATE(QPicturePaintEngine)
public:
    QPicturePaintEngine();
    ~QPicturePaintEngine();

    bool begin(QPaintDevice *pdev) override;
    bool end() override;

    void updateState(const QPaintEngineState &state) override;

    void updatePen(const QPen &pen);
    void updateBrush(const QBrush &brush);
    void updateBrushOrigin(const QPointF &origin);
    void updateFont(const QFont &font);
    void updateBackground(Qt::BGMode bgmode, const QBrush &bgBrush);
    void updateMatrix(const QTransform &matrix);
    void updateClipRegion(const QRegion &region, Qt::ClipOperation op);
    void updateClipPath(const QPainterPath &path, Qt::ClipOperation op);
    void updateRenderHints(QPainter::RenderHints hints);
    void updateCompositionMode(QPainter::CompositionMode cmode);
    void updateClipEnabled(bool enabled);
    void updateOpacity(qreal opacity);

    void drawEllipse(const QRectF &rect) override;
    void drawPath(const QPainterPath &path) override;
    void drawPolygon(const QPointF *points, int numPoints, PolygonDrawMode mode) override;
    using QPaintEngine::drawPolygon;

    void drawPixmap(const QRectF &r, const QPixmap &pm, const QRectF &sr) override;
    void drawTiledPixmap(const QRectF &r, const QPixmap &pixmap, const QPointF &s) override;
    void drawImage(const QRectF &r, const QImage &image, const QRectF &sr,
                   Qt::ImageConversionFlags flags = Qt::AutoColor) override;
    void drawTextItem(const QPointF &p, const QTextItem &ti) override;

    Type type() const override { return Picture; }

protected:
    QPicturePaintEngine(QPaintEnginePrivate &dptr);

private:
    Q_DISABLE_COPY_MOVE(QPicturePaintEngine)

    void writeCmdLength(int pos, const QRectF &r, bool corr);
};

QT_END_NAMESPACE

#endif // QT_NO_PICTURE

#endif // QPAINTENGINE_PIC_P_H
