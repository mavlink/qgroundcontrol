#include "GstVideoBufferFactory.h"

#include "GstFrameImportStrategy.h"
#include "GstFormatTable.h"

namespace GstVideoBufferFactory {

QByteArray appsinkCaps()
{
    QByteArray caps;
#ifdef QGC_GST_DMABUF
    // Prefer DMA-BUF so hardware decoders can avoid a forced software
    // videoconvert/download. GstDmaBufRhiVideoBuffer imports into QRhi textures
    // when possible and falls back to mmap for unsupported render backends.
    caps = GstFormatTable::dmaBufDrmCapsFormats() + ';' + GstFormatTable::dmaBufCapsFormats() + ';';
#endif
    caps += GstFormatTable::cpuCapsFormats();
    return caps;
}

MemoryKind classify(GstBuffer* buffer)
{
    if (!buffer)
        return MemoryKind::Unknown;

    if (GstFrameImportStrategy::select(buffer) == GstFrameImportStrategy::Kind::DmaBufRhi)
        return MemoryKind::DmaBuf;

    return MemoryKind::Cpu;
}

std::unique_ptr<QAbstractVideoBuffer> create(GstBuffer* buffer,
                                             const GstVideoInfo& videoInfo,
                                             const QVideoFrameFormat& format)
{
    return GstFrameImportStrategy::create(GstFrameImportStrategy::select(buffer), buffer, videoInfo, format);
}

}  // namespace GstVideoBufferFactory
