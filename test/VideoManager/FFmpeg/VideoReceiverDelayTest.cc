#include "VideoReceiverDelayTest.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <gst/gst.h>
#include <memory>
#include <mutex>
#include <optional>
#include <vector>

#include <QtCore/QFile>
#include <QtGui/QImage>
#include <QtMultimedia/QVideoFrame>
#include <QtMultimedia/QVideoSink>
#include <QtMultimediaQuick/private/qquickvideooutput_p.h>
#include <QtQuick/QQuickWindow>

#include "FFmpegVideoReceiver.h"
#include "GstScoped.h"
#include "GstVideoReceiver.h"
#include "QtMultimedia/QtMultimediaReceiverTestSupport.h"
#include "QtMultimediaVideoReceiver.h"
#include "VideoBackend.h"
#include "VideoDecoderOptions.h"
#include "VideoReceiver.h"
#include "VideoStreamStats.h"
#include "VideoTestHelpers.h"

namespace {

enum class DelayBackend
{
    GStreamer,
    QtMultimedia,
    FFmpeg,
};

constexpr int kDelayWidth = 1280;
constexpr int kDelayHeight = 720;
/// The time is written as 40 bits and an 8-bit checksum, one bit to a 32-pixel square of black or white luma, in two
/// rows of 24: squares on macroblock boundaries survive the encoder.
constexpr int kStampBits = 48;
constexpr int kStampColumns = 24;
constexpr int kStampSquare = 32;
constexpr int kStampLeft = 128;
constexpr int kStampTop = 32;
constexpr int kSettleFrames = 30;
constexpr int kMeasuredFrames = 300;
/// The first start meets the sender's first keyframe; the others wait for the next one, up to the second between two.
constexpr int kStarts = 6;

/// Microseconds on the steady clock since the test began: the sender's and the receivers' clock.
qint64 delayNowUs()
{
    static const auto origin = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - origin).count();
}

quint8 stampChecksum(quint64 value)
{
    quint8 checksum = 0x5a;
    for (int i = 0; i < 5; ++i) {
        checksum ^= static_cast<quint8>(value >> (8 * i));
    }
    return checksum;
}

QPoint stampSquare(int bit)
{
    return {kStampLeft + ((bit % kStampColumns) * kStampSquare), kStampTop + ((bit / kStampColumns) * kStampSquare)};
}

void paintStamp(uint8_t* luma, int stride, quint64 value)
{
    const quint64 word = ((value & 0xFFFFFFFFFFull) << 8) | stampChecksum(value & 0xFFFFFFFFFFull);
    for (int bit = 0; bit < kStampBits; ++bit) {
        const bool on = ((word >> (kStampBits - 1 - bit)) & 1u) != 0;
        const QPoint square = stampSquare(bit);
        for (int y = 0; y < kStampSquare; ++y) {
            std::memset(luma + (static_cast<ptrdiff_t>(square.y() + y) * stride) + square.x(), on ? 235 : 16,
                        kStampSquare);
        }
    }
}

/// The time written into a frame whose luma @p lumaAt gives, for a picture of @p size; nullopt where it doesn't read.
template <typename LumaAt>
std::optional<qint64> readStamp(QSize size, LumaAt lumaAt)
{
    const double scaleX = size.width() / double(kDelayWidth);
    const double scaleY = size.height() / double(kDelayHeight);
    quint64 word = 0;
    for (int bit = 0; bit < kStampBits; ++bit) {
        const QPoint square = stampSquare(bit);
        int sum = 0;
        for (int dy = -4; dy < 4; ++dy) {
            for (int dx = -4; dx < 4; ++dx) {
                sum += lumaAt(int((square.x() + (kStampSquare / 2) + dx) * scaleX),
                              int((square.y() + (kStampSquare / 2) + dy) * scaleY));
            }
        }
        word = (word << 1) | ((sum / 64 > 128) ? 1u : 0u);
    }
    const quint64 value = word >> 8;
    if (stampChecksum(value) != static_cast<quint8>(word & 0xFF)) {
        return std::nullopt;
    }
    return static_cast<qint64>(value);
}

std::optional<qint64> readFrameStamp(const QVideoFrame& shown)
{
    QVideoFrame frame(shown);
    const QVideoFrameFormat::PixelFormat format = frame.pixelFormat();
    const bool lumaFirst8 = (format == QVideoFrameFormat::Format_YUV420P) ||
                            (format == QVideoFrameFormat::Format_YUV422P) ||
                            (format == QVideoFrameFormat::Format_YV12) || (format == QVideoFrameFormat::Format_NV12) ||
                            (format == QVideoFrameFormat::Format_NV21) || (format == QVideoFrameFormat::Format_Y8);
    if (lumaFirst8 && frame.map(QVideoFrame::ReadOnly)) {
        const uchar* const bits = frame.bits(0);
        const int stride = frame.bytesPerLine(0);
        const std::optional<qint64> stamp =
            readStamp(frame.size(), [bits, stride](int x, int y) { return int(bits[(y * stride) + x]); });
        frame.unmap();
        return stamp;
    }
    const QImage image = frame.toImage();
    if (image.isNull()) {
        return std::nullopt;
    }
    return readStamp(image.size(), [&image](int x, int y) { return qGray(image.pixel(x, y)); });
}

/// Sends the stamped test stream as H.264 RTP to a port of loopback.
class DelaySender
{
public:
    ~DelaySender() { stop(); }

    static bool available() { return GStreamer::hasElementFactory("x264enc"); }

    bool start(quint16 port)
    {
        const QByteArray launch = QStringLiteral(
                                      "videotestsrc is-live=true pattern=black ! "
                                      "video/x-raw,format=I420,width=%1,height=%2,framerate=30/1 ! "
                                      "x264enc name=encoder tune=zerolatency speed-preset=ultrafast "
                                      "bitrate=2000 key-int-max=30 ! rtph264pay config-interval=1 pt=96 ! "
                                      "udpsink host=127.0.0.1 port=%3 sync=false async=false")
                                      .arg(kDelayWidth)
                                      .arg(kDelayHeight)
                                      .arg(port)
                                      .toLatin1();
        GError* error = nullptr;
        _pipeline = gst_parse_launch(launch.constData(), &error);
        if (error) {
            g_error_free(error);
        }
        if (!_pipeline) {
            return false;
        }
        GstElement* const encoder = gst_bin_get_by_name(GST_BIN(_pipeline), "encoder");
        GstPad* const pad = encoder ? gst_element_get_static_pad(encoder, "sink") : nullptr;
        if (pad) {
            (void) gst_pad_add_probe(pad, GST_PAD_PROBE_TYPE_BUFFER, &DelaySender::_stamp, nullptr, nullptr);
            gst_object_unref(pad);
        }
        if (encoder) {
            gst_object_unref(encoder);
        }
        return pad && (gst_element_set_state(_pipeline, GST_STATE_PLAYING) != GST_STATE_CHANGE_FAILURE);
    }

    void stop()
    {
        if (_pipeline) {
            (void) gst_element_set_state(_pipeline, GST_STATE_NULL);
            gst_object_unref(_pipeline);
            _pipeline = nullptr;
        }
    }

private:
    static GstPadProbeReturn _stamp(GstPad*, GstPadProbeInfo* info, gpointer)
    {
        GstBuffer* const buffer = gst_buffer_make_writable(GST_PAD_PROBE_INFO_BUFFER(info));
        GST_PAD_PROBE_INFO_DATA(info) = buffer;
        GstMapInfo map;
        if (gst_buffer_map(buffer, &map, GST_MAP_WRITE)) {
            paintStamp(map.data, kDelayWidth, static_cast<quint64>(delayNowUs()));
            gst_buffer_unmap(buffer, &map);
        }
        return GST_PAD_PROBE_OK;
    }

    GstElement* _pipeline = nullptr;
};

/// The frames a QVideoSink is given, as the thread that gives them sees them.
struct DelaySamples
{
    struct Sample
    {
        qint64 sentUs = 0;
        qint64 givenUs = 0;
        /// Once the frame's pixels were read: for a frame in GPU memory, after it was copied out.
        qint64 readUs = 0;
    };

    void onFrame(const QVideoFrame& frame)
    {
        const qint64 givenUs = delayNowUs();
        if (!frame.isValid()) {
            return;
        }
        const std::optional<qint64> sentUs = readFrameStamp(frame);
        const qint64 readUs = delayNowUs();
        const std::lock_guard<std::mutex> lock(mutex);
        pixelFormat = frame.pixelFormat();
        if (!sentUs) {
            ++unreadable;
            return;
        }
        samples.push_back({*sentUs, givenUs, readUs});
    }

    qsizetype count()
    {
        const std::lock_guard<std::mutex> lock(mutex);
        return static_cast<qsizetype>(samples.size());
    }

    std::mutex mutex;
    std::vector<Sample> samples;
    int unreadable = 0;
    QVideoFrameFormat::PixelFormat pixelFormat = QVideoFrameFormat::Format_Invalid;
};

double percentile(std::vector<double> values, double fraction)
{
    std::sort(values.begin(), values.end());
    const auto index = static_cast<size_t>(std::ceil(fraction * double(values.size()))) - 1;
    return values.at(std::min(index, values.size() - 1));
}

QString joinedMs(const QList<double>& values)
{
    QStringList texts;
    for (const double value : values) {
        texts.append(QString::number(value, 'f', 0));
    }
    return texts.join(QStringLiteral(", "));
}

bool delaySoftwareDecoding()
{
    return qEnvironmentVariable("QGC_DELAY_DECODING") == QStringLiteral("software");
}

std::unique_ptr<VideoReceiver> createDelayReceiver(DelayBackend backend)
{
    VideoReceiver::DisplayConfig display;
    // The frames are read in memory: the GStreamer receiver's GPU paths would hand over textures.
    display.gpuZeroCopy = (backend == DelayBackend::FFmpeg) && !delaySoftwareDecoding();
    std::unique_ptr<VideoReceiver> receiver;
    switch (backend) {
        case DelayBackend::GStreamer:
            receiver = std::make_unique<GstVideoReceiver>();
            break;
        case DelayBackend::QtMultimedia:
            receiver = std::make_unique<QtMultimediaVideoReceiver>();
            break;
        case DelayBackend::FFmpeg:
            receiver = std::make_unique<FFmpegVideoReceiver>();
            break;
    }
    receiver->setDisplayConfig(display);
    return receiver;
}

/// One receiver playing the stream at @p port into a VideoOutput in @p window, with the frames its sink is given.
struct DelayRun
{
    DelayRun(DelayBackend backend, bool lowLatency, quint16 port, QQuickWindow& window)
        : output(new QQuickVideoOutput(window.contentItem()))
        , receiver(createDelayReceiver(backend))
    {
        output->setSize(window.size());
        (void) QObject::connect(
            output->videoSink(), &QVideoSink::videoFrameChanged, &context,
            [this](const QVideoFrame& frame) { frames.onFrame(frame); }, Qt::DirectConnection);
        receiver->setName(QStringLiteral("delay"));
        receiver->setConfig({.uri = QStringLiteral("udp://127.0.0.1:%1").arg(port),
                             .timeout = std::chrono::seconds(5),
                             .lowLatency = lowLatency,
                             .autoReconnect = false});
        receiver->setVideoOutput(output.get());
    }

    ~DelayRun()
    {
        (void) QObject::disconnect(output->videoSink(), nullptr, &context, nullptr);
        receiver.reset();
    }

    QObject context;
    DelaySamples frames;
    std::unique_ptr<QQuickVideoOutput> output;
    std::unique_ptr<VideoReceiver> receiver;
};

const char* backendName(DelayBackend backend)
{
    switch (backend) {
        case DelayBackend::GStreamer:
            return "GStreamer";
        case DelayBackend::QtMultimedia:
            return "Qt Multimedia";
        case DelayBackend::FFmpeg:
            return "FFmpeg";
    }
    return "";
}

}  // namespace

void VideoReceiverDelayTest::cleanup()
{
    VideoBackend::instance().applyDecoderPriorities(VideoDecoderOption::Default);
    GStreamerTestBase::cleanup();
}

void VideoReceiverDelayTest::_testDelay_data()
{
    QTest::addColumn<int>("backend");
    QTest::addColumn<bool>("lowLatency");

    QTest::newRow("GStreamer") << int(DelayBackend::GStreamer) << false;
    QTest::newRow("GStreamer, low latency") << int(DelayBackend::GStreamer) << true;
    QTest::newRow("Qt Multimedia") << int(DelayBackend::QtMultimedia) << false;
    QTest::newRow("Qt Multimedia, low latency") << int(DelayBackend::QtMultimedia) << true;
    QTest::newRow("FFmpeg") << int(DelayBackend::FFmpeg) << false;
    QTest::newRow("FFmpeg, low latency") << int(DelayBackend::FFmpeg) << true;
}

void VideoReceiverDelayTest::_testDelay()
{
    QFETCH(int, backend);
    QFETCH(bool, lowLatency);

    if (!DelaySender::available()) {
        QSKIP("This GStreamer install lacks x264enc");
    }
    const auto delayBackend = static_cast<DelayBackend>(backend);
    const bool software = delaySoftwareDecoding();
    if (software && (delayBackend == DelayBackend::QtMultimedia) &&
        !qEnvironmentVariableIsSet("QT_FFMPEG_DECODING_HW_DEVICE_TYPES")) {
        QSKIP(
            "Qt Multimedia decodes in software only with QT_FFMPEG_DECODING_HW_DEVICE_TYPES set as the process starts");
    }
    VideoBackend::instance().applyDecoderPriorities(software ? VideoDecoderOption::Software
                                                             : VideoDecoderOption::Default);
    if (delayBackend == DelayBackend::QtMultimedia) {
        for (const QtMultimediaTest::IgnoredLog& log : QtMultimediaTest::playerLogs()) {
            ignoreLogMessage(log.category, log.type, log.pattern);
        }
    }

    const quint16 port = VideoTestHelpers::freeUdpPort();
    DelaySender sender;
    QVERIFY(sender.start(port));
    QQuickWindow window;
    window.resize(640, 360);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    QList<double> firstFrameMs;
    /// The median delay of each start's first second.
    QList<double> startDelayMs;
    std::vector<double> givenMs;
    std::vector<double> readMs;
    QString decoder;
    QString pixelFormat;
    int unreadable = 0;
    for (int start = 0; start < kStarts; ++start) {
        DelayRun run(delayBackend, lowLatency, port, window);
        const qint64 startedUs = delayNowUs();
        run.receiver->setActive(true);
        QVERIFY(UnitTest::waitForCondition([&run] { return run.frames.count() > 0; }, TestTimeout::longDuration(),
                                           u"first frame"));
        {
            const std::lock_guard<std::mutex> lock(run.frames.mutex);
            firstFrameMs.append((run.frames.samples.front().givenUs - startedUs) / 1000.0);
        }
        // A delay that a start sets for the whole session shows in its first second.
        QVERIFY(UnitTest::waitForCondition([&run] { return run.frames.count() >= kSettleFrames; },
                                           TestTimeout::longDuration(), u"first second"));
        {
            const std::lock_guard<std::mutex> lock(run.frames.mutex);
            std::vector<double> firstSecond;
            for (int i = 0; i < kSettleFrames; ++i) {
                const DelaySamples::Sample& sample = run.frames.samples.at(static_cast<size_t>(i));
                firstSecond.push_back((sample.givenUs - sample.sentUs) / 1000.0);
            }
            startDelayMs.append(percentile(firstSecond, 0.5));
        }
        if (start < (kStarts - 1)) {
            continue;
        }

        QVERIFY(UnitTest::waitForCondition([&run] { return run.frames.count() >= (kSettleFrames + kMeasuredFrames); },
                                           TestTimeout::longDuration(), u"measured frames"));
        const std::lock_guard<std::mutex> lock(run.frames.mutex);
        for (int i = kSettleFrames; i < (kSettleFrames + kMeasuredFrames); ++i) {
            const DelaySamples::Sample& sample = run.frames.samples.at(static_cast<size_t>(i));
            givenMs.push_back((sample.givenUs - sample.sentUs) / 1000.0);
            readMs.push_back((sample.readUs - sample.sentUs) / 1000.0);
        }
        unreadable = run.frames.unreadable;
        pixelFormat = QVideoFrameFormat::pixelFormatToString(run.frames.pixelFormat);
        const VideoStreamStats stats = run.receiver->stats();
        decoder = stats.decoder + (stats.hardwareDecoder ? QStringLiteral(" (hardware)") : QString());
    }

    const QList<double> laterStarts = firstFrameMs.mid(1);
    const QString result =
        QStringLiteral(
            "DELAY | %1%2 | decoding asked: %3 | decoder: %4 | pixel format: %5 | first frame ms: %6, "
            "median of the later starts %12 | delay of each start's first second ms: %13 | delay to sink ms of the "
            "last start: median %7, p95 %8 | delay to pixels read ms: median %9, p95 %10 | unreadable frames: %11")
            .arg(QString::fromLatin1(backendName(delayBackend)),
                 lowLatency ? QStringLiteral(" low latency") : QString(),
                 software ? QStringLiteral("software") : QStringLiteral("default"), decoder, pixelFormat)
            .arg(joinedMs(firstFrameMs))
            .arg(percentile(givenMs, 0.5), 0, 'f', 1)
            .arg(percentile(givenMs, 0.95), 0, 'f', 1)
            .arg(percentile(readMs, 0.5), 0, 'f', 1)
            .arg(percentile(readMs, 0.95), 0, 'f', 1)
            .arg(unreadable)
            .arg(percentile(std::vector<double>(laterStarts.cbegin(), laterStarts.cend()), 0.5), 0, 'f', 0)
            .arg(joinedMs(startDelayMs));
    std::fprintf(stdout, "%s\n", qPrintable(result));
    std::fflush(stdout);
    const QString resultsFile = qEnvironmentVariable("QGC_DELAY_RESULTS");
    if (!resultsFile.isEmpty()) {
        QFile file(resultsFile);
        QVERIFY(file.open(QIODevice::Append | QIODevice::Text));
        (void) file.write(result.toUtf8() + '\n');
    }

    // The checks a machine's speed doesn't decide: every frame read, and no time from before it was sent.
    QCOMPARE(givenMs.size(), size_t(kMeasuredFrames));
    QVERIFY2(*std::min_element(givenMs.begin(), givenMs.end()) > 0, "a frame was given to the sink before it was sent");
    QVERIFY2(unreadable <= (kSettleFrames + kMeasuredFrames) / 20, qPrintable(QString::number(unreadable)));
}

UT_REGISTER_TEST(VideoReceiverDelayTest, TestLabel::Integration)
