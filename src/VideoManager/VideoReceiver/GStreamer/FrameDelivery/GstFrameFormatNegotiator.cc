#include "GstFrameFormatNegotiator.h"

#include "GstFormatTable.h"
#include "QGCLoggingCategory.h"
#include "VideoFrameDelivery.h"

#ifdef QGC_GST_DMABUF
#include <gst/video/video-info-dma.h>
#endif

QGC_LOGGING_CATEGORY(GstFrameFormatNegotiatorLog, "Video.GstFrameFormatNegotiator")

GstFrameFormatNegotiator::GstFrameFormatNegotiator()
{
    gst_video_info_init(&_videoInfo);
}

GstFrameFormatNegotiator::~GstFrameFormatNegotiator()
{
    reset();
}

bool GstFrameFormatNegotiator::negotiate(GstCaps* caps, VideoFrameDelivery* delivery)
{
    if (!caps)
        return _videoInfoValid;

    if (caps == _negotiatedCaps || (_negotiatedCaps && gst_caps_is_equal(caps, _negotiatedCaps)))
        return true;

    bool parsed = false;
#ifdef QGC_GST_DMABUF
    if (gst_video_is_dma_drm_caps(caps)) {
        GstVideoInfoDmaDrm drmInfo;
        gst_video_info_dma_drm_init(&drmInfo);
        if (gst_video_info_dma_drm_from_caps(&drmInfo, caps) &&
            gst_video_info_dma_drm_to_video_info(&drmInfo, &_videoInfo)) {
            parsed = true;
        } else {
            qCWarning(GstFrameFormatNegotiatorLog)
                << "DMA_DRM caps parsed but could not convert to GstVideoInfo"
                << "(non-linear modifier leaked past caps filter?)";
        }
    }
#endif
    if (!parsed && !gst_video_info_from_caps(&_videoInfo, caps)) {
        qCWarning(GstFrameFormatNegotiatorLog) << "Failed to parse video info from caps";
        _videoInfoValid = false;
        return false;
    }

    _videoInfoValid = true;
    gst_caps_replace(&_negotiatedCaps, caps);

    const int width = GST_VIDEO_INFO_WIDTH(&_videoInfo);
    const int height = GST_VIDEO_INFO_HEIGHT(&_videoInfo);
    qCDebug(GstFrameFormatNegotiatorLog)
        << "Caps negotiated:" << gst_video_format_to_string(GST_VIDEO_INFO_FORMAT(&_videoInfo)) << width << "x" << height;

    const auto qtFmt = GstFormatTable::gstFormatToQt(GST_VIDEO_INFO_FORMAT(&_videoInfo));
    if (qtFmt == QVideoFrameFormat::Format_Invalid) {
        _videoInfoValid = false;
        return false;
    }

    _cachedFrameFormat = QVideoFrameFormat(QSize(width, height), qtFmt);
    GstFormatTable::applyColorimetry(_cachedFrameFormat, _videoInfo);
    GstFormatTable::applyHdrMetadata(_cachedFrameFormat, caps);

    const gint fpsN = GST_VIDEO_INFO_FPS_N(&_videoInfo);
    const gint fpsD = GST_VIDEO_INFO_FPS_D(&_videoInfo);
    if (fpsN > 0 && fpsD > 0)
        _cachedFrameFormat.setStreamFrameRate(static_cast<qreal>(fpsN) / fpsD);

    if (GstFormatTable::isHdrContent(_videoInfo) && _cachedFrameFormat.maxLuminance() <= 0.0F)
        _cachedFrameFormat.setMaxLuminance(1000.0F);

    if (delivery)
        delivery->announceFormat(_cachedFrameFormat);
    return true;
}

GstFrameFormatNegotiator::AllocationInfo GstFrameFormatNegotiator::allocationInfo() const
{
    if (!_videoInfoValid)
        return {};

    return {
        true,
        GST_VIDEO_INFO_SIZE(&_videoInfo),
        GST_VIDEO_INFO_FPS_N(&_videoInfo),
        GST_VIDEO_INFO_FPS_D(&_videoInfo),
    };
}

void GstFrameFormatNegotiator::reset()
{
    gst_caps_replace(&_negotiatedCaps, nullptr);
    _videoInfoValid = false;
    _cachedFrameFormat = {};
}
