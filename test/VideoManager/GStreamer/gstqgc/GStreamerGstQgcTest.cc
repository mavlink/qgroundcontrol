#include "GStreamerTest.h"

#ifdef QGC_GST_STREAMING

#include "CpuVideoFramePool.h"
#include "Fact.h"
#include "Fixtures/RAIIFixtures.h"
#include "GStreamer.h"
#include "GStreamerHelpers.h"
#include "GStreamerLogging.h"
#include "GstHwPathTelemetry.h"
#include "GstHwVideoBufferFactory.h"
#include "GstQgcFrameMap.h"
#include "GstSourceFactory.h"
#include "GstVideoReceiver.h"
#include "HwBuffers/dmabuf/GstDmaDrmCaps.h"
#include "QGCQVideoSinkController.h"
#include "gstqgc/GstQgcAllocation.h"
#include "gstqgc/GstQgcBinChain.h"
#include "gstqgc/GstQgcCaps.h"
#include "gstqgc/GstQgcVideoFormats.h"

#if defined(QGC_HAS_GST_DMABUF_GPU_PATH)
#include "GstDmaBufVideoBuffer.h"
#endif
#if defined(QGC_HAS_GST_GLMEMORY_GPU_PATH)
#include <gst/gl/gl.h>

#include "GstGlContextBridge.h"
#endif
#if defined(QGC_HAS_GST_D3D11_GPU_PATH)
#include "GstD3D11ContextBridge.h"
#endif
#if defined(QGC_HAS_GST_D3D12_GPU_PATH)
#include "GstD3D12ContextBridge.h"
#endif
#include "GstHwVideoBuffer.h"
#include "gstqgc/gstqgcvideosinkbin.h"
#if defined(QGC_HAS_ANY_GPU_PATH)
#include "QGCRhiCapture.h"
#endif
#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QElapsedTimer>
#include <QtCore/QEventLoop>
#include <QtCore/QFileInfo>
#include <QtCore/QRegularExpression>
#include <QtCore/QScopeGuard>
#include <QtCore/QStandardPaths>
#include <QtGui/QColor>
#include <QtGui/QImage>
#include <QtMultimedia/QVideoFrame>
#include <QtMultimedia/QVideoSink>
#include <QtMultimediaQuick/private/qquickvideooutput_p.h>
#include <QtQuick/QQuickWindow>
#include <QtTest/QSignalSpy>
#include <atomic>
#include <cstring>
#include <gst/gst.h>
#include <gst/video/gstvideometa.h>
#include <iterator>
#include <memory>
#include <string_view>
#include <vector>
#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
#include <gst/allocators/gstfdmemory.h>
#include <sys/mman.h>
#include <unistd.h>
#endif

namespace {

guint64 framesDelivered(GstElement* qvideosink)
{
    guint64 delivered = 0;
    g_object_get(qvideosink, "frames-delivered", &delivered, nullptr);
    return delivered;
}

/// A guint64 field of the sink's "stats" structure; -1 if the field is missing.
qint64 sinkStat(GstElement* qvideosink, const char* field)
{
    GstStructure* stats = nullptr;
    g_object_get(qvideosink, "stats", &stats, nullptr);
    guint64 value = 0;
    const bool found = stats && gst_structure_get_uint64(stats, field, &value);
    if (stats) {
        gst_structure_free(stats);
    }
    return found ? static_cast<qint64>(value) : -1;
}

/// Drives a standalone qgcqvideosink from the test thread through a linked source pad. show_frame runs inside
/// pushFrame(), so element counters are exact, and GUI-thread delivery happens only when the test pumps events.
class QgcQVideoSinkHarness
{
public:
    static constexpr int kWidth = 4;
    static constexpr int kHeight = 4;
    static constexpr gsize kFrameBytes = kWidth * kHeight * 4;

    explicit QgcQVideoSinkHarness(QVideoSink* target, bool gpuZeroCopy = false)
        : _sink(gst_element_factory_make_full("qgcqvideosink", "gpu-zerocopy", gpuZeroCopy, nullptr))
        , _bus(gst_bus_new())
        , _src(gst_pad_new("src", GST_PAD_SRC))
    {
        if (!_sink) {
            return;
        }
        gst_object_ref_sink(_sink);
        gst_element_set_bus(_sink, _bus);
        g_object_set(_sink, "qvideosink", static_cast<gpointer>(target), nullptr);
        GstPad* sinkPad = gst_element_get_static_pad(_sink, "sink");
        _ready = sinkPad && (gst_pad_link(_src, sinkPad) == GST_PAD_LINK_OK) && gst_pad_set_active(_src, TRUE) &&
                 (gst_element_set_state(_sink, GST_STATE_PLAYING) != GST_STATE_CHANGE_FAILURE);
        if (sinkPad) {
            gst_object_unref(sinkPad);
        }
    }

    ~QgcQVideoSinkHarness()
    {
        if (_sink) {
            gst_element_set_state(_sink, GST_STATE_NULL);
        }
        gst_pad_set_active(_src, FALSE);
        gst_object_unref(_src);
        gst_clear_object(&_sink);
        gst_object_unref(_bus);
    }

    QgcQVideoSinkHarness(const QgcQVideoSinkHarness&) = delete;
    QgcQVideoSinkHarness& operator=(const QgcQVideoSinkHarness&) = delete;

    bool isReady() const { return _ready; }

    GstElement* element() const { return _sink; }

    GstBus* bus() const { return _bus; }

    /// stream-start, BGRA caps (in @p memoryFeature, if given) and a TIME segment, as an upstream element sends before
    /// its first buffer.
    bool start(const char* memoryFeature = nullptr)
    {
        GstCaps* caps =
            gst_caps_new_simple("video/x-raw", "format", G_TYPE_STRING, "BGRA", "width", G_TYPE_INT, kWidth, "height",
                                G_TYPE_INT, kHeight, "framerate", GST_TYPE_FRACTION, 30, 1, nullptr);
        if (memoryFeature) {
            gst_caps_set_features(caps, 0, gst_caps_features_new(memoryFeature, nullptr));
        }
        const bool ok = pushEvent(gst_event_new_stream_start("qgc-harness")) && pushEvent(gst_event_new_caps(caps)) &&
                        pushSegment();
        gst_caps_unref(caps);
        return ok;
    }

    bool pushSegment()
    {
        GstSegment segment;
        gst_segment_init(&segment, GST_FORMAT_TIME);
        return pushEvent(gst_event_new_segment(&segment));
    }

    bool pushEvent(GstEvent* event) { return gst_pad_push_event(_src, event); }

    /// @p size below kFrameBytes produces a buffer gst_video_frame_map rejects.
    static GstBuffer* makeFrame(GstClockTime pts, gsize size = kFrameBytes)
    {
        GstBuffer* buffer = gst_buffer_new_allocate(nullptr, size, nullptr);
        gst_buffer_memset(buffer, 0, 0x80, size);
        GST_BUFFER_PTS(buffer) = pts;
        return buffer;
    }

    GstFlowReturn pushFrame(GstClockTime pts, bool discont = false, gsize size = kFrameBytes)
    {
        GstBuffer* buffer = makeFrame(pts, size);
        if (discont) {
            GST_BUFFER_FLAG_SET(buffer, GST_BUFFER_FLAG_DISCONT);
        }
        return pushBuffer(buffer);
    }

    GstFlowReturn pushBuffer(GstBuffer* buffer) { return gst_pad_push(_src, buffer); }

#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
    /// A frame in memfd-backed fd memory: not system memory, and no GPU path imports it.
    static GstBuffer* makeFdFrame(GstClockTime pts)
    {
        const int fd = memfd_create("qgc-sink-harness", MFD_CLOEXEC);
        if (fd < 0) {
            return nullptr;
        }
        if (ftruncate(fd, kFrameBytes) != 0) {
            close(fd);
            return nullptr;
        }
        GstAllocator* allocator = gst_fd_allocator_new();
        GstMemory* memory = gst_fd_allocator_alloc(allocator, fd, kFrameBytes, GST_FD_MEMORY_FLAG_NONE);
        gst_object_unref(allocator);
        GstBuffer* buffer = gst_buffer_new();
        gst_buffer_append_memory(buffer, memory);
        gst_buffer_memset(buffer, 0, 0x80, kFrameBytes);
        GST_BUFFER_PTS(buffer) = pts;
        return buffer;
    }
#endif

    guint64 delivered() const { return framesDelivered(_sink); }

private:
    GstElement* _sink = nullptr;
    GstBus* _bus = nullptr;
    GstPad* _src = nullptr;
    bool _ready = false;
};

}  // namespace
void GStreamerTest::_testAppsinkFrameDelivery()
{
    // Ensure the qgc plugin (including qgcvideosinkbin) is registered.
    // _testCompleteInit runs before this slot, but guard against reorder.
    GstElementFactory* guardFactory = gst_element_factory_find("qgcvideosinkbin");
    if (!guardFactory) {
        GStreamer::completeInit();
    } else {
        gst_object_unref(guardFactory);
    }

    GstElementFactory* factory = gst_element_factory_find("qgcvideosinkbin");
    QVERIFY2(factory, "qgcvideosinkbin factory not found");
    gst_object_unref(factory);

    GError* error = nullptr;
    GstElement* pipeline = gst_parse_launch(
        "videotestsrc num-buffers=10 ! "
        "video/x-raw,format=I420,width=320,height=240,framerate=30/1 ! "
        "videoconvert ! "
        "video/x-raw,format=BGRA ! "
        "qgcvideosinkbin name=sink",
        &error);
    if (error) {
        const QString msg = QString::fromUtf8(error->message);
        g_clear_error(&error);
        QFAIL(qPrintable(QStringLiteral("Pipeline parse error: %1").arg(msg)));
    }
    QVERIFY2(pipeline, "Failed to create appsink test pipeline");

    GstElement* sinkBin = gst_bin_get_by_name(GST_BIN(pipeline), "sink");
    QVERIFY2(sinkBin, "Could not find 'sink' element in pipeline");

    QVideoSink videoSink;
    QObject controllerOwner;

    int frameCount = 0;
    QSize lastFrameSize;
    QObject::connect(&videoSink, &QVideoSink::videoFrameChanged, &controllerOwner, [&](const QVideoFrame& frame) {
        frameCount++;
        lastFrameSize = frame.size();
    });

    QVERIFY2(GStreamer::setupQVideoSinkElement(sinkBin, &videoSink, &controllerOwner),
             "GStreamer::setupQVideoSinkElement() failed");
    auto* controller = controllerOwner.findChild<QGCQVideoSinkController*>(QString(), Qt::FindDirectChildrenOnly);
    QVERIFY(controller);

    gst_object_unref(sinkBin);

    GstStateChangeReturn ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
    QVERIFY2(ret != GST_STATE_CHANGE_FAILURE, "Pipeline failed to transition to PLAYING");

    GstBus* bus = gst_element_get_bus(pipeline);
    QVERIFY(bus);

    GstMessage* msg = gst_bus_timed_pop_filtered(bus, 10 * GST_SECOND,
                                                 static_cast<GstMessageType>(GST_MESSAGE_EOS | GST_MESSAGE_ERROR));
    QVERIFY2(msg, "Pipeline timed out waiting for EOS or ERROR");

    if (GST_MESSAGE_TYPE(msg) == GST_MESSAGE_ERROR) {
        GError* err = nullptr;
        gchar* debug = nullptr;
        gst_message_parse_error(msg, &err, &debug);
        const QString errMsg = QStringLiteral("%1 (%2)")
                                   .arg(err ? QString::fromUtf8(err->message) : QStringLiteral("unknown"))
                                   .arg(debug ? QString::fromUtf8(debug) : QString());
        g_clear_error(&err);
        g_free(debug);
        gst_message_unref(msg);
        gst_object_unref(bus);
        gst_element_set_state(pipeline, GST_STATE_NULL);
        gst_object_unref(pipeline);
        QFAIL(qPrintable(QStringLiteral("Pipeline error: %1").arg(errMsg)));
    }

    QCOMPARE(GST_MESSAGE_TYPE(msg), GST_MESSAGE_EOS);
    gst_message_unref(msg);
    gst_object_unref(bus);

    QTRY_VERIFY_WITH_TIMEOUT(frameCount > 0, TestTimeout::mediumMs());
    QCOMPARE(lastFrameSize, QSize(320, 240));

    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
}

void GStreamerTest::_testAppsinkYuvPassthrough()
{
    GstElementFactory* guardFactory = gst_element_factory_find("qgcvideosinkbin");
    if (!guardFactory) {
        GStreamer::completeInit();
    } else {
        gst_object_unref(guardFactory);
    }

    GError* error = nullptr;
    GstElement* pipeline = gst_parse_launch(
        "videotestsrc num-buffers=10 ! "
        "video/x-raw,format=I420,width=320,height=240,framerate=30/1 ! "
        "qgcvideosinkbin name=sink",
        &error);
    if (error) {
        const QString msg = QString::fromUtf8(error->message);
        g_clear_error(&error);
        QFAIL(qPrintable(QStringLiteral("Pipeline parse error: %1").arg(msg)));
    }
    QVERIFY2(pipeline, "Failed to create YUV passthrough pipeline");

    GstElement* sinkBin = gst_bin_get_by_name(GST_BIN(pipeline), "sink");
    QVERIFY2(sinkBin, "Could not find 'sink' element");

    QVideoSink videoSink;
    QObject controllerOwner;

    int frameCount = 0;
    QVideoFrameFormat::PixelFormat lastPixelFormat = QVideoFrameFormat::Format_Invalid;
    QObject::connect(&videoSink, &QVideoSink::videoFrameChanged, &controllerOwner, [&](const QVideoFrame& frame) {
        frameCount++;
        lastPixelFormat = frame.pixelFormat();
    });

    QVERIFY2(GStreamer::setupQVideoSinkElement(sinkBin, &videoSink, &controllerOwner),
             "GStreamer::setupQVideoSinkElement() failed");
    gst_object_unref(sinkBin);

    QVERIFY2(gst_element_set_state(pipeline, GST_STATE_PLAYING) != GST_STATE_CHANGE_FAILURE, "Pipeline failed to PLAY");

    GstBus* bus = gst_element_get_bus(pipeline);
    GstMessage* msg = gst_bus_timed_pop_filtered(bus, 10 * GST_SECOND,
                                                 static_cast<GstMessageType>(GST_MESSAGE_EOS | GST_MESSAGE_ERROR));
    QVERIFY2(msg, "Pipeline timed out");
    if (GST_MESSAGE_TYPE(msg) == GST_MESSAGE_ERROR) {
        gst_message_unref(msg);
        gst_object_unref(bus);
        gst_element_set_state(pipeline, GST_STATE_NULL);
        gst_object_unref(pipeline);
        QFAIL("YUV passthrough pipeline errored");
    }
    QCOMPARE(GST_MESSAGE_TYPE(msg), GST_MESSAGE_EOS);
    gst_message_unref(msg);
    gst_object_unref(bus);

    QTRY_VERIFY_WITH_TIMEOUT(frameCount > 0, TestTimeout::mediumMs());
    QCOMPARE(lastPixelFormat, QVideoFrameFormat::Format_YUV420P);

    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
}

void GStreamerTest::_testAppsinkPtsAndColorimetry()
{
    GstElementFactory* guardFactory = gst_element_factory_find("qgcvideosinkbin");
    if (!guardFactory) {
        GStreamer::completeInit();
    } else {
        gst_object_unref(guardFactory);
    }

    GError* error = nullptr;
    GstElement* pipeline = gst_parse_launch(
        "videotestsrc num-buffers=5 do-timestamp=true ! "
        "video/x-raw,format=I420,width=64,height=48,framerate=30/1,"
        "colorimetry=(string)bt709 ! "
        "qgcvideosinkbin name=sink",
        &error);
    if (error) {
        const QString msg = QString::fromUtf8(error->message);
        g_clear_error(&error);
        QFAIL(qPrintable(QStringLiteral("Pipeline parse error: %1").arg(msg)));
    }
    QVERIFY2(pipeline, "Failed to create PTS/colorimetry pipeline");

    GstElement* sinkBin = gst_bin_get_by_name(GST_BIN(pipeline), "sink");
    QVERIFY2(sinkBin, "Could not find 'sink' element");

    QVideoSink videoSink;
    QObject controllerOwner;

    int frameCount = 0;
    qint64 lastStartTime = -1;
    QVideoFrameFormat::ColorSpace lastColorSpace = QVideoFrameFormat::ColorSpace_Undefined;
    QObject::connect(&videoSink, &QVideoSink::videoFrameChanged, &controllerOwner, [&](const QVideoFrame& frame) {
        frameCount++;
        lastStartTime = frame.startTime();
        lastColorSpace = frame.surfaceFormat().colorSpace();
    });

    QVERIFY2(GStreamer::setupQVideoSinkElement(sinkBin, &videoSink, &controllerOwner),
             "GStreamer::setupQVideoSinkElement() failed");
    gst_object_unref(sinkBin);

    QVERIFY2(gst_element_set_state(pipeline, GST_STATE_PLAYING) != GST_STATE_CHANGE_FAILURE, "Pipeline failed to PLAY");

    GstBus* bus = gst_element_get_bus(pipeline);
    GstMessage* msg = gst_bus_timed_pop_filtered(bus, 10 * GST_SECOND,
                                                 static_cast<GstMessageType>(GST_MESSAGE_EOS | GST_MESSAGE_ERROR));
    QVERIFY2(msg, "Pipeline timed out");
    if (GST_MESSAGE_TYPE(msg) == GST_MESSAGE_ERROR) {
        gst_message_unref(msg);
        gst_object_unref(bus);
        gst_element_set_state(pipeline, GST_STATE_NULL);
        gst_object_unref(pipeline);
        QFAIL("PTS/colorimetry pipeline errored");
    }
    QCOMPARE(GST_MESSAGE_TYPE(msg), GST_MESSAGE_EOS);
    gst_message_unref(msg);
    gst_object_unref(bus);

    QTRY_VERIFY_WITH_TIMEOUT(frameCount > 0, TestTimeout::mediumMs());
    QVERIFY2(lastStartTime >= 0, "GstBuffer PTS not forwarded to QVideoFrame::startTime");
    QCOMPARE(lastColorSpace, QVideoFrameFormat::ColorSpace_BT709);

    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
}

void GStreamerTest::_testQgcVideoSinkBinGpuZeroCopyProperty()
{
    GStreamer::redirectGLibLogging();
    QVERIFY2(GStreamer::completeInit(), "GStreamer::completeInit() failed");

    GstElementFactory* factory = gst_element_factory_find("qgcvideosinkbin");
    QVERIFY2(factory, "qgcvideosinkbin factory not found");

    {
        GstElement* bin = gst_element_factory_create_full(factory, "gpu-zerocopy", FALSE, NULL);
        QVERIFY2(bin, "Failed to create qgcvideosinkbin (CPU branch)");

        gboolean prop = TRUE;
        g_object_get(bin, "gpu-zerocopy", &prop, NULL);
        QCOMPARE(prop, FALSE);

        GstElement* qvideosink = gst_bin_get_by_name(GST_BIN(bin), "qgcqvideosink");
        QVERIFY2(qvideosink, "qgcqvideosink missing from CPU bin");

        gboolean sync = TRUE;
        gboolean qos = TRUE;
        guint64 processingDeadline = 0;
        g_object_get(bin, "sync", &sync, "qos", &qos, "processing-deadline", &processingDeadline, NULL);
        QCOMPARE(sync, FALSE);
        QCOMPARE(qos, FALSE);
        QCOMPARE(processingDeadline, G_GUINT64_CONSTANT(20000000));

        g_object_get(qvideosink, "sync", &sync, "qos", &qos, "processing-deadline", &processingDeadline, NULL);
        QCOMPARE(sync, FALSE);
        QCOMPARE(qos, FALSE);
        QCOMPARE(processingDeadline, G_GUINT64_CONSTANT(20000000));

        constexpr guint64 kUpdatedDeadline = G_GUINT64_CONSTANT(1234567);
        g_object_set(bin, "sync", TRUE, "qos", TRUE, "processing-deadline", kUpdatedDeadline, NULL);

        g_object_get(bin, "sync", &sync, "qos", &qos, "processing-deadline", &processingDeadline, NULL);
        QCOMPARE(sync, TRUE);
        QCOMPARE(qos, TRUE);
        QCOMPARE(processingDeadline, kUpdatedDeadline);

        g_object_get(qvideosink, "sync", &sync, "qos", &qos, "processing-deadline", &processingDeadline, NULL);
        QCOMPARE(sync, TRUE);
        QCOMPARE(qos, TRUE);
        QCOMPARE(processingDeadline, kUpdatedDeadline);

        GstIterator* it = gst_bin_iterate_elements(GST_BIN(bin));
        int elementCount = 0;
        bool sawVideoconvert = false;
        gboolean done = FALSE;
        GValue val = G_VALUE_INIT;
        while (!done) {
            switch (gst_iterator_next(it, &val)) {
                case GST_ITERATOR_OK: {
                    ++elementCount;
                    GstElement* child = GST_ELEMENT(g_value_get_object(&val));
                    gchar* name = gst_element_get_name(child);
                    if (name && QString::fromUtf8(name).startsWith(QStringLiteral("videoconvert"))) {
                        sawVideoconvert = true;
                    }
                    g_free(name);
                    g_value_reset(&val);
                    break;
                }
                case GST_ITERATOR_RESYNC:
                    gst_iterator_resync(it);
                    break;
                case GST_ITERATOR_ERROR:
                case GST_ITERATOR_DONE:
                    done = TRUE;
                    break;
            }
        }
        g_value_unset(&val);
        gst_iterator_free(it);

        // CPU branch: videoconvert + PAR=1/1 capsfilter + format capsfilter + qgcqvideosink (4 children).
        QCOMPARE(elementCount, 4);
        QVERIFY2(sawVideoconvert, "CPU bin missing videoconvert");

        gst_object_unref(qvideosink);
        gst_object_unref(bin);
    }

    {
        // disable-par=TRUE drops the PAR capsfilter (videoconvert + format capsfilter + qgcqvideosink).
        GstElement* bin = gst_element_factory_create_full(factory, "gpu-zerocopy", FALSE, "disable-par", TRUE, NULL);
        QVERIFY2(bin, "Failed to create qgcvideosinkbin (CPU branch, disable-par=TRUE)");
        GstIterator* it = gst_bin_iterate_elements(GST_BIN(bin));
        int elementCount = 0;
        gboolean done = FALSE;
        GValue val = G_VALUE_INIT;
        while (!done) {
            switch (gst_iterator_next(it, &val)) {
                case GST_ITERATOR_OK:
                    ++elementCount;
                    g_value_reset(&val);
                    break;
                case GST_ITERATOR_RESYNC:
                    gst_iterator_resync(it);
                    break;
                case GST_ITERATOR_ERROR:
                case GST_ITERATOR_DONE:
                    done = TRUE;
                    break;
            }
        }
        g_value_unset(&val);
        gst_iterator_free(it);
        QCOMPARE(elementCount, 3);
        gst_object_unref(bin);
    }

#if defined(QGC_HAS_ANY_GPU_PATH)
    {
        GstElement* bin = gst_element_factory_create_full(factory, "gpu-zerocopy", TRUE, NULL);
        QVERIFY2(bin, "Failed to create qgcvideosinkbin (GPU branch)");

        gboolean prop = FALSE;
        g_object_get(bin, "gpu-zerocopy", &prop, NULL);
        QCOMPARE(prop, TRUE);

        GstElement* qvideosink = gst_bin_get_by_name(GST_BIN(bin), "qgcqvideosink");
        QVERIFY2(qvideosink, "qgcqvideosink missing from GPU bin");

        GstIterator* it = gst_bin_iterate_elements(GST_BIN(bin));
        int elementCount = 0;
#if defined(QGC_GST_BIN_USE_GLUPLOAD) || (defined(Q_OS_LINUX) && defined(QGC_GST_BIN_USE_DMABUF))
        bool sawGlupload = false;
        bool sawGlColorConvert = false;
#endif
#if defined(Q_OS_WIN) && (defined(QGC_HAS_GST_D3D11_GPU_PATH) || defined(QGC_HAS_GST_D3D12_GPU_PATH))
        bool sawD3D11Upload = false;
        bool sawD3D11Convert = false;
#endif
        gboolean done = FALSE;
        GValue val = G_VALUE_INIT;
        while (!done) {
            switch (gst_iterator_next(it, &val)) {
                case GST_ITERATOR_OK: {
                    ++elementCount;
                    GstElement* child = GST_ELEMENT(g_value_get_object(&val));
                    gchar* name = gst_element_get_name(child);
#if defined(QGC_GST_BIN_USE_GLUPLOAD) || (defined(Q_OS_LINUX) && defined(QGC_GST_BIN_USE_DMABUF))
                    if (name && QString::fromUtf8(name).startsWith(QStringLiteral("glupload"))) {
                        sawGlupload = true;
                    }
                    if (name && QString::fromUtf8(name).startsWith(QStringLiteral("glcolorconvert"))) {
                        sawGlColorConvert = true;
                    }
#endif
#if defined(Q_OS_WIN) && (defined(QGC_HAS_GST_D3D11_GPU_PATH) || defined(QGC_HAS_GST_D3D12_GPU_PATH))
                    if (name && QString::fromUtf8(name).startsWith(QStringLiteral("d3d11upload"))) {
                        sawD3D11Upload = true;
                    }
                    if (name && QString::fromUtf8(name).startsWith(QStringLiteral("d3d11convert"))) {
                        sawD3D11Convert = true;
                    }
#endif
                    g_free(name);
                    g_value_reset(&val);
                    break;
                }
                case GST_ITERATOR_RESYNC:
                    gst_iterator_resync(it);
                    break;
                case GST_ITERATOR_ERROR:
                case GST_ITERATOR_DONE:
                    done = TRUE;
                    break;
            }
        }
        g_value_unset(&val);
        gst_iterator_free(it);

        GstElement* formatFilter = gst_bin_get_by_name(GST_BIN(bin), "qgc-format-filter");
        QVERIFY2(formatFilter, "GPU bin missing qgc-format-filter");
        GstCaps* caps = nullptr;
        g_object_get(formatFilter, "caps", &caps, NULL);
        QVERIFY2(caps, "GPU bin format capsfilter has null caps");
#if defined(Q_OS_WIN) && (defined(QGC_HAS_GST_D3D11_GPU_PATH) || defined(QGC_HAS_GST_D3D12_GPU_PATH))
        bool hasSystemMemoryCaps = false;
        for (guint i = 0; i < gst_caps_get_size(caps); ++i) {
            const GstCapsFeatures* features = gst_caps_get_features(caps, i);
            if (!features || gst_caps_features_is_equal(features, GST_CAPS_FEATURES_MEMORY_SYSTEM_MEMORY)) {
                hasSystemMemoryCaps = true;
                break;
            }
        }
#endif
        gchar* capsStr = gst_caps_to_string(caps);
        const QString s = QString::fromUtf8(capsStr ? capsStr : "");
        g_free(capsStr);
        gst_caps_unref(caps);
        gst_object_unref(formatFilter);

// Branch on the bin's actual routing selector (QGC_GST_BIN_USE_GLUPLOAD / _DMABUF, set in
// HwBuffers/CMakeLists.txt), not on path-availability macros: on Linux with both DMABuf and GLMemory
// paths compiled, the bin routes through glupload, so keying off DMABUF && GLMEMORY picks the wrong arm.
#if defined(QGC_GST_BIN_USE_GLUPLOAD)
        QCOMPARE(elementCount, 4);
        QVERIFY2(sawGlupload, "GPU bin missing glupload");
        QVERIFY2(sawGlColorConvert, "GPU bin missing glcolorconvert");
        QVERIFY2(s.contains(QStringLiteral("memory:GLMemory")),
                 qUtf8Printable(QStringLiteral("GPU bin caps missing memory:GLMemory: ") + s));
        QVERIFY2(!s.contains(QStringLiteral("NV12")),
                 qUtf8Printable(QStringLiteral("GLMemory caps must force RGB(A), not multi-plane NV12: ") + s));
        QVERIFY2(s.contains(QStringLiteral("RGBA")) || s.contains(QStringLiteral("BGRA")),
                 qUtf8Printable(QStringLiteral("GLMemory caps missing RGB(A) format: ") + s));
#elif defined(Q_OS_LINUX) && defined(QGC_GST_BIN_USE_DMABUF)
        QCOMPARE(elementCount, 2);
        QVERIFY2(!sawGlupload, "Linux DMABuf GPU bin must not insert glupload when direct DMABuf is available");
        QVERIFY2(!sawGlColorConvert,
                 "Linux DMABuf GPU bin must not insert glcolorconvert when direct DMABuf is available");
        QVERIFY2(s.contains(QStringLiteral("memory:DMABuf")),
                 qUtf8Printable(QStringLiteral("GPU bin caps missing memory:DMABuf: ") + s));
        QVERIFY2(!s.contains(QStringLiteral("memory:GLMemory")),
                 qUtf8Printable(QStringLiteral("Direct DMABuf bin caps must not advertise GLMemory first: ") + s));
#elif defined(Q_OS_WIN) && (defined(QGC_HAS_GST_D3D11_GPU_PATH) || defined(QGC_HAS_GST_D3D12_GPU_PATH))
        QCOMPARE(elementCount, 2);
        QVERIFY2(!sawD3D11Upload, "Windows GPU sink must not upload software-decoded frames to D3D11");
        QVERIFY2(!sawD3D11Convert, "Windows GPU sink must not force software-decoded frames through D3D11 conversion");
        QVERIFY2(hasSystemMemoryCaps, "Windows GPU sink must retain a system-memory fallback for software decoders");
#if defined(QGC_HAS_GST_D3D11_GPU_PATH)
        QVERIFY2(s.contains(QStringLiteral("memory:D3D11Memory")),
                 qUtf8Printable(QStringLiteral("GPU bin caps missing memory:D3D11Memory: ") + s));
#endif
#if defined(QGC_HAS_GST_D3D12_GPU_PATH)
        QVERIFY2(s.contains(QStringLiteral("memory:D3D12Memory")),
                 qUtf8Printable(QStringLiteral("GPU bin caps missing memory:D3D12Memory: ") + s));
#endif
#if defined(QGC_HAS_GST_GLMEMORY_GPU_PATH)
        const qsizetype glIndex = s.indexOf(QStringLiteral("memory:GLMemory"));
        if (glIndex >= 0) {
#if defined(QGC_HAS_GST_D3D11_GPU_PATH)
            const qsizetype d3d11Index = s.indexOf(QStringLiteral("memory:D3D11Memory"));
            QVERIFY2(d3d11Index >= 0 && d3d11Index < glIndex,
                     qUtf8Printable(QStringLiteral("D3D11Memory must be advertised before GLMemory: ") + s));
#endif
#if defined(QGC_HAS_GST_D3D12_GPU_PATH)
            const qsizetype d3d12Index = s.indexOf(QStringLiteral("memory:D3D12Memory"));
            QVERIFY2(d3d12Index >= 0 && d3d12Index < glIndex,
                     qUtf8Printable(QStringLiteral("D3D12Memory must be advertised before GLMemory: ") + s));
#endif
        }
#endif
#else
        // Native memory passthrough: format_capsfilter → qgcqvideosink (2 children). Caps advertise
        // whichever GPU memory types are compiled in (e.g. GLMemory on macOS/iOS).
        QCOMPARE(elementCount, 2);
#if defined(QGC_HAS_GST_DMABUF_GPU_PATH)
        QVERIFY2(s.contains(QStringLiteral("memory:DMABuf")),
                 qUtf8Printable(QStringLiteral("GPU bin caps missing memory:DMABuf: ") + s));
#endif
#if defined(QGC_HAS_GST_GLMEMORY_GPU_PATH)
        QVERIFY2(s.contains(QStringLiteral("memory:GLMemory")),
                 qUtf8Printable(QStringLiteral("GPU bin caps missing memory:GLMemory: ") + s));
#endif
#endif

        gst_object_unref(qvideosink);
        gst_object_unref(bin);
    }
#endif

    gst_object_unref(factory);
}

void GStreamerTest::_testQgcVideoSinkBinRejectsFailedAdopt()
{
    QVERIFY2(GStreamer::completeInit(), "GStreamer::completeInit() failed");

    GstElement* const target = gst_bin_new("qgc-adopt-target");
    GstElement* const parent = gst_bin_new("qgc-adopt-existing-parent");
    GstElement* const element = gst_element_factory_make("identity", "preparented");
    QVERIFY(target && parent && element);
    const auto guard = qScopeGuard([&] {
        gst_object_unref(parent);
        gst_object_unref(target);
    });
    QVERIFY(gst_bin_add(GST_BIN(parent), element));

    GstElement* adopted = nullptr;
    {
        GstQgc::BinChain chain(GST_BIN(target));
        // adopt() consumes a ref even on failure.
        adopted = chain.adopt(GST_ELEMENT(gst_object_ref(element)));
        chain.commit();
    }

    QVERIFY2(!adopted, "BinChain::adopt must reject elements that gst_bin_add() did not accept");
    GstObject* const currentParent = gst_object_get_parent(GST_OBJECT(element));
    QCOMPARE(currentParent, GST_OBJECT(parent));
    gst_object_unref(currentParent);
}

namespace {

struct PipelineRunResult
{
    int frameCount = 0;
    QSize lastFrameSize;
    QVideoFrameFormat::PixelFormat lastPixelFormat = QVideoFrameFormat::Format_Invalid;
    QColor centerPixel;
    bool centerPixelValid = false;
    bool eos = false;
    QString errorMessage;
};

PipelineRunResult runPipelineThroughQVideoSink(QVideoSink& videoSink, QGCQVideoSinkController*& outController,
                                               const char* capsLine, int numBuffers = 5, bool gpuZerocopy = false,
                                               const char* sourceProperties = "", bool captureCenterPixel = false)
{
    PipelineRunResult r;
    const QString launch =
        QStringLiteral("videotestsrc %1 num-buffers=%2 ! %3 ! qgcvideosinkbin name=sink gpu-zerocopy=%4")
            .arg(QString::fromUtf8(sourceProperties))
            .arg(numBuffers)
            .arg(QString::fromUtf8(capsLine))
            .arg(gpuZerocopy ? "true" : "false");
    GError* err = nullptr;
    GstElement* pipeline = gst_parse_launch(launch.toUtf8().constData(), &err);
    if (err) {
        r.errorMessage = QString::fromUtf8(err->message);
        g_clear_error(&err);
        return r;
    }
    if (!pipeline) {
        r.errorMessage = QStringLiteral("Pipeline construction failed");
        return r;
    }
    GstElement* sinkBin = gst_bin_get_by_name(GST_BIN(pipeline), "sink");
    const std::unique_ptr<QObject> owner = std::make_unique<QObject>();
    if (!sinkBin || !GStreamer::setupQVideoSinkElement(sinkBin, &videoSink, owner.get())) {
        r.errorMessage = QStringLiteral("setupQVideoSinkElement() failed");
        if (sinkBin)
            gst_object_unref(sinkBin);
        gst_object_unref(pipeline);
        return r;
    }
    outController = owner->findChild<QGCQVideoSinkController*>(QString(), Qt::FindDirectChildrenOnly);
    if (!outController) {
        r.errorMessage = QStringLiteral("controller not created by setupQVideoSinkElement()");
        gst_object_unref(sinkBin);
        gst_object_unref(pipeline);
        return r;
    }
    QObject::connect(&videoSink, &QVideoSink::videoFrameChanged, owner.get(), [&](const QVideoFrame& f) {
        ++r.frameCount;
        r.lastFrameSize = f.size();
        r.lastPixelFormat = f.pixelFormat();
        if (captureCenterPixel) {
            const QImage image = f.toImage();
            if (!image.isNull()) {
                r.centerPixel = image.pixelColor(image.width() / 2, image.height() / 2);
                r.centerPixelValid = true;
            }
        }
    });
    gst_object_unref(sinkBin);

    if (gst_element_set_state(pipeline, GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE) {
        r.errorMessage = QStringLiteral("set_state(PLAYING) failed");
        outController = nullptr;
        gst_element_set_state(pipeline, GST_STATE_NULL);
        gst_object_unref(pipeline);
        return r;
    }
    GstBus* bus = gst_element_get_bus(pipeline);
    GstMessage* msg = gst_bus_timed_pop_filtered(bus, 5 * GST_SECOND,
                                                 static_cast<GstMessageType>(GST_MESSAGE_EOS | GST_MESSAGE_ERROR));
    if (msg) {
        if (GST_MESSAGE_TYPE(msg) == GST_MESSAGE_EOS)
            r.eos = true;
        else {
            GError* e = nullptr;
            gst_message_parse_error(msg, &e, nullptr);
            r.errorMessage = e ? QString::fromUtf8(e->message) : QStringLiteral("unknown error");
            g_clear_error(&e);
        }
        gst_message_unref(msg);
    } else {
        r.errorMessage = QStringLiteral("timeout waiting for EOS");
    }
    gst_object_unref(bus);
    // Drain any queued videoFrameChanged deliveries (bridged onto this thread) before teardown.
    // No positive count target here -- frameCount is asserted by the caller via QTRY -- so this
    // is a bounded settle drain rather than a condition wait.
    {
        QElapsedTimer drain;
        drain.start();
        while (drain.elapsed() < 50) {
            QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
        }
    }
    outController = nullptr;
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
    return r;
}

}  // namespace

void GStreamerTest::_testCapsCacheInvalidation()
{
    GStreamer::redirectGLibLogging();
    QVERIFY2(GStreamer::completeInit(), "completeInit failed");

    QVideoSink videoSink;
    QGCQVideoSinkController* controller = nullptr;

    auto r1 = runPipelineThroughQVideoSink(videoSink, controller,
                                           "video/x-raw,format=I420,width=320,height=240,framerate=30/1 ! "
                                           "videoconvert ! video/x-raw,format=BGRA");
    QVERIFY2(r1.eos, qUtf8Printable(QStringLiteral("Session 1: %1").arg(r1.errorMessage)));
    QTRY_VERIFY_WITH_TIMEOUT(r1.frameCount > 0, 2000);
    QCOMPARE(r1.lastPixelFormat, QVideoFrameFormat::Format_BGRA8888);

    auto r2 = runPipelineThroughQVideoSink(videoSink, controller,
                                           "video/x-raw,format=I420,width=320,height=240,framerate=30/1 ! "
                                           "videoconvert ! video/x-raw,format=RGBA");
    QVERIFY2(r2.eos, qUtf8Printable(QStringLiteral("Session 2: %1").arg(r2.errorMessage)));
    QTRY_VERIFY_WITH_TIMEOUT(r2.frameCount > 0, 2000);
    QCOMPARE(r2.lastPixelFormat, QVideoFrameFormat::Format_RGBA8888);
}

void GStreamerTest::_testGpuZeroCopyFallback()
{
    GStreamer::redirectGLibLogging();
    QVERIFY2(GStreamer::completeInit(), "completeInit failed");

#if defined(QGC_HAS_GST_DMABUF_GPU_PATH)
    const quint64 dmabufBefore = GstHwPathTelemetry::peekMapFailureCount(HwVideoBufferPath::DmaBuf);
#endif

    QVideoSink videoSink;
    QGCQVideoSinkController* controller = nullptr;
    auto r = runPipelineThroughQVideoSink(videoSink, controller,
                                          "video/x-raw,format=BGRA,width=320,height=240,framerate=30/1",
                                          /*numBuffers*/ 5, /*gpuZerocopy*/ true);
    // The gpu-zerocopy bin inserts glupload on GL builds; a headless runner with no EGL/GL context can't start it.
    if (!r.eos && (r.errorMessage.contains(QStringLiteral("egl"), Qt::CaseInsensitive) ||
                   r.errorMessage.contains(QStringLiteral("gl context"), Qt::CaseInsensitive))) {
        QSKIP(qPrintable(
            QStringLiteral("gpu-zerocopy pipeline needs a GL context (headless env): %1").arg(r.errorMessage)));
    }
    QVERIFY2(r.eos, qUtf8Printable(QStringLiteral("Pipeline: %1").arg(r.errorMessage)));
    QTRY_VERIFY_WITH_TIMEOUT(r.frameCount > 0, 2000);
    QCOMPARE(r.lastPixelFormat, QVideoFrameFormat::Format_BGRA8888);

#if defined(QGC_HAS_GST_DMABUF_GPU_PATH)
    QCOMPARE(GstHwPathTelemetry::peekMapFailureCount(HwVideoBufferPath::DmaBuf), dmabufBefore);
#endif
}

void GStreamerTest::_testWindowsGpuSinkPreservesSoftwarePixels()
{
#if defined(Q_OS_WIN) && defined(QGC_HAS_GST_D3D11_GPU_PATH)
    GStreamer::redirectGLibLogging();
    QVERIFY2(GStreamer::completeInit(), "completeInit failed");

    QVideoSink videoSink;
    QGCQVideoSinkController* controller = nullptr;
    const auto result = runPipelineThroughQVideoSink(videoSink, controller,
                                                     "video/x-raw,format=BGRA,width=320,height=240,framerate=30/1",
                                                     /*numBuffers*/ 5, /*gpuZerocopy*/ true, "pattern=red",
                                                     /*captureCenterPixel*/ true);

    QVERIFY2(result.eos, qUtf8Printable(QStringLiteral("Software-frame pipeline: %1").arg(result.errorMessage)));
    QVERIFY(result.frameCount > 0);
    QCOMPARE(result.lastPixelFormat, QVideoFrameFormat::Format_BGRA8888);
    QVERIFY2(result.centerPixelValid, "System-memory fallback frame could not be mapped for pixel verification");
    QVERIFY2((result.centerPixel.red() > 200) && (result.centerPixel.green() < 50) && (result.centerPixel.blue() < 50),
             qUtf8Printable(QStringLiteral("Expected a red software frame, received RGB(%1,%2,%3)")
                                .arg(result.centerPixel.red())
                                .arg(result.centerPixel.green())
                                .arg(result.centerPixel.blue())));
#else
    QSKIP("Windows D3D11 GPU sink path is unavailable in this build");
#endif
}

void GStreamerTest::_testWindowsGpuSinkPreservesDirectD3D11Memory()
{
#if defined(Q_OS_WIN) && defined(QGC_HAS_GST_D3D11_GPU_PATH)
    GStreamer::redirectGLibLogging();
    QVERIFY2(GStreamer::completeInit(), "completeInit failed");

    GstElementFactory* uploadFactory = gst_element_factory_find("d3d11upload");
    if (!uploadFactory) {
        QSKIP("d3d11upload is unavailable");
    }
    gst_object_unref(uploadFactory);

    QVideoSink videoSink;
    QGCQVideoSinkController* controller = nullptr;
    auto result =
        runPipelineThroughQVideoSink(videoSink, controller,
                                     "video/x-raw,format=NV12,width=320,height=240,framerate=30/1 ! d3d11upload ! "
                                     "video/x-raw(memory:D3D11Memory),format=NV12",
                                     /*numBuffers*/ 5, /*gpuZerocopy*/ true);
    if (!result.eos && result.errorMessage.contains(QStringLiteral("d3d11"), Qt::CaseInsensitive)) {
        QSKIP(qPrintable(QStringLiteral("D3D11 device unavailable in this runner: %1").arg(result.errorMessage)));
    }

    QVERIFY2(result.eos, qUtf8Printable(QStringLiteral("D3D11 pipeline: %1").arg(result.errorMessage)));
    QVERIFY(result.frameCount > 0);
    QCOMPARE(result.lastPixelFormat, QVideoFrameFormat::Format_NV12);
#else
    QSKIP("D3D11 GPU sink path is only available in supported Windows builds");
#endif
}

void GStreamerTest::_testAppsinkTeardownUnderLoad()
{
    GStreamer::redirectGLibLogging();
    QVERIFY2(GStreamer::completeInit(), "completeInit failed");

    QVideoSink videoSink;

    {
        GError* err = nullptr;
        GstElement* pipeline = gst_parse_launch(
            "videotestsrc is-live=true ! "
            "video/x-raw,format=BGRA,width=160,height=120,framerate=60/1 ! "
            "qgcvideosinkbin name=sink",
            &err);
        if (err) {
            g_clear_error(&err);
        }
        QVERIFY(pipeline);

        GstElement* sinkBin = gst_bin_get_by_name(GST_BIN(pipeline), "sink");
        QVERIFY(sinkBin);
        QObject controllerOwner1;
        QVERIFY(GStreamer::setupQVideoSinkElement(sinkBin, &videoSink, &controllerOwner1));
        GstElement* qvideosink = gst_bin_get_by_name(GST_BIN(sinkBin), "qgcqvideosink");
        gst_object_unref(sinkBin);
        QVERIFY(qvideosink);

        QVERIFY(gst_element_set_state(pipeline, GST_STATE_PLAYING) != GST_STATE_CHANGE_FAILURE);
        // Tear down only once frames are actually flowing.
        QVERIFY_TRUE_WAIT(framesDelivered(qvideosink) > 0, TestTimeout::mediumMs());
        gst_object_unref(qvideosink);
        // controllerOwner1 goes out of scope here — implicit teardown.

        gst_element_set_state(pipeline, GST_STATE_NULL);
        gst_object_unref(pipeline);
    }

    QGCQVideoSinkController* controller = nullptr;
    auto r = runPipelineThroughQVideoSink(videoSink, controller,
                                          "video/x-raw,format=RGBA,width=160,height=120,framerate=30/1");
    QVERIFY2(r.eos, qUtf8Printable(QStringLiteral("Restart: %1").arg(r.errorMessage)));
    QTRY_VERIFY_WITH_TIMEOUT(r.frameCount > 0, 2000);
    QCOMPARE(r.lastPixelFormat, QVideoFrameFormat::Format_RGBA8888);
}

void GStreamerTest::_testFrameCountsTelemetrySignal()
{
    GStreamer::redirectGLibLogging();
    QVERIFY2(GStreamer::completeInit(), "completeInit failed");

    // Telemetry counters are process-global and leak across tests; drain them so the
    // assertions below measure only this pipeline's contribution.
    for (auto p :
         {HwVideoBufferPath::None, HwVideoBufferPath::DmaBuf, HwVideoBufferPath::GlMemory, HwVideoBufferPath::D3D11,
          HwVideoBufferPath::D3D12, HwVideoBufferPath::IOSurface, HwVideoBufferPath::AHardwareBuffer}) {
        (void) GstHwPathTelemetry::takeDeliveredCount(p);
        (void) GstHwPathTelemetry::takeMapFailureCount(p);
    }

    QVideoSink videoSink;
    QObject controllerOwner;

    GError* err = nullptr;
    GstElement* pipeline = gst_parse_launch(
        "videotestsrc is-live=false num-buffers=90 ! "
        "video/x-raw,format=BGRA,width=320,height=240,framerate=30/1 ! "
        "qgcvideosinkbin name=sink",
        &err);
    if (err) {
        g_clear_error(&err);
    }
    QVERIFY2(pipeline, "Pipeline construction failed");

    GstElement* sinkBin = gst_bin_get_by_name(GST_BIN(pipeline), "sink");
    QVERIFY(sinkBin);
    QVERIFY(GStreamer::setupQVideoSinkElement(sinkBin, &videoSink, &controllerOwner));
    gst_object_unref(sinkBin);

    auto* controller = controllerOwner.findChild<QGCQVideoSinkController*>(QString(), Qt::FindDirectChildrenOnly);
    QVERIFY(controller);
    QSignalSpy spy(controller, &QGCQVideoSinkController::frameCountsChanged);

    QVERIFY(gst_element_set_state(pipeline, GST_STATE_PLAYING) != GST_STATE_CHANGE_FAILURE);

    QTRY_VERIFY_WITH_TIMEOUT(spy.size() > 0, 5000);

    QVERIFY(GstHwPathTelemetry::peekDeliveredCount(HwVideoBufferPath::None) > 0);
    quint64 gpuFallback = 0;
    for (auto p : {HwVideoBufferPath::DmaBuf, HwVideoBufferPath::GlMemory, HwVideoBufferPath::D3D11,
                   HwVideoBufferPath::D3D12, HwVideoBufferPath::IOSurface, HwVideoBufferPath::AHardwareBuffer}) {
        gpuFallback += GstHwPathTelemetry::peekMapFailureCount(p);
    }
    QCOMPARE(gpuFallback, quint64(0));

    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
}

void GStreamerTest::_testInactiveQgcQVideoSinkDropsAndCounts()
{
    GStreamer::redirectGLibLogging();
    QVERIFY2(GStreamer::completeInit(), "completeInit failed");

    QVideoSink videoSink;
    QObject controllerOwner;
    int frameCount = 0;
    QObject::connect(&videoSink, &QVideoSink::videoFrameChanged, &controllerOwner, [&](const QVideoFrame&) {
        ++frameCount;
    });

    GError* err = nullptr;
    GstElement* pipeline = gst_parse_launch(
        "videotestsrc is-live=false num-buffers=12 ! "
        "video/x-raw,format=BGRA,width=96,height=64,framerate=30/1 ! "
        "qgcvideosinkbin name=sink",
        &err);
    if (err) {
        const QString msg = QString::fromUtf8(err->message);
        g_clear_error(&err);
        QFAIL(qPrintable(QStringLiteral("Pipeline parse error: %1").arg(msg)));
    }
    QVERIFY2(pipeline, "Pipeline construction failed");
    const auto pipelineGuard = qScopeGuard([&] {
        gst_element_set_state(pipeline, GST_STATE_NULL);
        gst_object_unref(pipeline);
    });

    GstElement* sinkBin = gst_bin_get_by_name(GST_BIN(pipeline), "sink");
    QVERIFY(sinkBin);
    QVERIFY(GStreamer::setupQVideoSinkElement(sinkBin, &videoSink, &controllerOwner));

    GstElement* qvideosink = gst_bin_get_by_name(GST_BIN(sinkBin), "qgcqvideosink");
    gst_object_unref(sinkBin);
    QVERIFY(qvideosink);
    const auto sinkGuard = qScopeGuard([&] { gst_object_unref(qvideosink); });

    g_object_set(qvideosink, "active", FALSE, nullptr);

    QVERIFY(gst_element_set_state(pipeline, GST_STATE_PLAYING) != GST_STATE_CHANGE_FAILURE);
    GstBus* bus = gst_element_get_bus(pipeline);
    QVERIFY(bus);
    GstMessage* msg = gst_bus_timed_pop_filtered(bus, 5 * GST_SECOND,
                                                 static_cast<GstMessageType>(GST_MESSAGE_EOS | GST_MESSAGE_ERROR));
    gst_object_unref(bus);
    QVERIFY2(msg, "Pipeline timed out waiting for EOS or ERROR");
    const auto msgGuard = qScopeGuard([&] { gst_message_unref(msg); });

    if (GST_MESSAGE_TYPE(msg) == GST_MESSAGE_ERROR) {
        GError* gstError = nullptr;
        gchar* debug = nullptr;
        gst_message_parse_error(msg, &gstError, &debug);
        const QString errorMessage = QStringLiteral("%1 (%2)")
                                         .arg(gstError ? QString::fromUtf8(gstError->message)
                                                       : QStringLiteral("unknown"))
                                         .arg(debug ? QString::fromUtf8(debug) : QString());
        g_clear_error(&gstError);
        g_free(debug);
        QFAIL(qPrintable(QStringLiteral("Pipeline error: %1").arg(errorMessage)));
    }
    QCOMPARE(GST_MESSAGE_TYPE(msg), GST_MESSAGE_EOS);

    QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
    QCOMPARE(frameCount, 0);

    guint64 input = 0;
    guint64 dropped = 0;
    guint64 delivered = 0;
    g_object_get(qvideosink, "frames-input", &input, "frames-dropped", &dropped, "frames-delivered", &delivered,
                 nullptr);
    QVERIFY(input >= guint64(12));
    QCOMPARE(dropped, input);
    QCOMPARE(delivered, guint64(0));
}

void GStreamerTest::_testGetAppsinkAccessor()
{
    GStreamer::redirectGLibLogging();
    QVERIFY2(GStreamer::completeInit(), "completeInit failed");

    GstElement* bin = gst_element_factory_make("qgcvideosinkbin", nullptr);
    QVERIFY2(bin, "qgcvideosinkbin factory not found");

    // Construction is synchronous (GObject::constructed runs inside factory_make).
    GstElement* qvideosink = gst_qgc_video_sink_bin_get_qvideosink(GST_QGC_VIDEO_SINK_BIN(bin));
    QVERIFY2(qvideosink, "qvideosink accessor returned NULL after factory_make");
    QVERIFY(GST_IS_ELEMENT(qvideosink));
    gst_object_unref(qvideosink);

    // After transitioning to READY the accessor must still return a valid element.
    GstStateChangeReturn ret = gst_element_set_state(bin, GST_STATE_READY);
    QVERIFY(ret != GST_STATE_CHANGE_FAILURE);

    GstElement* qvideosink2 = gst_qgc_video_sink_bin_get_qvideosink(GST_QGC_VIDEO_SINK_BIN(bin));
    QVERIFY2(qvideosink2, "qvideosink accessor returned NULL after READY");
    QVERIFY(GST_IS_ELEMENT(qvideosink2));
    gst_object_unref(qvideosink2);

    gst_element_set_state(bin, GST_STATE_NULL);
    gst_object_unref(bin);
}

void GStreamerTest::_testQVideoSinkControllerClearsElementOnDestroy()
{
    GStreamer::redirectGLibLogging();
    QVERIFY2(GStreamer::completeInit(), "completeInit failed");

    GstElement* bin = gst_element_factory_make("qgcvideosinkbin", nullptr);
    QVERIFY2(bin, "qgcvideosinkbin factory not found");

    GstElement* qvideosink = gst_qgc_video_sink_bin_get_qvideosink(GST_QGC_VIDEO_SINK_BIN(bin));
    QVERIFY2(qvideosink, "qvideosink accessor returned NULL after factory_make");

    QVideoSink videoSink;
    auto controllerOwner = std::make_unique<QObject>();
    QVERIFY2(GStreamer::setupQVideoSinkElement(bin, &videoSink, controllerOwner.get()),
             "setupQVideoSinkElement() failed");

    gpointer installedSink = nullptr;
    gboolean active = FALSE;
    g_object_get(qvideosink, "qvideosink", &installedSink, "active", &active, nullptr);
    QCOMPARE(installedSink, static_cast<gpointer>(&videoSink));
    QCOMPARE(active, TRUE);

    controllerOwner.reset();

    installedSink = &videoSink;
    active = TRUE;
    g_object_get(qvideosink, "qvideosink", &installedSink, "active", &active, nullptr);
    QCOMPARE(installedSink, static_cast<gpointer>(nullptr));
    QCOMPARE(active, FALSE);

    gst_object_unref(qvideosink);
    gst_object_unref(bin);
}

void GStreamerTest::_testQVideoSinkControllerClearsElementWhenVideoSinkDestroyed()
{
    GStreamer::redirectGLibLogging();
    QVERIFY2(GStreamer::completeInit(), "completeInit failed");

    GstElement* bin = gst_element_factory_make("qgcvideosinkbin", nullptr);
    QVERIFY2(bin, "qgcvideosinkbin factory not found");

    GstElement* qvideosink = gst_qgc_video_sink_bin_get_qvideosink(GST_QGC_VIDEO_SINK_BIN(bin));
    QVERIFY2(qvideosink, "qvideosink accessor returned NULL after factory_make");

    auto videoSink = std::make_unique<QVideoSink>();
    QObject controllerOwner;
    QVERIFY2(GStreamer::setupQVideoSinkElement(bin, videoSink.get(), &controllerOwner),
             "setupQVideoSinkElement() failed");

    gpointer installedSink = nullptr;
    gboolean active = FALSE;
    g_object_get(qvideosink, "qvideosink", &installedSink, "active", &active, nullptr);
    QCOMPARE(installedSink, static_cast<gpointer>(videoSink.get()));
    QCOMPARE(active, TRUE);

    videoSink.reset();
    QCoreApplication::sendPostedEvents(&controllerOwner, QEvent::MetaCall);

    installedSink = reinterpret_cast<gpointer>(quintptr(0x1));
    active = FALSE;
    g_object_get(qvideosink, "qvideosink", &installedSink, "active", &active, nullptr);
    QCOMPARE(installedSink, static_cast<gpointer>(nullptr));
    QCOMPARE(active, FALSE);

    gst_object_unref(qvideosink);
    gst_object_unref(bin);
}

void GStreamerTest::_testQVideoSinkControllerNullSinkStillDeactivatesOnDestroy()
{
    GStreamer::redirectGLibLogging();
    QVERIFY2(GStreamer::completeInit(), "completeInit failed");

    GstElement* bin = gst_element_factory_make("qgcvideosinkbin", nullptr);
    QVERIFY2(bin, "qgcvideosinkbin factory not found");

    GstElement* qvideosink = gst_qgc_video_sink_bin_get_qvideosink(GST_QGC_VIDEO_SINK_BIN(bin));
    QVERIFY2(qvideosink, "qvideosink accessor returned NULL after factory_make");

    QVideoSink videoSink;
    auto controllerOwner = std::make_unique<QObject>();
    QVERIFY2(GStreamer::setupQVideoSinkElement(bin, &videoSink, controllerOwner.get()),
             "setupQVideoSinkElement() failed");

    auto* controller = controllerOwner->findChild<QGCQVideoSinkController*>(QString(), Qt::FindDirectChildrenOnly);
    QVERIFY(controller);

    controller->setVideoSink(QPointer<QVideoSink>());

    gpointer installedSink = &videoSink;
    gboolean active = FALSE;
    g_object_get(qvideosink, "qvideosink", &installedSink, "active", &active, nullptr);
    QCOMPARE(installedSink, static_cast<gpointer>(nullptr));
    QCOMPARE(active, FALSE);

    controllerOwner.reset();

    active = FALSE;
    g_object_get(qvideosink, "active", &active, nullptr);
    QCOMPARE(active, FALSE);

    gst_object_unref(qvideosink);
    gst_object_unref(bin);
}

void GStreamerTest::_testQVideoSinkControllerRepeatedSetupKeepsNewBindingActive()
{
    GStreamer::redirectGLibLogging();
    QVERIFY2(GStreamer::completeInit(), "completeInit failed");

    GstElement* bin = gst_element_factory_make("qgcvideosinkbin", nullptr);
    QVERIFY2(bin, "qgcvideosinkbin factory not found");

    GstElement* qvideosink = gst_qgc_video_sink_bin_get_qvideosink(GST_QGC_VIDEO_SINK_BIN(bin));
    QVERIFY2(qvideosink, "qvideosink accessor returned NULL after factory_make");

    auto firstSink = std::make_unique<QVideoSink>();
    auto secondSink = std::make_unique<QVideoSink>();
    QObject controllerOwner;

    QVERIFY2(GStreamer::setupQVideoSinkElement(bin, firstSink.get(), &controllerOwner),
             "initial setupQVideoSinkElement() failed");
    auto* firstController =
        controllerOwner.findChild<QGCQVideoSinkController*>(QString(), Qt::FindDirectChildrenOnly);
    QVERIFY(firstController);

    QVERIFY2(GStreamer::setupQVideoSinkElement(bin, secondSink.get(), &controllerOwner),
             "repeated setupQVideoSinkElement() failed");

    firstController->setActive(false);

    firstSink.reset();
    QCoreApplication::sendPostedEvents(&controllerOwner, QEvent::MetaCall);

    gpointer installedSink = nullptr;
    gboolean active = FALSE;
    g_object_get(qvideosink, "qvideosink", &installedSink, "active", &active, nullptr);
    QCOMPARE(installedSink, static_cast<gpointer>(secondSink.get()));
    QCOMPARE(active, TRUE);

    QCoreApplication::sendPostedEvents(&controllerOwner, QEvent::DeferredDelete);

    installedSink = nullptr;
    active = FALSE;
    g_object_get(qvideosink, "qvideosink", &installedSink, "active", &active, nullptr);
    QCOMPARE(installedSink, static_cast<gpointer>(secondSink.get()));
    QCOMPARE(active, TRUE);

    gst_object_unref(qvideosink);
    gst_object_unref(bin);
}

void GStreamerTest::_testQVideoSinkControllerNoWindowStartsInactive()
{
    GStreamer::redirectGLibLogging();
    QVERIFY2(GStreamer::completeInit(), "completeInit failed");

    GstElement* bin = gst_element_factory_make("qgcvideosinkbin", nullptr);
    QVERIFY2(bin, "qgcvideosinkbin factory not found");

    GstElement* qvideosink = gst_qgc_video_sink_bin_get_qvideosink(GST_QGC_VIDEO_SINK_BIN(bin));
    QVERIFY2(qvideosink, "qvideosink accessor returned NULL after factory_make");

    QObject controllerOwner;
    QGCQVideoSinkController controller(qvideosink, &controllerOwner);
    controller.setActive(true);

    QQuickVideoOutput videoOutput;
    QVERIFY(videoOutput.window() == nullptr);
    QGCQVideoSinkController::syncActiveToWindowVisibility(&controllerOwner, &videoOutput);

    gboolean active = TRUE;
    g_object_get(qvideosink, "active", &active, nullptr);
    QCOMPARE(active, FALSE);

    gst_object_unref(qvideosink);
    gst_object_unref(bin);
}

void GStreamerTest::_testCpuZeroCopyFrameRejectsWritableMap()
{
    GstVideoInfo info;
    gst_video_info_init(&info);
    gst_video_info_set_format(&info, GST_VIDEO_FORMAT_BGRA, 4, 4);

    GstBuffer* buffer = gst_buffer_new_allocate(nullptr, GST_VIDEO_INFO_SIZE(&info), nullptr);
    QVERIFY(buffer);
    auto bufferGuard = qScopeGuard([&] { gst_buffer_unref(buffer); });

    GstMapInfo mapInfo;
    QVERIFY(gst_buffer_map(buffer, &mapInfo, GST_MAP_WRITE));
    std::memset(mapInfo.data, 0x7f, mapInfo.size);
    gst_buffer_unmap(buffer, &mapInfo);

    QVideoFrameFormat format(QSize(4, 4), QVideoFrameFormat::Format_BGRA8888);
    auto wrapped = CpuVideoFramePool::wrapZeroCopy(buffer, info, format, std::make_shared<std::atomic<int>>(0));
    QVERIFY(wrapped);

    QVideoFrame frame(std::move(wrapped));
    QVERIFY(frame.isValid());

    QVERIFY(frame.map(QVideoFrame::ReadOnly));
    frame.unmap();

    QVERIFY(!frame.map(QVideoFrame::WriteOnly));
    QVERIFY(!frame.map(QVideoFrame::ReadWrite));
}

void GStreamerTest::_testCpuZeroCopyPinnedFramesCappedPerSink()
{
    GstVideoInfo info;
    gst_video_info_init(&info);
    gst_video_info_set_format(&info, GST_VIDEO_FORMAT_BGRA, 4, 4);
    GstBuffer* buffer = gst_buffer_new_allocate(nullptr, GST_VIDEO_INFO_SIZE(&info), nullptr);
    QVERIFY(buffer);
    auto bufferGuard = qScopeGuard([&] { gst_buffer_unref(buffer); });
    const QVideoFrameFormat format(QSize(4, 4), QVideoFrameFormat::Format_BGRA8888);

    const auto sinkA = std::make_shared<std::atomic<int>>(0);
    const auto sinkB = std::make_shared<std::atomic<int>>(0);
    std::vector<std::unique_ptr<QAbstractVideoBuffer>> pinned;
    for (int i = 0; i < CpuVideoFramePool::kMaxPinnedFrames; ++i) {
        pinned.push_back(CpuVideoFramePool::wrapZeroCopy(buffer, info, format, sinkA));
        QVERIFY(pinned.back());
    }

    // At the cap the caller must copy instead; another sink's budget is independent.
    QVERIFY(!CpuVideoFramePool::wrapZeroCopy(buffer, info, format, sinkA));
    QVERIFY(CpuVideoFramePool::wrapZeroCopy(buffer, info, format, sinkB));

    // Releasing a frame (Qt dropping it) frees its slot.
    pinned.pop_back();
    QVERIFY(CpuVideoFramePool::wrapZeroCopy(buffer, info, format, sinkA));
}

void GStreamerTest::_testCpuMemcpyActiveRowStrideHandling()
{
    GStreamer::redirectGLibLogging();
    QVERIFY2(GStreamer::completeInit(), "completeInit failed");

    // Width 753 is not 4-byte aligned; GStreamer will pad stride to 756.
    // If the memcpy copies stride bytes instead of active-row bytes, the
    // right-edge pixels of the right-most component will contain padding zeros.
    QVideoSink videoSink;
    QGCQVideoSinkController* controller = nullptr;
    QObject controllerOwner;

    QVideoFrame capturedFrame;
    // Controller teardown clears the QVideoSink with an invalid frame; keep the last real one.
    QObject::connect(&videoSink, &QVideoSink::videoFrameChanged, &controllerOwner, [&](const QVideoFrame& f) {
        if (f.isValid()) {
            capturedFrame = f;
        }
    });

    auto r = runPipelineThroughQVideoSink(videoSink, controller,
                                          "video/x-raw,format=BGRA,width=753,height=432,framerate=30/1",
                                          /*numBuffers=*/5);
    QVERIFY2(r.eos, qUtf8Printable(QStringLiteral("Pipeline: %1").arg(r.errorMessage)));
    QTRY_VERIFY_WITH_TIMEOUT(capturedFrame.isValid(), 2000);

    QVERIFY(capturedFrame.map(QVideoFrame::ReadOnly));
    const uchar* data = capturedFrame.bits(0);
    const int stride = capturedFrame.bytesPerLine(0);
    // Sample the last active pixel column (x=752) from row 0; BGRA so 4 bytes/pixel.
    const uchar* lastPixel = data + 752 * 4;
    // videotestsrc pattern=0 (smpte) fills with non-zero color in top rows.
    const bool nonZero = (lastPixel[0] | lastPixel[1] | lastPixel[2] | lastPixel[3]) != 0;
    capturedFrame.unmap();

    (void) stride;
    QVERIFY2(nonZero, "Last active column is all zeros — likely stride vs active-row memcpy bug");
}

void GStreamerTest::_testColorimetryPixelFormatMapping()
{
    QCOMPARE(toQtPixelFormat(GST_VIDEO_FORMAT_NV12), QVideoFrameFormat::Format_NV12);
    QCOMPARE(toQtPixelFormat(GST_VIDEO_FORMAT_NV21), QVideoFrameFormat::Format_NV21);
    QCOMPARE(toQtPixelFormat(GST_VIDEO_FORMAT_I420), QVideoFrameFormat::Format_YUV420P);
    QCOMPARE(toQtPixelFormat(GST_VIDEO_FORMAT_YV12), QVideoFrameFormat::Format_YV12);
    QCOMPARE(toQtPixelFormat(GST_VIDEO_FORMAT_Y42B), QVideoFrameFormat::Format_YUV422P);
    QCOMPARE(toQtPixelFormat(GST_VIDEO_FORMAT_P010_10LE), QVideoFrameFormat::Format_P010);
    QCOMPARE(toQtPixelFormat(GST_VIDEO_FORMAT_AYUV), QVideoFrameFormat::Format_AYUV);
    QCOMPARE(toQtPixelFormat(GST_VIDEO_FORMAT_YUY2), QVideoFrameFormat::Format_YUYV);
    QCOMPARE(toQtPixelFormat(GST_VIDEO_FORMAT_UYVY), QVideoFrameFormat::Format_UYVY);
    QCOMPARE(toQtPixelFormat(GST_VIDEO_FORMAT_GRAY8), QVideoFrameFormat::Format_Y8);
    QCOMPARE(toQtPixelFormat(GST_VIDEO_FORMAT_GRAY16_LE), QVideoFrameFormat::Format_Y16);
    QCOMPARE(toQtPixelFormat(GST_VIDEO_FORMAT_BGRA), QVideoFrameFormat::Format_BGRA8888);
    QCOMPARE(toQtPixelFormat(GST_VIDEO_FORMAT_RGBA), QVideoFrameFormat::Format_RGBA8888);
    QCOMPARE(toQtPixelFormat(GST_VIDEO_FORMAT_I420_10LE), QVideoFrameFormat::Format_YUV420P10);
    QCOMPARE(toQtPixelFormat(GST_VIDEO_FORMAT_P016_LE), QVideoFrameFormat::Format_P016);
    QCOMPARE(toQtPixelFormat(GST_VIDEO_FORMAT_Y444), QVideoFrameFormat::Format_Invalid);
}

void GStreamerTest::_testCpuCapsFormatsRoundTripToQt()
{
    GstCaps* caps = gst_caps_from_string(GstQgc::buildCpuCapsString().c_str());
    QVERIFY2(caps, "buildCpuCapsString() did not parse");
    const GstStructure* s = gst_caps_get_structure(caps, 0);
    const GValue* fmt = gst_structure_get_value(s, "format");
    QVERIFY2(fmt && GST_VALUE_HOLDS_LIST(fmt), "CPU caps format field is not a list");
    const guint n = gst_value_list_get_size(fmt);
    QVERIFY2(n > 0, "CPU caps format list empty");
    for (guint i = 0; i < n; ++i) {
        const char* name = g_value_get_string(gst_value_list_get_value(fmt, i));
        const GstVideoFormat gf = gst_video_format_from_string(name);
        QVERIFY2(gf != GST_VIDEO_FORMAT_UNKNOWN,
                 qPrintable(QStringLiteral("caps lists unknown gst format '%1'").arg(name)));
        QVERIFY2(toQtPixelFormat(gf) != QVideoFrameFormat::Format_Invalid,
                 qPrintable(QStringLiteral("format '%1' advertised but not Qt-renderable").arg(name)));
    }
    gst_caps_unref(caps);
}

void GStreamerTest::_testAllocationQueryHwMemoryPoolHint()
{
    GstCaps* caps = gst_caps_from_string(
        "video/x-raw(memory:DMABuf), format=(string)NV12, width=(int)64, height=(int)64, framerate=(fraction)30/1");
    QVERIFY2(caps, "HW caps did not parse");

    GstQuery* query = gst_query_new_allocation(caps, TRUE);
    GstQgc::populateAllocationQuery(query);

    QCOMPARE(gst_query_get_n_allocation_pools(query), 1U);
    GstBufferPool* pool = nullptr;
    guint size = 0, minBuffers = 0, maxBuffers = 0;
    gst_query_parse_nth_allocation_pool(query, 0, &pool, &size, &minBuffers, &maxBuffers);
    QVERIFY2(pool == nullptr, "HW path must not propose a generic pool for native memory");
    QVERIFY2(minBuffers > guint(CpuVideoFramePool::kMaxPinnedFrames),
             "min-buffer hint must leave the decoder a buffer beyond those the sink and Qt hold");
    QVERIFY2(gst_query_find_allocation_meta(query, GST_VIDEO_META_API_TYPE, nullptr),
             "VideoMeta not advertised on HW path");

    gst_query_unref(query);
    gst_caps_unref(caps);
}

void GStreamerTest::_testQgcVideoSinkBinAllocationQueryAdvertisesVideoMeta()
{
    QVERIFY2(GStreamer::completeInit(), "GStreamer::completeInit() failed");

#if defined(QGC_GST_BIN_USE_GLUPLOAD)
    QSKIP("Direct-DMABuf ALLOCATION VideoMeta contract does not apply when the GPU bin routes through glupload");
#endif

    GstElement* bin = gst_element_factory_make_full("qgcvideosinkbin", "gpu-zerocopy", TRUE, nullptr);
    QVERIFY2(bin, "qgcvideosinkbin factory did not create an element");
    auto binGuard = qScopeGuard([&] { gst_object_unref(bin); });

    GstPad* sinkPad = gst_element_get_static_pad(bin, "sink");
    QVERIFY2(sinkPad, "qgcvideosinkbin has no sink pad");
    auto sinkPadGuard = qScopeGuard([&] { gst_object_unref(sinkPad); });

    GstCaps* caps = gst_caps_from_string(
        "video/x-raw(memory:DMABuf), format=(string)NV12, width=(int)64, height=(int)64, framerate=(fraction)30/1");
    QVERIFY2(caps, "HW caps did not parse");
    auto capsGuard = qScopeGuard([&] { gst_caps_unref(caps); });

    GstQuery* query = gst_query_new_allocation(caps, TRUE);
    QVERIFY2(query, "Could not allocate ALLOCATION query");
    auto queryGuard = qScopeGuard([&] { gst_query_unref(query); });

    QVERIFY2(gst_pad_query(sinkPad, query), "qgcvideosinkbin did not answer ALLOCATION query");
    QVERIFY2(gst_query_find_allocation_meta(query, GST_VIDEO_META_API_TYPE, nullptr),
             "qgcvideosinkbin ALLOCATION query must advertise VideoMeta for gst-va DMABuf");

    QCOMPARE(gst_query_get_n_allocation_pools(query), 1U);
    GstBufferPool* pool = nullptr;
    guint size = 0, minBuffers = 0, maxBuffers = 0;
    gst_query_parse_nth_allocation_pool(query, 0, &pool, &size, &minBuffers, &maxBuffers);
    QVERIFY2(pool == nullptr, "qgcvideosinkbin must not propose a generic pool for native memory");
    QVERIFY2(minBuffers > 0, "qgcvideosinkbin min-buffer hint must be non-zero");
}

void GStreamerTest::_testQgcQVideoSinkProposesVideoMetaPool()
{
    QVERIFY2(GStreamer::completeInit(), "completeInit failed");

    GstElement* sink = gst_element_factory_make("qgcqvideosink", nullptr);
    QVERIFY2(sink, "qgcqvideosink factory not found");
    gst_object_ref_sink(sink);
    auto sinkGuard = qScopeGuard([&] { gst_object_unref(sink); });
    GstPad* sinkPad = gst_element_get_static_pad(sink, "sink");
    QVERIFY(sinkPad);
    auto sinkPadGuard = qScopeGuard([&] { gst_object_unref(sinkPad); });

    GstCaps* caps = gst_caps_from_string(
        "video/x-raw, format=(string)BGRA, width=(int)64, height=(int)64, framerate=(fraction)30/1");
    QVERIFY2(caps, "System-memory caps did not parse");
    auto capsGuard = qScopeGuard([&] { gst_caps_unref(caps); });
    GstQuery* query = gst_query_new_allocation(caps, /*need_pool=*/TRUE);
    auto queryGuard = qScopeGuard([&] { gst_query_unref(query); });

    QVERIFY2(gst_pad_query(sinkPad, query), "qgcqvideosink did not answer ALLOCATION");
    QCOMPARE(gst_query_get_n_allocation_pools(query), 1U);
    GstBufferPool* pool = nullptr;
    gst_query_parse_nth_allocation_pool(query, 0, &pool, nullptr, nullptr, nullptr);
    QVERIFY2(pool, "need_pool=TRUE must get a pool proposal");
    const bool hasVideoMeta = gst_buffer_pool_has_option(pool, GST_BUFFER_POOL_OPTION_VIDEO_META);
    gst_object_unref(pool);
    QVERIFY2(hasVideoMeta, "proposed pool must support VIDEO_META so decoders can write into it directly");
}

void GStreamerTest::_testAllocationQuerySystemMemoryNoPoolStillAdvertisesMetas()
{
    // When upstream does not need a pool, qgcqvideosink must not force one. It still has to advertise consumed metas
    // so crop/orientation/video metadata survive into GstQgcFrameMap.
    GstCaps* caps = gst_caps_from_string(
        "video/x-raw, format=(string)BGRA, width=(int)64, height=(int)64, framerate=(fraction)30/1");
    QVERIFY2(caps, "System-memory caps did not parse");

    GstQuery* query = gst_query_new_allocation(caps, FALSE);
    GstQgc::populateAllocationQuery(query);

    QCOMPARE(gst_query_get_n_allocation_pools(query), 0U);
    QVERIFY2(gst_query_find_allocation_meta(query, GST_VIDEO_META_API_TYPE, nullptr),
             "VideoMeta not advertised for system-memory caps");
    QVERIFY2(gst_query_find_allocation_meta(query, GST_VIDEO_CROP_META_API_TYPE, nullptr),
             "VideoCropMeta not advertised for system-memory caps");

    gst_query_unref(query);
    gst_caps_unref(caps);
}

void GStreamerTest::_testColorimetryColorSpaceMapping()
{
    // Mirrors Qt 6.10.3 qgst.cpp: SMPTE240M maps to AdobeRgb, FCC remains Undefined.
    QCOMPARE(toQtColorSpace(GST_VIDEO_COLOR_MATRIX_SMPTE240M), QVideoFrameFormat::ColorSpace_AdobeRgb);
    QCOMPARE(toQtColorSpace(GST_VIDEO_COLOR_MATRIX_FCC), QVideoFrameFormat::ColorSpace_Undefined);
}

void GStreamerTest::_testColorimetryResolutionHeuristicMatchesQt()
{
    const auto inferredColorSpace = [](int height) {
        GstVideoInfo info;
        gst_video_info_init(&info);
        gst_video_info_set_format(&info, GST_VIDEO_FORMAT_I420, 1280, height);
        info.colorimetry.matrix = GST_VIDEO_COLOR_MATRIX_UNKNOWN;
        info.colorimetry.range = GST_VIDEO_COLOR_RANGE_UNKNOWN;
        info.colorimetry.transfer = GST_VIDEO_TRANSFER_UNKNOWN;

        QVideoFrameFormat format(QSize(1280, height), QVideoFrameFormat::Format_YUV420P);
        applyColorimetry(format, info, nullptr);
        return format.colorSpace();
    };

    QCOMPARE(inferredColorSpace(576), QVideoFrameFormat::ColorSpace_BT601);
    QCOMPARE(inferredColorSpace(720), QVideoFrameFormat::ColorSpace_BT709);
}

void GStreamerTest::_testColorimetryTransferMapping()
{
    QCOMPARE(toQtColorTransfer(GST_VIDEO_TRANSFER_SRGB), QVideoFrameFormat::ColorTransfer_Gamma22);
    // Regression: BT601 used to fall through to BT709 — Qt's own backend maps it distinctly.
    QCOMPARE(toQtColorTransfer(GST_VIDEO_TRANSFER_BT601), QVideoFrameFormat::ColorTransfer_BT601);
    QCOMPARE(toQtColorTransfer(GST_VIDEO_TRANSFER_BT709), QVideoFrameFormat::ColorTransfer_BT709);
    QCOMPARE(toQtColorTransfer(GST_VIDEO_TRANSFER_BT2020_10), QVideoFrameFormat::ColorTransfer_BT709);
    QCOMPARE(toQtColorTransfer(GST_VIDEO_TRANSFER_SMPTE2084), QVideoFrameFormat::ColorTransfer_ST2084);
    QCOMPARE(toQtColorTransfer(GST_VIDEO_TRANSFER_ARIB_STD_B67), QVideoFrameFormat::ColorTransfer_STD_B67);
    QCOMPARE(toQtColorTransfer(GST_VIDEO_TRANSFER_GAMMA10), QVideoFrameFormat::ColorTransfer_Linear);
    QCOMPARE(toQtColorTransfer(GST_VIDEO_TRANSFER_GAMMA28), QVideoFrameFormat::ColorTransfer_Gamma28);
    QCOMPARE(toQtColorTransfer(GST_VIDEO_TRANSFER_SMPTE240M), QVideoFrameFormat::ColorTransfer_Gamma22);
    QCOMPARE(toQtColorTransfer(GST_VIDEO_TRANSFER_ADOBERGB), QVideoFrameFormat::ColorTransfer_Gamma22);
    QCOMPARE(toQtColorTransfer(GST_VIDEO_TRANSFER_LOG100), QVideoFrameFormat::ColorTransfer_Unknown);
}

void GStreamerTest::_testColorimetryColorRangeMapping()
{
    QCOMPARE(toQtColorRange(GST_VIDEO_COLOR_RANGE_0_255), QVideoFrameFormat::ColorRange_Full);
    QCOMPARE(toQtColorRange(GST_VIDEO_COLOR_RANGE_16_235), QVideoFrameFormat::ColorRange_Video);
    QCOMPARE(toQtColorRange(GST_VIDEO_COLOR_RANGE_UNKNOWN), QVideoFrameFormat::ColorRange_Unknown);
}

void GStreamerTest::_testPixelFormatAcceptedButNotAdvertised()
{
    QCOMPARE(toQtPixelFormat(GST_VIDEO_FORMAT_BGRx), QVideoFrameFormat::Format_BGRX8888);
    QCOMPARE(toQtPixelFormat(GST_VIDEO_FORMAT_RGBx), QVideoFrameFormat::Format_RGBX8888);
    QCOMPARE(toQtPixelFormat(GST_VIDEO_FORMAT_ARGB), QVideoFrameFormat::Format_ARGB8888);
    QCOMPARE(toQtPixelFormat(GST_VIDEO_FORMAT_xRGB), QVideoFrameFormat::Format_XRGB8888);

    QCOMPARE(toQtPixelFormat(GST_VIDEO_FORMAT_BGR), QVideoFrameFormat::Format_Invalid);
    QCOMPARE(toQtPixelFormat(GST_VIDEO_FORMAT_RGB), QVideoFrameFormat::Format_Invalid);
    QCOMPARE(toQtPixelFormat(GST_VIDEO_FORMAT_UNKNOWN), QVideoFrameFormat::Format_Invalid);
}

void GStreamerTest::_testAdvertisedFormatListMatchesTable()
{
    const std::string list = GstQgc::advertisedFormatList();
    for (const auto& e : GstQgc::kVideoFormatTable) {
        if (e.capsToken) {
            QVERIFY2(
                list.find(e.capsToken) != std::string::npos,
                qPrintable(
                    QStringLiteral("advertised token '%1' missing from list").arg(QString::fromUtf8(e.capsToken))));
        }
    }
    QVERIFY2(list.find("BGRx") == std::string::npos, "accepted-only BGRx must not be advertised");
    QVERIFY2(list.find("I420_10LE") == std::string::npos, "accepted-only I420_10LE must not be advertised");
    QVERIFY2(list.rfind("{ ", 0) == 0 && list.find(" }") != std::string::npos, "list must be brace-wrapped");
}

void GStreamerTest::_testAdvertisedFormatListForOffer_data()
{
    QTest::addColumn<uint>("offer");
    QTest::addColumn<QString>("expected");

    // Multi-plane GLMemory renders green through Qt's GL import, so glupload must stay single-plane RGB.
    QTest::newRow("glupload") << uint(GstQgc::kOfferGlUpload) << QStringLiteral("{ BGRA, RGBA }");
    QTest::newRow("linear-dma-drm") << uint(GstQgc::kOfferLinearDmaDrm)
                                    << QStringLiteral("{ NV12, NV21, I420, P010_10LE, BGRA, RGBA }");
}

void GStreamerTest::_testAdvertisedFormatListForOffer()
{
    QFETCH(uint, offer);
    QFETCH(QString, expected);
    QCOMPARE(QString::fromStdString(GstQgc::advertisedFormatList(offer)), expected);
}

void GStreamerTest::_testColorimetryFrameRatePropagation()
{
    // Verify that a 30/1 caps framerate is surfaced on the delivered QVideoFrame.
    GstElementFactory* guardFactory = gst_element_factory_find("qgcvideosinkbin");
    if (!guardFactory) {
        GStreamer::completeInit();
    } else {
        gst_object_unref(guardFactory);
    }

    GError* error = nullptr;
    GstElement* pipeline = gst_parse_launch(
        "videotestsrc num-buffers=3 ! "
        "video/x-raw,format=I420,width=64,height=48,framerate=30/1 ! "
        "qgcvideosinkbin name=sink",
        &error);
    if (error) {
        const QString msg = QString::fromUtf8(error->message);
        g_clear_error(&error);
        QFAIL(qPrintable(QStringLiteral("Pipeline parse error: %1").arg(msg)));
    }
    QVERIFY2(pipeline, "Failed to create framerate pipeline");

    GstElement* sinkBin = gst_bin_get_by_name(GST_BIN(pipeline), "sink");
    QVERIFY2(sinkBin, "Could not find 'sink' element");

    QVideoSink videoSink;
    QObject controllerOwner;

    QVideoFrameFormat lastFormat;
    QObject::connect(&videoSink, &QVideoSink::videoFrameChanged, &controllerOwner,
                     [&](const QVideoFrame& frame) { lastFormat = frame.surfaceFormat(); });

    QVERIFY2(GStreamer::setupQVideoSinkElement(sinkBin, &videoSink, &controllerOwner),
             "GStreamer::setupQVideoSinkElement() failed");
    gst_object_unref(sinkBin);

    QVERIFY2(gst_element_set_state(pipeline, GST_STATE_PLAYING) != GST_STATE_CHANGE_FAILURE, "Pipeline failed to PLAY");

    GstBus* bus = gst_element_get_bus(pipeline);
    GstMessage* msg = gst_bus_timed_pop_filtered(bus, 10 * GST_SECOND,
                                                 static_cast<GstMessageType>(GST_MESSAGE_EOS | GST_MESSAGE_ERROR));
    QVERIFY2(msg, "Pipeline timed out");
    const bool isError = (GST_MESSAGE_TYPE(msg) == GST_MESSAGE_ERROR);
    gst_message_unref(msg);
    gst_object_unref(bus);
    if (isError) {
        gst_element_set_state(pipeline, GST_STATE_NULL);
        gst_object_unref(pipeline);
        QFAIL("Framerate pipeline errored");
    }

    QTRY_VERIFY_WITH_TIMEOUT(lastFormat.isValid(), TestTimeout::mediumMs());
    QCOMPARE(lastFormat.streamFrameRate(), 30.0);

    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
}

void GStreamerTest::_testApplyOrientationToFrameMapping()
{
#ifndef QGC_HAS_GST_VIDEO_ORIENTATION_META
    QSKIP("GStreamer build lacks GstVideoOrientationMeta");
#else
    // Verifies each GStreamer orientation enum maps to the correct (rotation, mirrored) tuple
    // on QVideoFrame. Lock-down test: prior versions had subtle mismatches between gst's
    // diagonal-flip semantics and Qt's rotate-then-mirror composition.
    struct Case
    {
        GstVideoOrientationMethod gst;
        QtVideo::Rotation expectedRot;
        bool expectedMirrored;
    };

    const Case cases[] = {
        {GST_VIDEO_ORIENTATION_IDENTITY, QtVideo::Rotation::None, false},
        {GST_VIDEO_ORIENTATION_90R, QtVideo::Rotation::Clockwise90, false},
        {GST_VIDEO_ORIENTATION_180, QtVideo::Rotation::Clockwise180, false},
        {GST_VIDEO_ORIENTATION_90L, QtVideo::Rotation::Clockwise270, false},
        {GST_VIDEO_ORIENTATION_HORIZ, QtVideo::Rotation::None, true},
        {GST_VIDEO_ORIENTATION_VERT, QtVideo::Rotation::Clockwise180, true},
        {GST_VIDEO_ORIENTATION_UL_LR, QtVideo::Rotation::Clockwise90, true},
        {GST_VIDEO_ORIENTATION_UR_LL, QtVideo::Rotation::Clockwise270, true},
    };
    for (const Case& c : cases) {
        QVideoFrame frame = QVideoFrame(QVideoFrameFormat(QSize(2, 2), QVideoFrameFormat::Format_BGRA8888));
        // Pre-poison so a no-op switch case (default branch) wouldn't accidentally match.
        frame.setRotation(QtVideo::Rotation::Clockwise90);
        frame.setMirrored(true);
        applyOrientationToFrame(frame, c.gst);
        QCOMPARE(frame.rotation(), c.expectedRot);
        QCOMPARE(frame.mirrored(), c.expectedMirrored);
    }
#endif
}

void GStreamerTest::_testAdapterFlushDropsInFlightSamples()
{
    // Push GST_EVENT_FLUSH_START into qgcqvideosink mid-stream; verify no buffer reaches show_frame while flushing and
    // that, after FLUSH_STOP, every buffer the sink saw is accounted for as delivered or dropped. Counts come from the
    // element: latest-frame-only delivery coalesces videoFrameChanged emissions, so those are not a frame count.
    GStreamer::redirectGLibLogging();
    QVERIFY2(GStreamer::completeInit(), "completeInit failed");

    QVideoSink sink;
    QObject controllerOwner;

    GError* err = nullptr;
    // identity is_live=false to avoid the live-source latency offset; videotestsrc → identity → qgcvideosinkbin.
    GstElement* pipeline = gst_parse_launch(
        "videotestsrc num-buffers=10 ! "
        "video/x-raw,format=BGRA,width=64,height=48,framerate=30/1 ! "
        "identity name=id ! "
        "qgcvideosinkbin name=sink",
        &err);
    if (err) {
        g_clear_error(&err);
    }
    QVERIFY2(pipeline, "Pipeline construction failed");
    GstElement* sinkBin = gst_bin_get_by_name(GST_BIN(pipeline), "sink");
    QVERIFY(sinkBin);
    QVERIFY2(GStreamer::setupQVideoSinkElement(sinkBin, &sink, &controllerOwner),
             "GStreamer::setupQVideoSinkElement() failed");
    gst_object_unref(sinkBin);

    GstElement* vsink = gst_bin_get_by_name(GST_BIN(pipeline), "qgcqvideosink");
    QVERIFY(vsink);

    QVERIFY(gst_element_set_state(pipeline, GST_STATE_PLAYING) != GST_STATE_CHANGE_FAILURE);
    // Wait for a few frames to confirm baseline delivery works.
    QVERIFY_TRUE_WAIT(framesDelivered(vsink) > 0, TestTimeout::mediumMs());

    // Send FLUSH_START upstream of qgcqvideosink. identity element forwards events.
    GstElement* id = gst_bin_get_by_name(GST_BIN(pipeline), "id");
    QVERIFY(id);
    GstPad* idSrc = gst_element_get_static_pad(id, "src");
    QVERIFY(idSrc);
    GstPad* flushStartPeer = gst_pad_get_peer(idSrc);
    QVERIFY(flushStartPeer);
    const gboolean flushStartSent = gst_pad_send_event(flushStartPeer, gst_event_new_flush_start());
    gst_object_unref(flushStartPeer);
    QVERIFY(flushStartSent);

    // Snapshot the accounting just after FLUSH_START. frames-input counts every buffer show_frame saw;
    // frames-delivered counts those handed to the QVideoSink.
    guint64 inputBefore = 0, deliveredBefore = 0;
    g_object_get(vsink, "frames-input", &inputBefore, "frames-delivered", &deliveredBefore, nullptr);

    // During FLUSH_START the base sink rejects buffers before show_frame, so neither counter may move. Negative
    // assertion: hold a bounded window open while pumping the event loop.
    {
        QElapsedTimer flushWindow;
        flushWindow.start();
        while (flushWindow.elapsed() < 150) {
            QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
        }
    }
    guint64 inputDuring = 0, deliveredDuring = 0;
    g_object_get(vsink, "frames-input", &inputDuring, "frames-delivered", &deliveredDuring, nullptr);
    QCOMPARE(deliveredDuring, deliveredBefore);
    QCOMPARE(inputDuring, inputBefore);

    // Send FLUSH_STOP — restore normal flow. videotestsrc may have emitted EOS by now,
    // so we don't strictly require new frames, just that the in-flight set was discarded.
    GstPad* flushStopPeer = gst_pad_get_peer(idSrc);
    QVERIFY(flushStopPeer);
    const gboolean flushStopSent = gst_pad_send_event(flushStopPeer, gst_event_new_flush_stop(/*reset_time=*/TRUE));
    gst_object_unref(flushStopPeer);
    QVERIFY(flushStopSent);

    // The flush must leave no buffer stuck in flight: every buffer the sink accepted is accounted for as either
    // delivered or dropped. A leaked in-flight buffer would keep input strictly above delivered+dropped.
    const auto accountedFor = [vsink]() {
        guint64 input = 0, delivered = 0, dropped = 0;
        g_object_get(vsink, "frames-input", &input, "frames-delivered", &delivered, "frames-dropped", &dropped,
                     nullptr);
        return input == delivered + dropped;
    };
    QVERIFY_TRUE_WAIT(accountedFor(), TestTimeout::shortMs());
    gst_object_unref(vsink);

    gst_object_unref(idSrc);
    gst_object_unref(id);
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
}

void GStreamerTest::_testQgcQVideoSinkDeliversLatestFrameOnly()
{
    QVERIFY2(GStreamer::completeInit(), "completeInit failed");

    int emissions = 0;
    qint64 lastStartUs = -1;
    QVideoSink videoSink;
    QObject::connect(&videoSink, &QVideoSink::videoFrameChanged, &videoSink, [&](const QVideoFrame& frame) {
        ++emissions;
        lastStartUs = frame.startTime();
    });
    QgcQVideoSinkHarness harness(&videoSink);
    QVERIFY(harness.isReady());
    QVERIFY(harness.start());

    // Three frames while the GUI thread is "stalled": all are handed off, but only the newest is shown.
    for (int i = 1; i <= 3; ++i) {
        QCOMPARE(harness.pushFrame(i * GST_SECOND), GST_FLOW_OK);
    }
    QCOMPARE(harness.delivered(), guint64(3));
    QCoreApplication::sendPostedEvents(&videoSink, QEvent::MetaCall);
    QCOMPARE(emissions, 1);
    QCOMPARE(lastStartUs, qint64(3 * GST_SECOND / GST_USECOND));

    // Delivery re-arms once the GUI side has taken the frame.
    QCOMPARE(harness.pushFrame(4 * GST_SECOND), GST_FLOW_OK);
    QCoreApplication::sendPostedEvents(&videoSink, QEvent::MetaCall);
    QCOMPARE(emissions, 2);
    QCOMPARE(lastStartUs, qint64(4 * GST_SECOND / GST_USECOND));
}

void GStreamerTest::_testQgcQVideoSinkShowsPrerollBufferOnce()
{
    QVERIFY2(GStreamer::completeInit(), "completeInit failed");

    QVideoSink videoSink;
    QgcQVideoSinkHarness harness(&videoSink);
    QVERIFY(harness.isReady());
    QVERIFY(harness.start());

    // The first buffer is both the preroll buffer and the first rendered one.
    QCOMPARE(harness.pushFrame(GST_SECOND), GST_FLOW_OK);
    guint64 input = 0;
    g_object_get(harness.element(), "frames-input", &input, nullptr);
    QCOMPARE(input, guint64(1));
    QCOMPARE(harness.delivered(), guint64(1));
}

void GStreamerTest::_testQgcQVideoSinkRenegotiatesMidStream()
{
    QVERIFY2(GStreamer::completeInit(), "completeInit failed");

    QVideoSink videoSink;
    QgcQVideoSinkHarness harness(&videoSink);
    QVERIFY(harness.isReady());
    QVERIFY(harness.start());
    QCOMPARE(harness.pushFrame(GST_SECOND), GST_FLOW_OK);
    QCoreApplication::sendPostedEvents(&videoSink, QEvent::MetaCall);
    QCOMPARE(videoSink.videoFrame().pixelFormat(), QVideoFrameFormat::Format_BGRA8888);

    // Same byte size as the first format, so a frame mapped with stale caps would still map and show the old layout.
    GstCaps* const caps = gst_caps_new_simple("video/x-raw", "format", G_TYPE_STRING, "RGBA", "width", G_TYPE_INT, 8,
                                              "height", G_TYPE_INT, 2, "framerate", GST_TYPE_FRACTION, 30, 1, nullptr);
    QVERIFY(harness.pushEvent(gst_event_new_caps(caps)));
    gst_caps_unref(caps);
    QCOMPARE(harness.pushFrame(2 * GST_SECOND), GST_FLOW_OK);
    QCoreApplication::sendPostedEvents(&videoSink, QEvent::MetaCall);

    const QVideoFrame frame = videoSink.videoFrame();
    QCOMPARE(frame.pixelFormat(), QVideoFrameFormat::Format_RGBA8888);
    QCOMPARE(frame.size(), QSize(8, 2));
}

void GStreamerTest::_testQgcQVideoSinkAppliesCropMeta()
{
    QVERIFY2(GStreamer::completeInit(), "completeInit failed");

    QVideoSink videoSink;
    QgcQVideoSinkHarness harness(&videoSink);
    QVERIFY(harness.isReady());
    QVERIFY(harness.start());

    GstBuffer* const buffer = QgcQVideoSinkHarness::makeFrame(GST_SECOND);
    GstVideoCropMeta* const crop = gst_buffer_add_video_crop_meta(buffer);
    crop->x = 1;
    crop->y = 1;
    crop->width = 2;
    crop->height = 3;
    QCOMPARE(harness.pushBuffer(buffer), GST_FLOW_OK);
    QCoreApplication::sendPostedEvents(&videoSink, QEvent::MetaCall);

    const QVideoFrameFormat format = videoSink.videoFrame().surfaceFormat();
    QCOMPARE(format.viewport(), QRect(1, 1, 2, 3));
    QCOMPARE(format.frameSize(), QSize(QgcQVideoSinkHarness::kWidth, QgcQVideoSinkHarness::kHeight));
}

void GStreamerTest::_testQgcQVideoSinkStatsArePerSink()
{
    QVERIFY2(GStreamer::completeInit(), "completeInit failed");

    QVideoSink videoSinkA;
    QVideoSink videoSinkB;
    QgcQVideoSinkHarness harnessA(&videoSinkA);
    QgcQVideoSinkHarness harnessB(&videoSinkB);
    QVERIFY(harnessA.isReady() && harnessB.isReady());
    QVERIFY(harnessA.start() && harnessB.start());

    QCOMPARE(harnessA.pushFrame(GST_SECOND), GST_FLOW_OK);
    QCOMPARE(harnessA.pushFrame(2 * GST_SECOND), GST_FLOW_OK);
    QCOMPARE(harnessB.pushFrame(GST_SECOND), GST_FLOW_OK);

    const char* const cpuField = GstHwPathTelemetry::sinkFramesField(HwVideoBufferPath::None);
    QCOMPARE(sinkStat(harnessA.element(), cpuField), qint64(2));
    QCOMPARE(sinkStat(harnessB.element(), cpuField), qint64(1));
    QCOMPARE(sinkStat(harnessB.element(), "frames-delivered"), qint64(1));

    GstStructure* rawStats = nullptr;
    g_object_get(harnessA.element(), "stats", &rawStats, nullptr);
    const GStreamer::GstStructurePtr stats = GStreamer::adoptStructure(rawStats);
    QVERIFY(stats);
    const HwBuffers::PathStats formatted = HwBuffers::formatSinkStats(stats.get());
    QCOMPARE(formatted.line, QStringLiteral(" CPU:2"));
    QCOMPARE(formatted.totalDelivered, quint64(0));

    // A receiver reuses its sink across sessions and reports each session from its start.
    QCOMPARE(harnessA.pushFrame(3 * GST_SECOND), GST_FLOW_OK);
    GstStructure* rawLater = nullptr;
    g_object_get(harnessA.element(), "stats", &rawLater, nullptr);
    const GStreamer::GstStructurePtr later = GStreamer::adoptStructure(rawLater);
    QVERIFY(later);
    QCOMPARE(HwBuffers::formatSinkStats(later.get(), stats.get()).line, QStringLiteral(" CPU:1"));
}

void GStreamerTest::_testQgcQVideoSinkGpuPathSelection_data()
{
    QTest::addColumn<bool>("gpuZeroCopy");
    QTest::addColumn<QString>("memoryFeature");
    QTest::addColumn<bool>("fdMemory");
    QTest::addColumn<qint64>("expectedGpuFallbacks");
    QTest::addColumn<quint64>("expectedNoPathVerdicts");

    // Every row pushes three buffers, in system memory unless fdMemory.
    QTest::newRow("gpu-zerocopy-off") << false << QStringLiteral("memory:DMABuf") << false << qint64(0) << quint64(0);
    // Software-decoded frames never enter path selection.
    QTest::newRow("system-memory-caps") << true << QString() << false << qint64(0) << quint64(0);
    // Other memory under plain video/x-raw caps (vtdec's CoreVideo buffers, V4L2 dmabuf export) still tries the GPU
    // paths: the first frame finds no path for this allocator, the rest reuse that verdict.
    QTest::newRow("system-memory-caps-fd-buffers") << true << QString() << true << qint64(3) << quint64(1);
    QTest::newRow("gpu-caps-system-buffers")
        << true << QStringLiteral("memory:DMABuf") << false << qint64(3) << quint64(1);
}

void GStreamerTest::_testQgcQVideoSinkGpuPathSelection()
{
#if !defined(QGC_HAS_ANY_GPU_PATH)
    QSKIP("No GPU path compiled in this build");
#else
    QFETCH(bool, gpuZeroCopy);
    QFETCH(QString, memoryFeature);
    QFETCH(bool, fdMemory);
    QFETCH(qint64, expectedGpuFallbacks);
    QFETCH(quint64, expectedNoPathVerdicts);
#if !defined(Q_OS_LINUX) || defined(Q_OS_ANDROID)
    if (fdMemory) {
        QSKIP("fd-backed test buffers need memfd (desktop Linux)");
    }
#endif
    QVERIFY2(GStreamer::completeInit(), "completeInit failed");

    QVideoSink videoSink;
    QgcQVideoSinkHarness harness(&videoSink, gpuZeroCopy);
    QVERIFY(harness.isReady());
    const QByteArray feature = memoryFeature.toUtf8();
    QVERIFY(harness.start(feature.isEmpty() ? nullptr : feature.constData()));

    using GstHwPathTelemetry::HwFallbackReason;
    const quint64 noPathBefore =
        GstHwPathTelemetry::peekFallbackReason(HwVideoBufferPath::None, HwFallbackReason::UnknownMemType);
    for (int i = 1; i <= 3; ++i) {
#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
        if (fdMemory) {
            GstBuffer* const buffer = QgcQVideoSinkHarness::makeFdFrame(i * GST_SECOND);
            QVERIFY(buffer);
            QCOMPARE(harness.pushBuffer(buffer), GST_FLOW_OK);
            continue;
        }
#endif
        QCOMPARE(harness.pushFrame(i * GST_SECOND), GST_FLOW_OK);
    }
    const quint64 noPathAfter =
        GstHwPathTelemetry::peekFallbackReason(HwVideoBufferPath::None, HwFallbackReason::UnknownMemType);

    QCOMPARE(sinkStat(harness.element(), GstHwPathTelemetry::sinkFramesField(HwVideoBufferPath::None)), qint64(3));
    QCOMPARE(sinkStat(harness.element(), "gpu-fallbacks"), expectedGpuFallbacks);
    QCOMPARE(sinkStat(harness.element(), "stream-demotions"), qint64(expectedGpuFallbacks > 0 ? 1 : 0));
    QCOMPARE(noPathAfter - noPathBefore, expectedNoPathVerdicts);
#endif
}

void GStreamerTest::_testQgcQVideoSinkDropsUndeliveredFrame_data()
{
    QTest::addColumn<bool>("viaFlush");
    QTest::newRow("flush-start") << true;
    QTest::newRow("stop") << false;
}

void GStreamerTest::_testQgcQVideoSinkDropsUndeliveredFrame()
{
    QFETCH(bool, viaFlush);
    QVERIFY2(GStreamer::completeInit(), "completeInit failed");

    int emissions = 0;
    QVideoSink videoSink;
    QObject::connect(&videoSink, &QVideoSink::videoFrameChanged, &videoSink, [&](const QVideoFrame&) { ++emissions; });
    QgcQVideoSinkHarness harness(&videoSink);
    QVERIFY(harness.isReady());
    QVERIFY(harness.start());

    QCOMPARE(harness.pushFrame(GST_SECOND), GST_FLOW_OK);
    if (viaFlush) {
        QVERIFY(harness.pushEvent(gst_event_new_flush_start()));
    } else {
        QCOMPARE(gst_element_set_state(harness.element(), GST_STATE_NULL), GST_STATE_CHANGE_SUCCESS);
    }
    QCoreApplication::sendPostedEvents(&videoSink, QEvent::MetaCall);
    QCOMPARE(emissions, 0);
    QVERIFY(!videoSink.videoFrame().isValid());
}

void GStreamerTest::_testQgcQVideoSinkPtsGuardResync_data()
{
    QTest::addColumn<QString>("boundary");
    QTest::addColumn<bool>("discont");
    QTest::addColumn<qint64>("nextPtsMs");
    QTest::addColumn<bool>("expectDelivered");

    // Baseline frame is at 10 s; 9.9 s is a small regression the guard drops unless a timeline boundary resets it.
    QTest::newRow("small-regression-dropped") << QString() << false << qint64(9900) << false;
    QTest::newRow("flush-resets") << QStringLiteral("flush") << false << qint64(9900) << true;
    QTest::newRow("segment-resets") << QStringLiteral("segment") << false << qint64(9900) << true;
    QTest::newRow("stream-start-resets") << QStringLiteral("stream-start") << false << qint64(9900) << true;
    QTest::newRow("discont-resyncs") << QString() << true << qint64(9900) << true;
    QTest::newRow("large-jump-resyncs") << QString() << false << qint64(0) << true;
}

void GStreamerTest::_testQgcQVideoSinkPtsGuardResync()
{
    QFETCH(QString, boundary);
    QFETCH(bool, discont);
    QFETCH(qint64, nextPtsMs);
    QFETCH(bool, expectDelivered);
    QVERIFY2(GStreamer::completeInit(), "completeInit failed");

    QVideoSink videoSink;
    QgcQVideoSinkHarness harness(&videoSink);
    QVERIFY(harness.isReady());
    QVERIFY(harness.start());
    QCOMPARE(harness.pushFrame(10 * GST_SECOND), GST_FLOW_OK);
    QCOMPARE(harness.delivered(), guint64(1));

    // Identical caps throughout, so GstBaseSink never re-runs set_caps.
    if (boundary == QStringLiteral("flush")) {
        QVERIFY(harness.pushEvent(gst_event_new_flush_start()));
        QVERIFY(harness.pushEvent(gst_event_new_flush_stop(/*reset_time=*/TRUE)));
        QVERIFY(harness.pushSegment());  // data after a flush requires a fresh segment
    } else if (boundary == QStringLiteral("segment")) {
        QVERIFY(harness.pushSegment());
    } else if (boundary == QStringLiteral("stream-start")) {
        QVERIFY(harness.pushEvent(gst_event_new_stream_start("qgc-harness-restart")));
    }

    QCOMPARE(harness.pushFrame(static_cast<GstClockTime>(nextPtsMs) * GST_MSECOND, discont), GST_FLOW_OK);
    QCOMPARE(harness.delivered(), guint64(expectDelivered ? 2 : 1));
}

void GStreamerTest::_testQgcQVideoSinkMapFailuresPostElementError()
{
    QVERIFY2(GStreamer::completeInit(), "completeInit failed");

    QVideoSink videoSink;
    QgcQVideoSinkHarness harness(&videoSink);
    QVERIFY(harness.isReady());
    QVERIFY(harness.start());

    // Periodic per-failure warnings are rate-limited by process-global counters, so their count is not fixed.
    ignoreLogMessage("Video.GStreamer.HwBuffers.CpuPool", QtWarningMsg,
                     QRegularExpression(QStringLiteral("gst_video_frame_map failed")));
    ignoreLogMessage("Video.GStreamer.QgcQVideoSink", QtWarningMsg,
                     QRegularExpression(QStringLiteral("mapping failed")));
    expectLogMessage("Video.GStreamer.QgcQVideoSink", QtWarningMsg,
                     QRegularExpression(QStringLiteral("consecutive map failures")));

    // Undersized buffers can never be mapped; the sink must eventually give up with a descriptive element error.
    GstFlowReturn ret = GST_FLOW_OK;
    for (int i = 0; (i < 1000) && (ret == GST_FLOW_OK); ++i) {
        ret = harness.pushFrame(GST_CLOCK_TIME_NONE, false, /*size=*/4);
    }
    verifyExpectedLogMessage();
    QCOMPARE(ret, GST_FLOW_ERROR);

    GstMessage* msg = gst_bus_pop_filtered(harness.bus(), GST_MESSAGE_ERROR);
    QVERIFY2(msg, "map-failure escalation must post an element error before returning GST_FLOW_ERROR");
    GError* error = nullptr;
    gchar* debug = nullptr;
    gst_message_parse_error(msg, &error, &debug);
    const bool streamFailed = g_error_matches(error, GST_STREAM_ERROR, GST_STREAM_ERROR_FAILED);
    const QString details = QString::fromUtf8(debug ? debug : "");
    g_clear_error(&error);
    g_free(debug);
    gst_message_unref(msg);
    QVERIFY(streamFailed);
    QVERIFY2(details.contains(QStringLiteral("consecutive map failures")), qPrintable(details));
}

void GStreamerTest::_testQVideoSinkControllerReleaseClearsVideoFrame()
{
    QVERIFY2(GStreamer::completeInit(), "completeInit failed");

    QVideoSink videoSink;
    QgcQVideoSinkHarness harness(nullptr);
    QVERIFY(harness.isReady());
    QVERIFY(harness.start());
    QObject controllerOwner;
    auto* controller = new QGCQVideoSinkController(harness.element(), &controllerOwner);
    controller->setVideoSink(QPointer<QVideoSink>(&videoSink));

    QCOMPARE(harness.pushFrame(GST_SECOND), GST_FLOW_OK);
    QCoreApplication::sendPostedEvents(&videoSink, QEvent::MetaCall);
    QVERIFY(videoSink.videoFrame().isValid());

    // The released sink must not keep the last GstSample-backed frame alive.
    controller->prepareForRelease();
    QVERIFY(!videoSink.videoFrame().isValid());
}

#endif
