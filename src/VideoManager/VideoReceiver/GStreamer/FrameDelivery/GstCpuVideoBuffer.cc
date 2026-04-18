#include "GstCpuVideoBuffer.h"

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GstCpuVideoBufferLog, "Video.GstCpuVideoBuffer")

GstCpuVideoBuffer::GstCpuVideoBuffer(GstBuffer* buffer, const GstVideoInfo& videoInfo, const QVideoFrameFormat& format)
    : _buffer(gst_buffer_ref(buffer)), _videoInfo(videoInfo), _format(format)
{}

GstCpuVideoBuffer::~GstCpuVideoBuffer()
{
    if (_mapped) {
        gst_video_frame_unmap(&_mappedFrame);
        _mapped = false;
    }
    if (_buffer)
        gst_buffer_unref(_buffer);
}

QAbstractVideoBuffer::MapData GstCpuVideoBuffer::map(QVideoFrame::MapMode mode)
{
    MapData data;

    // QAbstractVideoBuffer::map() is non-concurrent per instance — Qt serializes
    // map/unmap on the owning QVideoFrame. Nested map without unmap is a caller bug.
    Q_ASSERT(!_mapped);
    if (_mapped) {
        qCWarning(GstCpuVideoBufferLog) << "already mapped — ignoring nested map()";
        return data;
    }

    // QVideoFrame::MapMode is an enum whose values are bit-compatible
    // (ReadOnly=1, WriteOnly=2, ReadWrite=ReadOnly|WriteOnly). Compose
    // GstMapFlags additively rather than using equality checks, so we
    // remain correct if Qt ever extends the enum.
    const auto modeBits = static_cast<unsigned>(mode);
    guint flagsBits = 0;
    if (modeBits & QVideoFrame::ReadOnly)
        flagsBits |= GST_MAP_READ;
    if (modeBits & QVideoFrame::WriteOnly)
        flagsBits |= GST_MAP_WRITE;
    if (flagsBits == 0)
        flagsBits = GST_MAP_READ;  // NotMapped / unknown → treat as read
    const auto flags = static_cast<GstMapFlags>(flagsBits);

    if (!gst_video_frame_map(&_mappedFrame, &_videoInfo, _buffer, flags)) {
        qCWarning(GstCpuVideoBufferLog) << "Failed to map GstBuffer for CPU access";
        return data;
    }

    _mapped = true;

    data.planeCount = GST_VIDEO_FRAME_N_PLANES(&_mappedFrame);
    for (int i = 0; i < data.planeCount; ++i) {
        data.data[i] = static_cast<uchar*>(GST_VIDEO_FRAME_PLANE_DATA(&_mappedFrame, i));
        data.bytesPerLine[i] = GST_VIDEO_FRAME_PLANE_STRIDE(&_mappedFrame, i);

        const int compHeight = GST_VIDEO_FRAME_COMP_HEIGHT(&_mappedFrame, i);
        data.dataSize[i] = data.bytesPerLine[i] * compHeight;
    }

    return data;
}

void GstCpuVideoBuffer::unmap()
{
    if (_mapped) {
        gst_video_frame_unmap(&_mappedFrame);
        _mapped = false;
    }
}

QVideoFrameFormat GstCpuVideoBuffer::format() const
{
    return _format;
}
