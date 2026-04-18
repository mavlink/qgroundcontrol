#include "GstSampleFrameConverter.h"

#include <QtMultimedia/QAbstractVideoBuffer>
#include <memory>
#include <gst/video/gstvideometa.h>
#include <gst/video/videoorientation.h>

#include "GstVideoBufferFactory.h"
#include "QGCLoggingCategory.h"
#include "VideoFrameDelivery.h"

QGC_LOGGING_CATEGORY(GstSampleFrameConverterLog, "Video.GstSampleFrameConverter")

GstSampleFrameConverter::GstSampleFrameConverter() = default;

GstSampleFrameConverter::~GstSampleFrameConverter()
{
    reset();
}

GstSampleFrameConverter::AllocationInfo GstSampleFrameConverter::allocationInfo() const
{
    QMutexLocker lock(&_mutex);
    return _formatNegotiator.allocationInfo();
}

std::optional<QVideoFrame> GstSampleFrameConverter::convert(GstSample* sample,
                                                            GstAppSink* sink,
                                                            VideoFrameDelivery* delivery)
{
    GstBuffer* buffer = gst_sample_get_buffer(sample);
    if (!buffer || !sink || !delivery)
        return std::nullopt;

    GstVideoInfo videoInfo{};
    QVideoFrameFormat frameFormat;

    {
        QMutexLocker lock(&_mutex);
        if (!_formatNegotiator.negotiate(gst_sample_get_caps(sample), delivery))
            return std::nullopt;

        if (!_formatNegotiator.valid())
            return std::nullopt;

        videoInfo = _formatNegotiator.videoInfo();
        const int width = GST_VIDEO_INFO_WIDTH(&videoInfo);
        const int height = GST_VIDEO_INFO_HEIGHT(&videoInfo);
        if (width <= 0 || height <= 0)
            return std::nullopt;

        frameFormat = _formatNegotiator.frameFormat();
    }

    _latencyTracker.record(buffer, sink, delivery);

    QVideoFrameFormat format = frameFormat;
    GstVideoCropMeta* crop = gst_buffer_get_video_crop_meta(buffer);
    if (crop && crop->width > 0 && crop->height > 0)
        format.setViewport(QRect(crop->x, crop->y, crop->width, crop->height));

    std::unique_ptr<QAbstractVideoBuffer> abBuf =
        GstVideoBufferFactory::create(buffer, videoInfo, format);
    QVideoFrame videoFrame(std::move(abBuf));
    if (!videoFrame.isValid())
        return std::nullopt;

    if (GST_BUFFER_PTS_IS_VALID(buffer)) {
        const qint64 ptsUs = static_cast<qint64>(GST_BUFFER_PTS(buffer) / 1000);
        videoFrame.setStartTime(ptsUs);
        if (GST_BUFFER_DURATION_IS_VALID(buffer))
            videoFrame.setEndTime(ptsUs + static_cast<qint64>(GST_BUFFER_DURATION(buffer) / 1000));
    }

    videoFrame.setRotation(static_cast<QtVideo::Rotation>(_rotation.load(std::memory_order_acquire)));
    videoFrame.setMirrored(_mirrored.load(std::memory_order_acquire));

    return videoFrame;
}

void GstSampleFrameConverter::handleTagEvent(GstEvent* event)
{
    GstTagList* tags = nullptr;
    gst_event_parse_tag(event, &tags);
    if (!tags)
        return;

    GstVideoOrientationMethod method = GST_VIDEO_ORIENTATION_IDENTITY;
    if (!gst_video_orientation_from_tag(tags, &method))
        return;

    bool mirrored = false;
    auto rotation = QtVideo::Rotation::None;
    switch (method) {
        case GST_VIDEO_ORIENTATION_IDENTITY:
            break;
        case GST_VIDEO_ORIENTATION_90R:
            rotation = QtVideo::Rotation::Clockwise90;
            break;
        case GST_VIDEO_ORIENTATION_180:
            rotation = QtVideo::Rotation::Clockwise180;
            break;
        case GST_VIDEO_ORIENTATION_90L:
            rotation = QtVideo::Rotation::Clockwise270;
            break;
        case GST_VIDEO_ORIENTATION_HORIZ:
            mirrored = true;
            break;
        case GST_VIDEO_ORIENTATION_VERT:
            mirrored = true;
            rotation = QtVideo::Rotation::Clockwise180;
            break;
        case GST_VIDEO_ORIENTATION_UL_LR:
            mirrored = true;
            rotation = QtVideo::Rotation::Clockwise90;
            break;
        case GST_VIDEO_ORIENTATION_UR_LL:
            mirrored = true;
            rotation = QtVideo::Rotation::Clockwise270;
            break;
        case GST_VIDEO_ORIENTATION_AUTO:
        case GST_VIDEO_ORIENTATION_CUSTOM:
        default:
            return;
    }

    _mirrored.store(mirrored, std::memory_order_release);
    _rotation.store(static_cast<int>(rotation), std::memory_order_release);

    qCDebug(GstSampleFrameConverterLog) << "Image orientation method:" << method;
}

void GstSampleFrameConverter::invalidateClock()
{
    _latencyTracker.invalidateClock();
}

void GstSampleFrameConverter::reset()
{
    QMutexLocker lock(&_mutex);
    _formatNegotiator.reset();
    _mirrored.store(false, std::memory_order_release);
    _rotation.store(static_cast<int>(QtVideo::Rotation::None), std::memory_order_release);
    _latencyTracker.reset();
}
