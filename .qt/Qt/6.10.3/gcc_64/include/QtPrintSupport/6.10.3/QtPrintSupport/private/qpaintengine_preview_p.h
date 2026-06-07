// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QPAINTENGINE_PREVIEW_P_H
#define QPAINTENGINE_PREVIEW_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of QPreviewPrinter and friends.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//
//

#include <QtPrintSupport/private/qtprintsupportglobal_p.h>
#include <QtGui/qpaintengine.h>
#include <QtPrintSupport/qprintengine.h>

QT_REQUIRE_CONFIG(printpreviewwidget);

QT_BEGIN_NAMESPACE

class QPreviewPaintEnginePrivate;

class QPreviewPaintEngine : public QPaintEngine, public QPrintEngine
{
    Q_DECLARE_PRIVATE(QPreviewPaintEngine)
public:
    QPreviewPaintEngine();
    ~QPreviewPaintEngine();

    bool begin(QPaintDevice *dev) override;
    bool end() override;

    void updateState(const QPaintEngineState &state) override;

    void drawPath(const QPainterPath &path) override;
    void drawPolygon(const QPointF *points, int pointCount, PolygonDrawMode mode) override;
    void drawTextItem(const QPointF &p, const QTextItem &textItem) override;

    void drawPixmap(const QRectF &r, const QPixmap &pm, const QRectF &sr) override;
    void drawTiledPixmap(const QRectF &r, const QPixmap &pm, const QPointF &p) override;

    QList<const QPicture *> pages();

    QPaintEngine::Type type() const override { return Picture; }

    void setProxyEngines(QPrintEngine *printEngine, QPaintEngine *paintEngine);

    void setProperty(PrintEnginePropertyKey key, const QVariant &value) override;
    QVariant property(PrintEnginePropertyKey key) const override;

    bool newPage() override;
    bool abort() override;

    int metric(QPaintDevice::PaintDeviceMetric) const override;

    QPrinter::PrinterState printerState() const override;
};

QT_END_NAMESPACE

#endif
