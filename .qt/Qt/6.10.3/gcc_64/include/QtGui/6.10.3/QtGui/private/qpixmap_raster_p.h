// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QPIXMAP_RASTER_P_H
#define QPIXMAP_RASTER_P_H

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


QT_BEGIN_NAMESPACE

class Q_GUI_EXPORT QRasterPlatformPixmap : public QPlatformPixmap
{
public:
    QRasterPlatformPixmap(PixelType type);
    ~QRasterPlatformPixmap();

    QPlatformPixmap *createCompatiblePlatformPixmap() const override;

    void resize(int width, int height) override;
    bool fromData(const uchar *buffer, uint len, const char *format, Qt::ImageConversionFlags flags) override;
    void fromImage(const QImage &image, Qt::ImageConversionFlags flags) override;
    void fromImageInPlace(QImage &image, Qt::ImageConversionFlags flags) override;
    void fromImageReader(QImageReader *imageReader, Qt::ImageConversionFlags flags) override;

    void copy(const QPlatformPixmap *data, const QRect &rect) override;
    bool scroll(int dx, int dy, const QRect &rect) override;
    void fill(const QColor &color) override;
    bool hasAlphaChannel() const override;
    QImage toImage() const override;
    QImage toImage(const QRect &rect) const override;
    QPaintEngine* paintEngine() const override;
    QImage* buffer() override;
    qreal devicePixelRatio() const override;
    void setDevicePixelRatio(qreal scaleFactor) override;


protected:
    int metric(QPaintDevice::PaintDeviceMetric metric) const override;
    void createPixmapForImage(QImage sourceImage, Qt::ImageConversionFlags flags);
    void setImage(const QImage &image);
    QImage image;
    static QImage::Format systemNativeFormat();

private:
    friend class QPixmap;
    friend class QBitmap;
    friend class QPixmapCacheEntry;
    friend class QRasterPaintEngine;
};

QT_END_NAMESPACE

#endif // QPIXMAP_RASTER_P_H


