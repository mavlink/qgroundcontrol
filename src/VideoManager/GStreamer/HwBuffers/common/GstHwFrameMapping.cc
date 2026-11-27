#include "GstHwFrameMapping.h"

#include "GstHwVideoInfo.h"

namespace GstHw {

namespace {

/// Qt map data for a frame mapped with gst_video_frame_map, each plane sized by its own row count.
QAbstractVideoBuffer::MapData mapDataFor(const GstVideoFrame& frame)
{
    QAbstractVideoBuffer::MapData data;
    data.planeCount = static_cast<int>(GST_VIDEO_FRAME_N_PLANES(&frame));
    for (int i = 0; i < data.planeCount; ++i) {
        data.data[i] = static_cast<uchar*>(GST_VIDEO_FRAME_PLANE_DATA(&frame, i));
        data.bytesPerLine[i] = GST_VIDEO_FRAME_PLANE_STRIDE(&frame, i);
        data.dataSize[i] = static_cast<int>(static_cast<qsizetype>(GST_VIDEO_FRAME_PLANE_STRIDE(&frame, i)) *
                                            planeHeight(frame.info, i));
    }
    return data;
}

}  // namespace

bool FrameMapping::mapFrame(GstBuffer* buffer, const GstVideoInfo& info, GstMapFlags flags)
{
    if (!_mapped) {
        _mapped = buffer && gst_video_frame_map(&_frame, &info, buffer, flags);
    }
    return _mapped;
}

QAbstractVideoBuffer::MapData FrameMapping::map(GstBuffer* buffer, const GstVideoInfo& info)
{
    if (!mapFrame(buffer, info)) {
        return {};
    }
    return mapDataFor(_frame);
}

void FrameMapping::unmap() noexcept
{
    if (_mapped) {
        gst_video_frame_unmap(&_frame);
        _mapped = false;
    }
}

}  // namespace GstHw
