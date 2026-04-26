#include "ColoredSvgImageProvider.h"

#include <QtCore/QFileInfo>
#include <QtCore/QLoggingCategory>
#include <QtGui/QImage>
#include <QtGui/QPainter>
#include <QtSvg/QSvgRenderer>

Q_LOGGING_CATEGORY(ColoredSvgImageProviderLog, "QmlControls.ColoredSvgImageProvider")

ColoredSvgImageProvider::ColoredSvgImageProvider()
    : QQuickImageProvider(QQuickImageProvider::Image)
{
}

QImage ColoredSvgImageProvider::requestImage(const QString &id, QSize *size, const QSize &requestedSize)
{
    qCDebug(ColoredSvgImageProviderLog) << "id:" << id << "requestedSize:" << requestedSize;

    const qsizetype q = id.indexOf(QLatin1Char('?'));
    if (q < 0) {
        qCWarning(ColoredSvgImageProviderLog) << "missing ?color= in id:" << id;
        return {};
    }

    QString path = id.left(q);
    const QString query = id.mid(q + 1);

    // Normalize "/foo" or "qrc:/foo" to a Qt resource path ":/foo".
    if (path.startsWith(QLatin1String("qrc:/"))) {
        path = path.mid(3);
    } else if (path.startsWith(QLatin1Char('/'))) {
        path = QLatin1Char(':') + path;
    } else {
        path = QStringLiteral(":/") + path;
    }

    QColor tint;
    for (const QString &kv : query.split(QLatin1Char('&'), Qt::SkipEmptyParts)) {
        if (kv.startsWith(QLatin1String("color="))) {
            tint = QColor(QLatin1Char('#') + kv.mid(6));
            break;
        }
    }
    if (!tint.isValid()) {
        qCWarning(ColoredSvgImageProviderLog) << "invalid color in id:" << id;
        return {};
    }

    const bool isSvg = QFileInfo(path).suffix().compare(QLatin1String("svg"), Qt::CaseInsensitive) == 0;

    QSize outSize = requestedSize;
    QImage rendered;

    // QML callers commonly set only sourceSize.height, leaving width=0 ("scale by aspect").
    // Fill the missing dim from the source's intrinsic aspect so we rasterize at the final
    // display resolution rather than defaultSize() and let Image scale it (blurry).
    auto fillMissingDim = [](QSize req, QSize intrinsic) -> QSize {
        if (intrinsic.width() <= 0 || intrinsic.height() <= 0) {
            return req;
        }
        const bool hasW = req.width()  > 0;
        const bool hasH = req.height() > 0;
        if (hasW && !hasH) {
            return {req.width(), qRound(req.width() * (qreal(intrinsic.height()) / intrinsic.width()))};
        }
        if (hasH && !hasW) {
            return {qRound(req.height() * (qreal(intrinsic.width()) / intrinsic.height())), req.height()};
        }
        return req;
    };

    if (isSvg) {
        QSvgRenderer renderer(path);
        if (!renderer.isValid()) {
            qCWarning(ColoredSvgImageProviderLog) << "QSvgRenderer rejected:" << path;
            return {};
        }
        // Default is IgnoreAspectRatio — would distort the icon when target size isn't square.
        renderer.setAspectRatioMode(Qt::KeepAspectRatio);
        outSize = fillMissingDim(outSize, renderer.defaultSize());
        if (outSize.width() <= 0 || outSize.height() <= 0) {
            outSize = renderer.defaultSize();
        }
        outSize = outSize.expandedTo(QSize(1, 1));
        rendered = QImage(outSize, QImage::Format_ARGB32_Premultiplied);
        rendered.fill(Qt::transparent);
        QPainter p(&rendered);
        p.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
        renderer.render(&p);
    } else {
        QImage src(path);
        if (src.isNull()) {
            qCWarning(ColoredSvgImageProviderLog) << "failed to load:" << path;
            return {};
        }
        outSize = fillMissingDim(outSize, src.size());
        if (outSize.width() > 0 && outSize.height() > 0 && outSize != src.size()) {
            src = src.scaled(outSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        }
        outSize = src.size().expandedTo(QSize(1, 1));
        rendered = src.convertToFormat(QImage::Format_ARGB32_Premultiplied);
    }

    // SourceIn replaces RGB but preserves the source alpha mask — exactly the old ColorOverlay semantic.
    QPainter p(&rendered);
    p.setCompositionMode(QPainter::CompositionMode_SourceIn);
    p.fillRect(rendered.rect(), tint);
    p.end();

    if (size != nullptr) {
        *size = outSize;
    }
    return rendered;
}
