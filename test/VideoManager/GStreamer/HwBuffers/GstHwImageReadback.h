#pragma once

#include <memory>

#include <QtCore/QString>
#include <QtGui/QColor>
#include <QtGui/QImage>
#include <QtMultimedia/QVideoFrame>
#include <QtMultimedia/private/qhwvideobuffer_p.h>
#include <QtMultimedia/private/qvideoframeconverter_p.h>

class QRhi;

/// Pixel checks of GPU frames through QVideoFrame::toImage(), which converts them on a QRhi of their API: D3D11/D3D12,
/// Metal, and GL for glupload's GLMemory.
namespace GstTest {

/// Wraps a GPU path's buffer so QVideoFrame::toImage() converts it with @p rhi, the QRhi the test created, as it uses
/// the render thread's QRhi for a frame mapped there, instead of a thread-local one of Qt's choosing.
class RhiBoundVideoBuffer final : public QHwVideoBuffer
{
public:
    RhiBoundVideoBuffer(std::unique_ptr<QHwVideoBuffer> buffer, QRhi* rhi)
        : QHwVideoBuffer(buffer->handleType())
        , _buffer(std::move(buffer))
        , _rhi(rhi)
    {}

    QRhi* associatedCurrentThreadRhi() const override { return _rhi; }

    QVideoFrameFormat format() const override { return _buffer->format(); }

    MapData map(QVideoFrame::MapMode mode) override { return _buffer->map(mode); }

    void unmap() override { _buffer->unmap(); }

    QMatrix4x4 externalTextureMatrix() const override { return _buffer->externalTextureMatrix(); }

    QVideoFrameTexturesUPtr mapTextures(QRhi& rhi, QVideoFrameTexturesUPtr& oldTextures) override
    {
        return _buffer->mapTextures(rhi, oldTextures);
    }

    void initTextureConverter(QRhi& rhi) override { _buffer->initTextureConverter(rhi); }

private:
    std::unique_ptr<QHwVideoBuffer> _buffer;
    QRhi* _rhi = nullptr;
};

/// @p frame converted on the CPU, the reference for a GPU conversion. QVideoFrame::toImage() would first probe for a
/// thread-local QRhi, which fails with a warning on the offscreen platform tests run on.
inline QImage cpuImage(const QVideoFrame& frame)
{
    return qImageFromVideoFrame(frame, /*forceCpu=*/true);
}

/// Where @p actual first differs from @p expected by more than @p tolerance in a color channel (GPU and CPU YUV
/// conversions round differently); empty when they match. A mismatch within @p edgeRadius pixels of a color edge in
/// @p expected is let through: a GPU samples subsampled chroma (NV12) bilinearly across the edge, where the CPU
/// conversion repeats each chroma sample. A wrong pitch, plane offset or padding still shows inside the bars.
inline QString firstImageMismatch(const QImage& actual, const QImage& expected, int tolerance, int edgeRadius = 0)
{
    if (actual.isNull() || expected.isNull()) {
        return QStringLiteral("no image (actual null: %1, expected null: %2)")
            .arg(actual.isNull())
            .arg(expected.isNull());
    }
    if (actual.size() != expected.size()) {
        return QStringLiteral("size %1x%2, expected %3x%4")
            .arg(actual.width())
            .arg(actual.height())
            .arg(expected.width())
            .arg(expected.height());
    }
    const auto close = [tolerance](const QColor& a, const QColor& b) {
        return (qAbs(a.red() - b.red()) <= tolerance) && (qAbs(a.green() - b.green()) <= tolerance) &&
               (qAbs(a.blue() - b.blue()) <= tolerance);
    };
    const auto nearEdge = [&expected, &close, edgeRadius](int x, int y, const QColor& want) {
        for (int ny = qMax(0, y - edgeRadius); ny <= qMin(expected.height() - 1, y + edgeRadius); ++ny) {
            for (int nx = qMax(0, x - edgeRadius); nx <= qMin(expected.width() - 1, x + edgeRadius); ++nx) {
                if (!close(expected.pixelColor(nx, ny), want)) {
                    return true;
                }
            }
        }
        return false;
    };
    for (int y = 0; y < expected.height(); ++y) {
        for (int x = 0; x < expected.width(); ++x) {
            const QColor got = actual.pixelColor(x, y);
            const QColor want = expected.pixelColor(x, y);
            if (!close(got, want) && !nearEdge(x, y, want)) {
                return QStringLiteral("(%1,%2): got %3, expected %4").arg(x).arg(y).arg(got.name(), want.name());
            }
        }
    }
    return {};
}

}  // namespace GstTest
