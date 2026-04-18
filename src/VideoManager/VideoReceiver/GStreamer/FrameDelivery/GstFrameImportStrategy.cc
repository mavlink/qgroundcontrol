#include "GstFrameImportStrategy.h"

#include "GstCpuVideoBuffer.h"
#include "GstDmaBufRhiVideoBuffer.h"

#ifdef QGC_GST_DMABUF
#include <gst/allocators/gstdmabuf.h>
#endif

namespace GstFrameImportStrategy {

Kind select(GstBuffer* buffer)
{
#ifdef QGC_GST_DMABUF
    GstMemory* firstMem = buffer ? gst_buffer_peek_memory(buffer, 0) : nullptr;
    if (firstMem && gst_is_dmabuf_memory(firstMem))
        return Kind::DmaBufRhi;
#else
    Q_UNUSED(buffer);
#endif

    return Kind::Cpu;
}

std::unique_ptr<QAbstractVideoBuffer> create(Kind kind,
                                             GstBuffer* buffer,
                                             const GstVideoInfo& videoInfo,
                                             const QVideoFrameFormat& format)
{
#ifdef QGC_GST_DMABUF
    if (kind == Kind::DmaBufRhi)
        return std::make_unique<GstDmaBufRhiVideoBuffer>(buffer, videoInfo, format);
#else
    Q_UNUSED(kind);
#endif

    return std::make_unique<GstCpuVideoBuffer>(buffer, videoInfo, format);
}

}  // namespace GstFrameImportStrategy
