#include "QGCTileFallback.h"

#include <QtCore/QBuffer>
#include <QtGui/QImage>
#include <QtGui/QPainter>

QImage scaleAncestorToChild(const QImage &ancestor, int x, int y, int levelDelta, QSize tileSize)
{
    if (ancestor.isNull() || (levelDelta <= 0) || tileSize.isEmpty()) {
        return QImage();
    }

    const int divisions = 1 << levelDelta;  // 2^levelDelta cells per ancestor side
    const int cellX = x & (divisions - 1);   // x mod 2^levelDelta
    const int cellY = y & (divisions - 1);   // y mod 2^levelDelta

    const int subW = ancestor.width() / divisions;
    const int subH = ancestor.height() / divisions;
    if ((subW <= 0) || (subH <= 0)) {
        return QImage();
    }

    // Scale the ancestor sub-square straight into the target tile in one pass,
    // avoiding the intermediate heap allocation that copy()+scaled() would make.
    QImage result(tileSize, QImage::Format_ARGB32_Premultiplied);
    result.fill(Qt::transparent);
    QPainter painter(&result);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    painter.drawImage(QRectF(QPointF(0, 0), QSizeF(tileSize)),
                      ancestor,
                      QRectF(cellX * subW, cellY * subH, subW, subH));
    painter.end();
    return result;
}

QByteArray encodeFallbackTile(const QImage &image)
{
    if (image.isNull()) {
        return QByteArray();
    }
    QByteArray bytes;
    QBuffer buffer(&bytes);
    if (!buffer.open(QIODevice::WriteOnly)) {
        return QByteArray();
    }
    if (!image.save(&buffer, "PNG")) {
        return QByteArray();
    }
    return bytes;
}
