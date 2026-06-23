#include "GstHwVideoBuffer.h"

GstHwVideoBuffer::GstHwVideoBuffer(QVideoFrame::HandleType handleType,
                                   GstSample *sample,
                                   const GstVideoInfo &videoInfo,
                                   QVideoFrameFormat format)
    : QHwVideoBuffer(handleType, nullptr)
    , _sample(sample ? gst_sample_ref(sample) : nullptr)
    , _videoInfo(videoInfo)
    , _format(std::move(format))
{
    // Crop is applied by the renderer via QVideoFrameFormat::viewport(); see GstAppSinkAdapter::applyCropMeta.
}

GstHwVideoBuffer::~GstHwVideoBuffer()
{
    if (_sample) {
        gst_sample_unref(_sample);
    }
}
