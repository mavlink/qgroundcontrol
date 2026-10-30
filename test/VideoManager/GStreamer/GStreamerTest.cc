#include "GStreamerTest.h"

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GStreamerTestLog, "Video.GStreamer.GStreamerTest")

#ifdef QGC_GST_STREAMING

#include <gst/gst.h>
#include <memory>

#include "Fact.h"
#include "Fixtures/RAIIFixtures.h"
#include "GStreamer.h"
#include "GStreamerHelpers.h"
#include "GStreamerTestHelpers.h"
#include "GstVideoReceiver.h"
#include "PipelineDotDumper.h"
#include "RecordingBranch.h"

void GStreamerTest::_testIsValidRtspUri()
{
    QVERIFY(GStreamer::isValidRtspUri("rtsp://127.0.0.1:8554/test"));
    QVERIFY(GStreamer::isValidRtspUri("rtsp://user:pass@10.0.0.1/stream"));
    QVERIFY(GStreamer::isValidRtspUri("rtspu://192.168.1.1:554/video"));
    QVERIFY(GStreamer::isValidRtspUri("rtspt://example.com/live"));

    QVERIFY(!GStreamer::isValidRtspUri(nullptr));
    QVERIFY(!GStreamer::isValidRtspUri(""));
    QVERIFY(!GStreamer::isValidRtspUri("not-a-uri"));
    QVERIFY(!GStreamer::isValidRtspUri("http://example.com"));
    QVERIFY(!GStreamer::isValidRtspUri("udp://127.0.0.1:5600"));
    QVERIFY(!GStreamer::isValidRtspUri("rtsp://"));
}

void GStreamerTest::_testIsHardwareDecoderFactory_data()
{
    QTest::addColumn<QString>("factoryName");
    QTest::addColumn<bool>("expectedHardware");

    QTest::newRow("null-factory") << QString() << false;
    QTest::newRow("avdec_h264") << QStringLiteral("avdec_h264") << false;
    // Software decoders whose names merely contain a hardware family tag ("nv").
    QTest::newRow("avdec_wnv1") << QStringLiteral("avdec_wnv1") << false;
    QTest::newRow("avdec_dsicinvideo") << QStringLiteral("avdec_dsicinvideo") << false;
    QTest::newRow("vah264dec") << QStringLiteral("vah264dec") << true;
    QTest::newRow("vaapih264dec") << QStringLiteral("vaapih264dec") << true;
    QTest::newRow("vulkanh264dec") << QStringLiteral("vulkanh264dec") << true;
}

void GStreamerTest::_testIsHardwareDecoderFactory()
{
    QFETCH(QString, factoryName);
    QFETCH(bool, expectedHardware);

    if (factoryName.isEmpty()) {
        QCOMPARE(GStreamer::isHardwareDecoderFactory(nullptr), expectedHardware);
        return;
    }

    const GStreamer::GstFactoryPtr factory =
        GStreamer::adoptFactory(gst_element_factory_find(factoryName.toUtf8().constData()));
    if (!factory) {
        QSKIP(qPrintable(QStringLiteral("Decoder factory not installed: %1").arg(factoryName)));
    }
    QCOMPARE(GStreamer::isHardwareDecoderFactory(factory.get()), expectedHardware);
}

void GStreamerTest::_testDecoderFamilyForFactory_data()
{
    QTest::addColumn<QString>("factoryName");
    QTest::addColumn<int>("expectedFamily");

    QTest::newRow("nvidia") << QStringLiteral("nvh264dec") << int(GStreamer::ForceVideoDecoderNVIDIA);
    QTest::newRow("va") << QStringLiteral("vah265dec") << int(GStreamer::ForceVideoDecoderVAAPI);
    // Legacy gstreamer-vaapi is demoted and never bumped by the VAAPI option.
    QTest::newRow("legacy-vaapi") << QStringLiteral("vaapih264dec") << int(GStreamer::ForceVideoDecoderDefault);
    QTest::newRow("qsv") << QStringLiteral("qsvh265dec") << int(GStreamer::ForceVideoDecoderIntel);
    QTest::newRow("msdk") << QStringLiteral("msdkh264dec") << int(GStreamer::ForceVideoDecoderIntel);
    QTest::newRow("d3d11") << QStringLiteral("d3d11h264dec") << int(GStreamer::ForceVideoDecoderDirectX3D);
    QTest::newRow("d3d12") << QStringLiteral("d3d12h265dec") << int(GStreamer::ForceVideoDecoderDirectX3D);
    QTest::newRow("videotoolbox") << QStringLiteral("vtdec_hw") << int(GStreamer::ForceVideoDecoderVideoToolbox);
    QTest::newRow("vulkan") << QStringLiteral("vulkanh264dec") << int(GStreamer::ForceVideoDecoderVulkan);
    QTest::newRow("software-with-nv") << QStringLiteral("avdec_wnv1") << int(GStreamer::ForceVideoDecoderDefault);
    QTest::newRow("android-mediacodec") << QStringLiteral("amcviddec-c2qtiavcdecoder")
                                        << int(GStreamer::ForceVideoDecoderDefault);
}

void GStreamerTest::_testDecoderFamilyForFactory()
{
    QFETCH(QString, factoryName);
    QFETCH(int, expectedFamily);

    QCOMPARE(int(GStreamer::decoderFamilyForFactory(factoryName.toUtf8().constData())), expectedFamily);
}

void GStreamerTest::_testD3D12RhiDisablesGpuZeroCopySink()
{
#if !defined(Q_OS_WIN) || !defined(QGC_HAS_GST_D3D12_GPU_PATH)
    QSKIP("D3D12 sink policy is Windows D3D12-only");
#else
    const TestFixtures::EnvVarFixture rhiBackend("QSG_RHI_BACKEND");
    qputenv("QSG_RHI_BACKEND", QByteArray("d3d12"));

    QVERIFY2(!GStreamer::gpuZeroCopyAllowedForCurrentGraphicsApi(false, false),
             "D3D12 RHI must force the CPU sink path because gst-d3d12 cannot wrap Qt's ID3D12Device");
    QVERIFY(GStreamer::gpuZeroCopyAllowedForCurrentGraphicsApi(false, true) == false);
    QVERIFY(GStreamer::gpuZeroCopyAllowedForCurrentGraphicsApi(true, false) == false);

    qputenv("QSG_RHI_BACKEND", QByteArray("d3d11"));
    QVERIFY(GStreamer::gpuZeroCopyAllowedForCurrentGraphicsApi(false, false));
#endif
}

void GStreamerTest::_testPipelineDotOmitsElementProperties()
{
    const GStreamer::GstElementPtr pipeline =
        GStreamer::adoptElement(GST_ELEMENT(gst_object_ref_sink(gst_pipeline_new("safe-dot-test"))));

    GstElement* source = gst_element_factory_make("filesrc", "source");
    GstElement* sink = gst_element_factory_make("fakesink", "sink");
    QVERIFY(source);
    QVERIFY(sink);

    constexpr auto kSecretLocation = "/tmp/qgc-dot-secret-token";
    g_object_set(source, "location", kSecretLocation, nullptr);
    gst_bin_add_many(GST_BIN(pipeline.get()), source, sink, nullptr);
    QVERIFY(gst_element_link(source, sink));

    const GStreamer::GCharPtr dotData = GStreamer::adoptGChar(
        gst_debug_bin_to_dot_data(GST_BIN(pipeline.get()), PipelineDotDumper::kDiagnosticDotGraphDetails));
    QVERIFY(dotData);
    const QByteArray dot(dotData.get());

    QVERIFY(dot.contains("source"));
    QVERIFY(dot.contains("sink"));
    QVERIFY(!dot.contains(kSecretLocation));
    QVERIFY(!dot.contains("qgc-dot-secret-token"));
}

void GStreamerTest::_testCompleteInit()
{
    QVERIFY2(GStreamer::completeInit(), "GStreamer::completeInit() failed");

    GstRegistry* registry = gst_registry_get();
    QVERIFY(registry);

    GstPlugin* qgcPlugin = gst_registry_find_plugin(registry, "qgc");
    QVERIFY2(qgcPlugin, "Static plugin 'qgc' not registered after completeInit()");
    gst_clear_object(&qgcPlugin);

    for (const char* const name : {"appsink", "qgcvideosinkbin"}) {
        QVERIFY2(GStreamer::adoptFactory(gst_element_factory_find(name)),
                 qPrintable(QStringLiteral("Factory '%1' not found after completeInit()").arg(QLatin1String(name))));
    }
}

void GStreamerTest::_testCreateVideoReceiver()
{
    std::unique_ptr<VideoReceiver> receiver(GStreamer::createVideoReceiver(nullptr));
    QVERIFY2(receiver, "GStreamer::createVideoReceiver() returned nullptr");
    QVERIFY(qobject_cast<GstVideoReceiver*>(receiver.get()));
}

void GStreamerTest::_testRecordingSinkAcceptsElementaryStreams_data()
{
    QTest::addColumn<QString>("capsString");
    QTest::addColumn<int>("format");

    constexpr const char* h264Caps = "video/x-h264,stream-format=byte-stream,alignment=au";
    constexpr const char* h265Caps = "video/x-h265,stream-format=byte-stream,alignment=au";

    QTest::newRow("h264-mkv") << QString::fromLatin1(h264Caps) << static_cast<int>(VideoReceiver::FileFormat::Mkv);
    QTest::newRow("h264-mov") << QString::fromLatin1(h264Caps) << static_cast<int>(VideoReceiver::FileFormat::Mov);
    QTest::newRow("h264-mp4") << QString::fromLatin1(h264Caps) << static_cast<int>(VideoReceiver::FileFormat::Mp4);
    QTest::newRow("h265-mkv") << QString::fromLatin1(h265Caps) << static_cast<int>(VideoReceiver::FileFormat::Mkv);
    QTest::newRow("h265-mov") << QString::fromLatin1(h265Caps) << static_cast<int>(VideoReceiver::FileFormat::Mov);
    QTest::newRow("h265-mp4") << QString::fromLatin1(h265Caps) << static_cast<int>(VideoReceiver::FileFormat::Mp4);
}

void GStreamerTest::_testRecordingSinkAcceptsElementaryStreams()
{
    QFETCH(QString, capsString);
    QFETCH(int, format);

    const auto fileFormat = static_cast<VideoReceiver::FileFormat>(format);
    const char* parserFactory = capsString.startsWith(QLatin1String("video/x-h265")) ? "h265parse" : "h264parse";
    const char* muxerFactory = (fileFormat == VideoReceiver::FileFormat::Mkv)
                                   ? "matroskamux"
                                   : ((fileFormat == VideoReceiver::FileFormat::Mov) ? "qtmux" : "mp4mux");

    const QString missing = GstTest::firstMissingFactory({"capsfilter", parserFactory, "filesink", muxerFactory});
    if (!missing.isEmpty()) {
        QSKIP(qPrintable(QStringLiteral("Required GStreamer factory is unavailable: %1").arg(missing)));
    }

    const GStreamer::GstCapsPtr inputCaps = GStreamer::adoptCaps(gst_caps_from_string(capsString.toUtf8().constData()));
    QVERIFY(inputCaps);

    GstElement* const capsfilter = gst_element_factory_make("capsfilter", nullptr);
    QVERIFY(capsfilter);
    const GStreamer::GstElementPtr source = GStreamer::adoptElement(GST_ELEMENT(gst_object_ref_sink(capsfilter)));
    g_object_set(source.get(), "caps", inputCaps.get(), nullptr);

    const GStreamer::GstElementPtr sink =
        RecordingBranch::makeFileSink(QStringLiteral("recording-test"), fileFormat, inputCaps.get());
    QVERIFY(sink);

    QVERIFY2(gst_element_link(source.get(), sink.get()),
             qPrintable(QStringLiteral("Recording sink rejected %1").arg(capsString)));
}

void GStreamerTest::_testBindDebugLevelFactRejectsNullContext()
{
    Fact fact;
    GStreamer::bindDebugLevelFact(&fact, nullptr);
}

void GStreamerTest::_testRuntimeVersionCheck()
{
    guint major, minor, micro, nano;
    gst_version(&major, &minor, &micro, &nano);

    QVERIFY2(major == 1, "Unexpected GStreamer major version");
#if defined(Q_OS_LINUX)
    QVERIFY2(minor >= 20, qPrintable(QStringLiteral("GStreamer runtime version %1.%2.%3 is below minimum 1.20.0")
                                         .arg(major)
                                         .arg(minor)
                                         .arg(micro)));
#else
    QVERIFY2(minor >= 28, qPrintable(QStringLiteral("GStreamer runtime version %1.%2.%3 is below bundled SDK minimum 1.28.0")
                                         .arg(major)
                                         .arg(minor)
                                         .arg(micro)));
#endif

    if (major != GST_VERSION_MAJOR || minor != GST_VERSION_MINOR) {
        qCWarning(GStreamerTestLog) << "GStreamer version mismatch: built against" << GST_VERSION_MAJOR << "."
                                    << GST_VERSION_MINOR << "but runtime is" << major << "." << minor;
    }
}

#endif

UT_REGISTER_TEST(GStreamerTest, TestLabel::Unit)
