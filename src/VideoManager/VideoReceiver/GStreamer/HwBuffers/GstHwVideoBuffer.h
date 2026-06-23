#pragma once

#include <QtCore/qglobal.h>
#include <QtMultimedia/QVideoFrame>
#include <QtMultimedia/QVideoFrameFormat>
#include <private/qhwvideobuffer_p.h>

#include <gst/gst.h>
#include <gst/video/video-info.h>

/// \brief Common base for GStreamer-backed QHwVideoBuffer subclasses.
///
/// Owns a ref on GstSample for its lifetime; holds a GstVideoInfo and
/// QVideoFrameFormat so subclasses don't duplicate those three members.
///
class GstHwVideoBuffer : public QHwVideoBuffer
{
public:
    GstHwVideoBuffer(QVideoFrame::HandleType handleType,
                     GstSample *sample,
                     const GstVideoInfo &videoInfo,
                     QVideoFrameFormat format);
    ~GstHwVideoBuffer() override;

    QVideoFrameFormat format() const override { return _format; }

    /// Streaming-thread sanity check on per-plane handles. Failure routes to CPU memcpy.
    /// GPU texture validity is checked later in mapTextures.
    virtual bool validatePlaneHandles() const { return true; }

protected:
    GstSample *_sample = nullptr;
    GstVideoInfo _videoInfo{};
    QVideoFrameFormat _format;
};
