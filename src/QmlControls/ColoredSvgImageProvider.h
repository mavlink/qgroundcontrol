#pragma once

#include <QtQuick/QQuickImageProvider>

/// Image provider that rasterizes an SVG (or any QImage-loadable Qt resource)
/// and recolors its alpha mask with a caller-supplied tint, avoiding the need
/// for a runtime ColorOverlay shader.
///
/// URL format: image://coloredsvg/<resource-path>?color=<RRGGBB|AARRGGBB>
class ColoredSvgImageProvider : public QQuickImageProvider
{
public:
    static constexpr const char *ProviderId = "coloredsvg";

    ColoredSvgImageProvider();

    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize) final;
};
