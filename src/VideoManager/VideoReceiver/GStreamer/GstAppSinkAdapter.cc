#include "GstAppSinkAdapter.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QMetaObject>
#include <QtMultimedia/QVideoFrame>
#include <QtMultimedia/QVideoFrameFormat>
#include <QtMultimedia/QVideoSink>

#include <gst/app/gstappsink.h>
#include <gst/video/video-info.h>

QGC_LOGGING_CATEGORY(GstAppSinkAdapterLog, "Video.GstAppSinkAdapter")

GstAppSinkAdapter::GstAppSinkAdapter(QObject *parent)
    : QObject(parent)
{
}

GstAppSinkAdapter::~GstAppSinkAdapter()
{
    teardown();
}

bool GstAppSinkAdapter::setup(GstElement *sinkBin, QVideoSink *videoSink)
{
    if (!sinkBin || !videoSink) {
        qCWarning(GstAppSinkAdapterLog) << "setup() called with null arguments";
        return false;
    }

    teardown();

    if (!GST_IS_BIN(sinkBin)) {
        qCWarning(GstAppSinkAdapterLog) << "sinkBin is not a GstBin";
        return false;
    }

    _appsink = gst_bin_get_by_name(GST_BIN(sinkBin), "qgcappsink");
    if (!_appsink) {
        qCWarning(GstAppSinkAdapterLog) << "Could not find 'qgcappsink' in sink bin";
        return false;
    }

    _videoSink = videoSink;
    _signalId = g_signal_connect(_appsink, "new-sample", G_CALLBACK(onNewSample), this);
    qCDebug(GstAppSinkAdapterLog) << "Connected to appsink, signal id:" << _signalId;
    return true;
}

void GstAppSinkAdapter::teardown()
{
    if (_appsink && _signalId) {
        g_signal_handler_disconnect(_appsink, _signalId);
        _signalId = 0;
    }
    gst_clear_object(&_appsink);
    _videoSink = nullptr;
}

GstFlowReturn GstAppSinkAdapter::onNewSample(GstElement *appsink, gpointer userData)
{
    auto *self = static_cast<GstAppSinkAdapter *>(userData);

    GstSample *sample = gst_app_sink_pull_sample(GST_APP_SINK(appsink));
    if (!sample) {
        return GST_FLOW_ERROR;
    }

    GstBuffer *buffer = gst_sample_get_buffer(sample);
    GstCaps *caps = gst_sample_get_caps(sample);
    if (!buffer || !caps) {
        gst_sample_unref(sample);
        return GST_FLOW_ERROR;
    }

    GstVideoInfo videoInfo;
    if (!gst_video_info_from_caps(&videoInfo, caps)) {
        qCWarning(GstAppSinkAdapterLog) << "Failed to parse video info from caps";
        gst_sample_unref(sample);
        return GST_FLOW_ERROR;
    }

    if (GST_VIDEO_INFO_FORMAT(&videoInfo) != GST_VIDEO_FORMAT_BGRA) {
        qCWarning(GstAppSinkAdapterLog) << "Unexpected video format (expected BGRA)";
        gst_sample_unref(sample);
        return GST_FLOW_ERROR;
    }

    const int width = GST_VIDEO_INFO_WIDTH(&videoInfo);
    const int height = GST_VIDEO_INFO_HEIGHT(&videoInfo);
    if (width <= 0 || height <= 0) {
        gst_sample_unref(sample);
        return GST_FLOW_ERROR;
    }

    GstMapInfo mapInfo;
    if (!gst_buffer_map(buffer, &mapInfo, GST_MAP_READ)) {
        qCWarning(GstAppSinkAdapterLog) << "Failed to map GStreamer buffer";
        gst_sample_unref(sample);
        return GST_FLOW_ERROR;
    }

    const QSize frameSize(width, height);
    const QVideoFrameFormat format(frameSize, QVideoFrameFormat::Format_BGRA8888);
    QVideoFrame videoFrame(format);

    if (!videoFrame.map(QVideoFrame::WriteOnly)) {
        qCWarning(GstAppSinkAdapterLog) << "Failed to map QVideoFrame for writing";
        gst_buffer_unmap(buffer, &mapInfo);
        gst_sample_unref(sample);
        return GST_FLOW_ERROR;
    }

    const int dstStride = videoFrame.bytesPerLine(0);
    const int srcStride = GST_VIDEO_INFO_PLANE_STRIDE(&videoInfo, 0);
    const uchar *src = mapInfo.data;
    uchar *dst = videoFrame.bits(0);

    const int rowBytes = width * 4; // BGRA = 4 bytes per pixel
    if (rowBytes > srcStride || rowBytes > dstStride) {
        qCWarning(GstAppSinkAdapterLog) << "Stride smaller than row size:"
                                        << "rowBytes" << rowBytes
                                        << "srcStride" << srcStride
                                        << "dstStride" << dstStride;
        videoFrame.unmap();
        gst_buffer_unmap(buffer, &mapInfo);
        gst_sample_unref(sample);
        return GST_FLOW_ERROR;
    }

    const gsize requiredSize = static_cast<gsize>(height - 1) * srcStride + rowBytes;
    if (mapInfo.size < requiredSize) {
        qCWarning(GstAppSinkAdapterLog) << "Buffer too small:" << mapInfo.size << "<" << requiredSize;
        videoFrame.unmap();
        gst_buffer_unmap(buffer, &mapInfo);
        gst_sample_unref(sample);
        return GST_FLOW_ERROR;
    }

    if (srcStride == dstStride) {
        memcpy(dst, src, static_cast<size_t>(height) * srcStride);
    } else {
        for (int y = 0; y < height; ++y) {
            memcpy(dst + y * dstStride, src + y * srcStride, rowBytes);
        }
    }

    videoFrame.unmap();
    gst_buffer_unmap(buffer, &mapInfo);
    gst_sample_unref(sample);

    // Dispatch to the QVideoSink's owning thread — onNewSample runs on a
    // GStreamer streaming thread, but QVideoSink is a QObject bound to the
    // main/Qt thread.
    if (self->_videoSink) {
        QMetaObject::invokeMethod(self->_videoSink, [sink = self->_videoSink, frame = std::move(videoFrame)]() {
            sink->setVideoFrame(frame);
        }, Qt::QueuedConnection);
    }

    return GST_FLOW_OK;
}
