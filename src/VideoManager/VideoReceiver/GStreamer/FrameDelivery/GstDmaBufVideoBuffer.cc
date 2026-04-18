#include "GstDmaBufVideoBuffer.h"

#ifdef QGC_GST_DMABUF

#include <sys/mman.h>
#include <unistd.h>

#include <gst/allocators/gstdmabuf.h>
#include <gst/video/video-frame.h>

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GstDmaBufVideoBufferLog, "Video.GstDmaBufVideoBuffer")

GstDmaBufVideoBuffer::GstDmaBufVideoBuffer(GstBuffer* buffer, const GstVideoInfo& videoInfo,
                                           const QVideoFrameFormat& format)
    : _buffer(gst_buffer_ref(buffer)), _videoInfo(videoInfo), _format(format)
{
    _planeCount = GST_VIDEO_INFO_N_PLANES(&_videoInfo);
    if (_planeCount < 0 || _planeCount > 4) {
        qCWarning(GstDmaBufVideoBufferLog) << "Unexpected plane count:" << _planeCount;
        _planeCount = 0;
    }
}

GstDmaBufVideoBuffer::~GstDmaBufVideoBuffer()
{
    if (_mapped) {
        _doUnmap();
    }
    if (_buffer) {
        gst_buffer_unref(_buffer);
    }
}

QAbstractVideoBuffer::MapData GstDmaBufVideoBuffer::map(QVideoFrame::MapMode mode)
{
    // If already mapped, return cached pointers — the fd and mmap region are
    // stable for the buffer's lifetime (GstBuffer holds the dmabuf fd open).
    // WriteOnly requests skip the cache conservatively; the render path is
    // always ReadOnly so this branch is never taken in practice.
    const auto modeBits = static_cast<unsigned>(mode);
    const bool wantWrite = (modeBits & QVideoFrame::WriteOnly) != 0;
    if (_mapped && !wantWrite) {
        MapData data;
        data.planeCount = _planeCount;
        const guint nMem = gst_buffer_n_memory(_buffer);
        for (int plane = 0; plane < _planeCount; ++plane) {
            const guint memIdx = (nMem >= static_cast<guint>(_planeCount)) ? static_cast<guint>(plane) : 0U;
            GstMemory* mem = gst_buffer_peek_memory(_buffer, memIdx);
            const gsize memSize = gst_memory_get_sizes(mem, nullptr, nullptr);
            const gsize planeOffset = (memIdx == 0 && nMem < static_cast<guint>(_planeCount))
                                          ? GST_VIDEO_INFO_PLANE_OFFSET(&_videoInfo, plane)
                                          : 0;
            data.data[plane] = static_cast<uchar*>(_planeData[plane]) + planeOffset;
            data.bytesPerLine[plane] = _planeStride[plane];
            data.dataSize[plane] = memSize - planeOffset;
        }
        return data;
    }

    MapData data;

    if (_mapped) {
        // Nested write map — not expected but handle safely.
        qCWarning(GstDmaBufVideoBufferLog) << "nested write map() — ignoring";
        return data;
    }

    // Compose prot flags from Qt's MapMode.
    int prot = 0;
    if (modeBits & QVideoFrame::ReadOnly)
        prot |= PROT_READ;
    if (modeBits & QVideoFrame::WriteOnly)
        prot |= PROT_WRITE;
    if (prot == 0)
        prot = PROT_READ;  // NotMapped / unknown → read

    const guint nMem = gst_buffer_n_memory(_buffer);
    data.planeCount = _planeCount;

    // Multi-plane DMA-BUF can come in two shapes:
    //   (a) one GstMemory per plane (common for V4L2 MPLANE decoders)
    //   (b) single GstMemory covering all planes, planes at GstVideoInfo offsets
    // Qt only needs per-plane pointers — we handle both by keying mmap on memory index.
    for (int plane = 0; plane < _planeCount; ++plane) {
        const guint memIdx = (nMem >= static_cast<guint>(_planeCount)) ? static_cast<guint>(plane) : 0U;
        GstMemory* mem = gst_buffer_peek_memory(_buffer, memIdx);
        if (!mem || !gst_is_dmabuf_memory(mem)) {
            qCWarning(GstDmaBufVideoBufferLog) << "plane" << plane << "memory is not DMA-BUF — aborting map";
            _doUnmap();
            return {};
        }
        const gint fd = gst_dmabuf_memory_get_fd(mem);
        if (fd < 0) {
            qCWarning(GstDmaBufVideoBufferLog) << "gst_dmabuf_memory_get_fd() returned -1 on plane" << plane;
            _doUnmap();
            return {};
        }

        const gsize memSize = gst_memory_get_sizes(mem, nullptr, nullptr);
        const gsize planeOffset = (memIdx == 0 && nMem < static_cast<guint>(_planeCount))
                                      ? GST_VIDEO_INFO_PLANE_OFFSET(&_videoInfo, plane)
                                      : 0;
        const gsize planeSize = memSize - planeOffset;

        void* ptr = mmap(nullptr, memSize, prot, MAP_SHARED, fd, 0);
        if (ptr == MAP_FAILED) {
            qCWarning(GstDmaBufVideoBufferLog) << "mmap() failed on plane" << plane << "fd" << fd;
            _doUnmap();
            return {};
        }

        _planeData[plane] = ptr;
        _planeSize[plane] = memSize;  // _doUnmap needs the total region, not the plane slice
        _planeStride[plane] = GST_VIDEO_INFO_PLANE_STRIDE(&_videoInfo, plane);

        data.data[plane] = static_cast<uchar*>(ptr) + planeOffset;
        data.bytesPerLine[plane] = _planeStride[plane];
        data.dataSize[plane] = planeSize;
    }

    _mapped = true;
    return data;
}

void GstDmaBufVideoBuffer::unmap()
{
    // No-op for cached ReadOnly maps: the region stays valid until the buffer
    // is destroyed. _doUnmap() is called from the destructor.
}

void GstDmaBufVideoBuffer::_doUnmap()
{
    if (_planeData[0] == nullptr)
        return;
    for (int plane = 0; plane < 4; ++plane) {
        if (_planeData[plane] && _planeSize[plane] > 0) {
            // Same region may be aliased across planes when a single GstMemory
            // backs all of them — guard against double munmap by clearing siblings.
            void* ptr = _planeData[plane];
            const size_t size = _planeSize[plane];
            (void)munmap(ptr, size);
            for (int p2 = plane; p2 < 4; ++p2) {
                if (_planeData[p2] == ptr) {
                    _planeData[p2] = nullptr;
                    _planeSize[p2] = 0;
                }
            }
        }
    }
    _mapped = false;
}

QVideoFrameFormat GstDmaBufVideoBuffer::format() const
{
    return _format;
}

#endif  // QGC_GST_DMABUF
