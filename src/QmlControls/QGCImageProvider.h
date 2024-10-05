/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QObject>
#include <QtGui/QImage>
#include <QtQuick/QQuickImageProvider>

/// This is used to expose images from ImageProtocolHandler
class QGCImageProvider : public QQuickImageProvider
{
public:
    QGCImageProvider(QQmlImageProviderBase::ImageType type = QQmlImageProviderBase::ImageType::Image);
    ~QGCImageProvider();

    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize) final;
    void setImage(const QImage &image, uint8_t vehicleId = 0) { _images[vehicleId] = image.mirrored(); }

private:
    QMap<uint8_t, QImage> _images;
    QImage _dummy;
};
