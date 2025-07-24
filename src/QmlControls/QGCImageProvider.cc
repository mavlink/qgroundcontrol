/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "QGCImageProvider.h"

#include <QtGui/QPainter>
#include <QtGui/QFont>

QGCImageProvider::QGCImageProvider(QQmlImageProviderBase::ImageType imageType)
    : QQuickImageProvider(imageType)
    , _dummy(320, 240, QImage::Format_RGBA8888)
{
    // qCDebug(ImageProtocolManagerLog) << Q_FUNC_INFO << this;

    Q_ASSERT(imageType == QQmlImageProviderBase::ImageType::Image);

    // Dummy temporary image until something comes along
    _dummy.fill(Qt::black);
    QPainter painter(&_dummy);
    QFont f = painter.font();
    f.setPixelSize(20);
    painter.setFont(f);
    painter.setPen(Qt::white);
    painter.drawText(QRectF(0, 0, _dummy.width(), _dummy.height()), Qt::AlignCenter, QStringLiteral("Waiting..."));
    _images[0] = _dummy;
}

QGCImageProvider::~QGCImageProvider()
{
    // qCDebug(ImageProtocolManagerLog) << Q_FUNC_INFO << this;
}

QImage QGCImageProvider::requestImage(const QString &id, QSize *size, const QSize &requestedSize)
{
    Q_UNUSED(requestedSize);

    if (id.isEmpty()) {
        return _dummy;
    }

    if (!id.contains("/")) {
        return _dummy;
    }

    const QStringList url = id.split('/', Qt::SkipEmptyParts);
    if (url.size() != 2) {
        return _dummy;
    }

    bool ok = false;
    const uint8_t vehicleId = url[0].toUInt(&ok);
    if (!ok) {
        return _dummy;
    }

    const uint8_t index = url[1].toUInt(&ok);
    if (!ok) {
        return _dummy;
    }

    if (!_images.contains(vehicleId)) {
        return _dummy;
    }

    const QImage image = _images[vehicleId];
    // image->scaled(requestedSize);
    *size = image.size();

    return image;
}
