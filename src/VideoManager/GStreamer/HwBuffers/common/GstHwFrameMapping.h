#pragma once

#include <gst/gst.h>
#include <gst/video/video-frame.h>
#include <gst/video/video-info.h>

#include <QtMultimedia/QAbstractVideoBuffer>

namespace GstHw {

/// A buffer's gst_video_frame_map, held until unmap() or destruction: the CPU view behind a buffer's
/// QAbstractVideoBuffer::map(), or any other scoped frame map.
class FrameMapping
{
public:
    FrameMapping() = default;

    ~FrameMapping() { unmap(); }

    FrameMapping(const FrameMapping&) = delete;
    FrameMapping& operator=(const FrameMapping&) = delete;

    /// Maps @p buffer with @p flags unless a map is already held; false when that fails.
    bool mapFrame(GstBuffer* buffer, const GstVideoInfo& info, GstMapFlags flags = GST_MAP_READ);

    /// The mapped frame; valid while mapFrame() succeeded and unmap() hasn't run.
    const GstVideoFrame& frame() const { return _frame; }

    /// mapFrame() for reading, as Qt map data; empty when the map fails.
    QAbstractVideoBuffer::MapData map(GstBuffer* buffer, const GstVideoInfo& info);
    void unmap() noexcept;

private:
    // A C struct, zeroed: its flags have no enumerator for none.
    GstVideoFrame _frame = {};  // NOLINT(bugprone-invalid-enum-default-initialization)
    bool _mapped = false;
};

}  // namespace GstHw
