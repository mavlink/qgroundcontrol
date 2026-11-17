#pragma once

#include <gst/video/video.h>
#include <memory>
#include <string>
#include <utility>

#include <QtCore/QString>
#include <QtGui/QColor>
#include <QtGui/QImage>
#include <QtMultimedia/QVideoFrame>
#include <QtMultimedia/private/qhwvideobuffer_p.h>
#include <QtMultimedia/private/qvideoframeconverter_p.h>

#include "GStreamerTestHelpers.h"
#include "GstScoped.h"

#if defined(QGC_HAS_GST_DMABUF_GPU_PATH)
#include <cerrno>
#include <gst/allocators/gstdmabuf.h>
#include <poll.h>
#endif

class QRhi;

/// Pixel checks of GPU frames through QVideoFrame::toImage(), which converts them on a QRhi of their API: D3D11/D3D12,
/// Metal, and GL for glupload's GLMemory. And the VA frames the DMABuf readback cases import.
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

#if defined(QGC_HAS_GST_DMABUF_GPU_PATH)

/// True once the driver has finished writing @p buffer's dma-bufs, whose write fences poll() waits for.
inline bool waitForDmaBufWrites(GstBuffer* buffer, int timeoutMs)
{
    for (guint i = 0; i < gst_buffer_n_memory(buffer); ++i) {
        GstMemory* const memory = gst_buffer_peek_memory(buffer, i);
        if (!gst_is_dmabuf_memory(memory)) {
            continue;
        }
        pollfd pfd = {gst_dmabuf_memory_get_fd(memory), POLLIN, 0};
        int ret = 0;
        do {
            ret = ::poll(&pfd, 1, timeoutMs);
        } while ((ret < 0) && (errno == EINTR));
        if ((ret <= 0) || !(pfd.revents & POLLIN)) {
            return false;
        }
    }
    return true;
}

/// A frame a VA element left in DMABuf memory and the same frame downloaded to system memory, with the pipeline that
/// made them.
struct VaFramePair
{
    TestPipeline pipeline;
    SamplePtr gpu;
    SamplePtr cpu;
};

/// One SMPTE frame of @p source through the VA element @p producer twice: into the layout the driver picks among
/// those of @p drmCaps, the DMA_DRM caps an importer offers (usually a tiled one), and downloaded to system memory as
/// @p cpuFormat, the reference. A frame sampled with the wrong pitch, plane offset, plane format or tiling shows in
/// the bars. Skips the test where this host's VA driver can't; @p frames then holds no sample.
inline void pullVaFramePair(VaFramePair& frames, const QString& source, const QString& producer, std::string drmCaps,
                            GstVideoFormat cpuFormat)
{
    drmCaps.resize(drmCaps.find_last_not_of("; ") + 1);
    TestPipeline& pipeline = frames.pipeline;
    if (!pipeline.launch(QStringLiteral("videotestsrc num-buffers=1 pattern=smpte ! %1 ! tee name=t "
                                        "t. ! queue ! %2 ! capsfilter name=gpucaps ! fakesink name=gpu "
                                        "t. ! queue ! %2 ! video/x-raw,format=%3 ! fakesink name=cpu")
                             .arg(source, producer, QLatin1String(gst_video_format_to_string(cpuFormat))))) {
        QSKIP(qPrintable(QStringLiteral("VA pipeline parse skipped: %1").arg(pipeline.error())));
    }
    const GStreamer::GstCapsPtr gpuCaps = GStreamer::adoptCaps(gst_caps_from_string(drmCaps.c_str()));
    QVERIFY(gpuCaps);
    g_object_set(pipeline.element("gpucaps").get(), "caps", gpuCaps.get(), nullptr);
    // VA hands out DMA_DRM memory only to a peer that takes GstVideoMeta, as qgcqvideosink does.
    const GStreamer::GstPadPtr gpuPad =
        GStreamer::adoptPad(gst_element_get_static_pad(pipeline.element("gpu").get(), "sink"));
    QVERIFY(gpuPad);
    GStreamer::PadProbe allocationProbe;
    QVERIFY(allocationProbe.add(
        gpuPad.get(), static_cast<GstPadProbeType>(GST_PAD_PROBE_TYPE_QUERY_DOWNSTREAM | GST_PAD_PROBE_TYPE_PUSH),
        +[](GstPad*, GstPadProbeInfo* info, gpointer) -> GstPadProbeReturn {
            GstQuery* query = GST_PAD_PROBE_INFO_QUERY(info);
            if (GST_QUERY_TYPE(query) != GST_QUERY_ALLOCATION) {
                return GST_PAD_PROBE_OK;
            }
            gst_query_add_allocation_meta(query, GST_VIDEO_META_API_TYPE, nullptr);
            return GST_PAD_PROBE_HANDLED;
        },
        nullptr));
    if (!pipeline.play()) {
        QSKIP("VA pipeline failed to PLAY (no VA driver / DRI device?)");
    }
    const QString failure = pipeline.runToEos();
    if (!failure.isEmpty()) {
        QSKIP(qPrintable(QStringLiteral("VA driver can't produce the offered layouts (%1): %2")
                             .arg(QString::fromStdString(drmCaps), failure)));
    }
    SamplePtr gpu = pipeline.lastSample("gpu");
    SamplePtr cpu = pipeline.lastSample("cpu");
    QVERIFY(gpu && cpu);
    // The importer waits 100 ms for the driver to finish a frame, then shows it some other way. A busy GPU takes
    // longer over a postprocessor's frame, and these tests are about the frame's layout, not about that wait.
    QVERIFY2(waitForDmaBufWrites(gst_sample_get_buffer(gpu.get()), TestTimeout::mediumMs()),
             "the VA driver did not finish writing the frame");
    frames.gpu = std::move(gpu);
    frames.cpu = std::move(cpu);
}

#endif  // QGC_HAS_GST_DMABUF_GPU_PATH

}  // namespace GstTest
