#pragma once

#ifdef QGC_GST_STREAMING

#include <atomic>
#include <chrono>
#include <gst/gst.h>
#include <gst/video/video-info.h>
#include <initializer_list>
#include <memory>
#include <mutex>
#include <utility>
#include <vector>

#include <QtCore/QByteArray>
#include <QtCore/QHash>
#include <QtCore/QList>
#include <QtCore/QRegularExpression>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtNetwork/QHostAddress>
#include <QtNetwork/QTcpServer>

#include "GstScoped.h"
#include "UnitTest.h"

class QRhi;
class QTcpSocket;

namespace GStreamer::SourceFactory {
struct Config;
}

/// Helpers shared by the GStreamer test classes.
namespace GstTest {

/// @p duration as a GstClockTime, for GStreamer calls that take a timeout.
inline GstClockTime clockTime(std::chrono::milliseconds duration)
{
    return static_cast<GstClockTime>(std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count());
}

/// Initializes GStreamer once per process; returns an empty string when it is usable, else why it isn't.
/// Skip on a non-empty result before UnitTest::init(): skipping in init() after it also skips cleanup().
QString initGStreamer();

/// The GLib type-registration criticals the GStreamer log bridge passes on: they depend on the order in which the
/// process registered its types, and come 0..N times.
inline QRegularExpression glibTypeRegistrationCriticals()
{
    return QRegularExpression(
        QStringLiteral("cannot register existing type|"
                       "g_type_add_interface_static.*G_TYPE_IS_INSTANTIATABLE|"
                       "g_once_init_leave.*result != 0"));
}

/// The first of @p names with no element factory registered, or an empty string.
QString firstMissingFactory(std::initializer_list<const char*> names);

/// The text of an ERROR message, followed by its debug details in parentheses when @p withDetails.
QString errorText(GstMessage* message, bool withDetails = true);

/// The caps currently set on @p pad, as text; empty when none are.
QString currentCapsString(GstPad* pad);

/// Buffer probe that counts into the std::atomic<int> passed as user data.
GstPadProbeReturn countBuffers(GstPad* pad, GstPadProbeInfo* info, gpointer userData);

/// @p bin's children, recursing into child bins when @p recurse.
std::vector<GStreamer::GstElementPtr> childElements(GstBin* bin, bool recurse = false);

/// Factory names of @p bin's children, sorted.
QStringList childFactoryNames(GstBin* bin, bool recurse = false);

/// The first element made by @p factoryName anywhere inside the bin @p bin, or null (also for a null @p bin).
GStreamer::GstElementPtr findByFactory(GstElement* bin, const char* factoryName);

/// A sample holding one uninitialized @p format frame; the sample owns the only buffer ref, so the buffer is writable.
GstSample* makeVideoSample(GstVideoFormat format, int width, int height, GstVideoInfo* info = nullptr);

/// Sets @p released once @p object is finalized.
void watchRelease(GstMiniObject* object, bool* released);

/// GStreamer::SourceFactory::create()'s source bin for @p uri, sunk, or null.
GStreamer::GstElementPtr createSource(const QString& uri, const GStreamer::SourceFactory::Config& config);

/// A QRhi on the Null backend: no device, and createFrom() accepts any native handle.
std::unique_ptr<QRhi> makeNullRhi();

#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
/// A sealable memfd holding @p bytes, padded to whole pages as udmabuf needs; the caller owns it. -1 on failure.
int makeMemfd(const QByteArray& bytes);
#endif

/// The NAL unit types in the Annex B @p data, in order.
QList<int> nalTypes(const QByteArray& data, bool h265);

/// Demuxes the MP4/MOV at @p path to EOS. Returns the number of video samples, or -1 with @p failure set when the
/// file can't be played to the end.
int countPlayableFrames(const QString& path, QString& failure);

/// A pipeline that is set to NULL and released on destruction.
class TestPipeline
{
public:
    TestPipeline() = default;
    /// Takes over the caller's reference to @p pipeline (or any element), floating or not.
    explicit TestPipeline(GstElement* pipeline);
    ~TestPipeline();

    TestPipeline(const TestPipeline&) = delete;
    TestPipeline& operator=(const TestPipeline&) = delete;

    /// Builds the pipeline from a gst-launch @p description; false, with error() set, when it doesn't parse.
    bool launch(const QString& description);

    const QString& error() const { return _error; }

    GstElement* get() const { return _pipeline.get(); }

    GstBin* bin() const { return GST_BIN(_pipeline.get()); }

    /// The child named @p name, or null.
    GStreamer::GstElementPtr element(const char* name) const;

    /// False when the pipeline refuses to start.
    bool play();
    /// The first EOS or ERROR message within @p timeout, or null.
    GStreamer::GstMessagePtr waitForEosOrError(std::chrono::milliseconds timeout = TestTimeout::mediumDuration()) const;
    /// Waits for EOS; returns an empty string on EOS, else the error or the timeout.
    QString runToEos(std::chrono::milliseconds timeout = TestTimeout::mediumDuration()) const;

private:
    GStreamer::GstElementPtr _pipeline;
    QString _error;
};

/// Encoded test streams (src/VideoManager/GStreamer/Runtime/Media, regenerated by its generate.sh), 30 fps with a
/// keyframe every 5 frames.
inline constexpr char kH264Stream[] = ":/unittest/video/h264_320x240.h264";
inline constexpr char kH264SmallStream[] = ":/unittest/video/h264_160x120.h264";
inline constexpr char kH265Stream[] = ":/unittest/video/h265_320x240.h265";
inline constexpr int kStreamFrameRate = 30;

/// One access unit of an encoded test stream.
struct AccessUnit
{
    QByteArray bytes;
    bool keyframe = false;
};

/// The access units of the Annex B stream @p resource (one of the k*Stream names; the extension names the codec),
/// split by h264parse or h265parse. Empty, with @p error set, when that fails.
QList<AccessUnit> loadAccessUnits(const QString& resource, QString* error = nullptr);

/// Sends an encoded test stream as live RTP to 127.0.0.1, looping it at its frame rate, as a camera does: no encoder
/// involved. The units go through appsrc ! payloader (config-interval=-1) ! udpsink, paced by udpsink's clock.
class FixtureRtpSender
{
public:
    /// Loads @p resource; error() tells why when that fails.
    explicit FixtureRtpSender(const QString& resource = QString::fromLatin1(kH264Stream));
    ~FixtureRtpSender();

    FixtureRtpSender(const FixtureRtpSender&) = delete;
    FixtureRtpSender& operator=(const FixtureRtpSender&) = delete;

    /// (Re)starts sending to @p port from the stream's first frame. False for port 0 or when the pipeline fails.
    bool start(quint16 port);
    /// Stops sending, as a camera losing its link.
    void stop();

    /// Units sent since construction.
    int frames() const { return _frames.load(std::memory_order_relaxed); }

    /// Goes on with @p resource's first frame, a keyframe, after the unit in flight. Same codec only.
    bool switchTo(const QString& resource);
    /// While held, keyframes (and with them the parameter sets) are skipped, as a long GOP the receiver joined late.
    void holdKeyframes(bool hold);

    const QString& error() const { return _error; }

private:
    static void _needData(GstElement* appsrc, guint length, gpointer self);

    bool _h265 = false;
    QString _error;
    std::mutex _mutex;
    QList<AccessUnit> _units;
    qsizetype _next = 0;
    GstClockTime _pts = 0;
    bool _keyframesHeld = false;
    std::atomic<int> _frames{0};
    // Declared last, so the pipeline stops before the state its callback and probe use goes away.
    std::unique_ptr<TestPipeline> _pipeline;
};

/// Records every video decoder's rank and restores them on destruction, so a test can reprioritize decoders.
class DecoderRankSnapshot
{
public:
    DecoderRankSnapshot();
    ~DecoderRankSnapshot();

    DecoderRankSnapshot(const DecoderRankSnapshot&) = delete;
    DecoderRankSnapshot& operator=(const DecoderRankSnapshot&) = delete;

private:
    std::vector<std::pair<GStreamer::GstFeaturePtr, guint>> _ranks;
};

/// An RTSP server on a free loopback port, for driving rtspsrc without gst-rtsp-server. Given an SDP, it answers
/// OPTIONS and DESCRIBE and records each SETUP URL without answering it, so the client stops right after picking
/// streams. Without one it never answers, as a camera whose RTSP service hung.
class FakeRtspServer
{
public:
    explicit FakeRtspServer(const QByteArray& sdp = {});

    FakeRtspServer(const FakeRtspServer&) = delete;
    FakeRtspServer& operator=(const FakeRtspServer&) = delete;

    [[nodiscard]] bool listen() { return _server.listen(QHostAddress::LocalHost, 0); }

    QString uri() const { return QStringLiteral("rtsp://127.0.0.1:%1/test").arg(_server.serverPort()); }

    int connections() const { return _connections; }

    const QList<QByteArray>& setupUrls() const { return _setupUrls; }

private:
    void _serve(QTcpSocket* socket);

    QTcpServer _server;
    QByteArray _sdp;
    QHash<QTcpSocket*, QByteArray> _pending;
    QList<QByteArray> _setupUrls;
    int _connections = 0;
};

}  // namespace GstTest

/// Skips the test unless GStreamer initializes (GstTest::initGStreamer()).
#define GSTTEST_REQUIRE_GSTREAMER()                                  \
    do {                                                             \
        const QString gstTestUnavailable = GstTest::initGStreamer(); \
        if (!gstTestUnavailable.isEmpty()) {                         \
            QSKIP(qPrintable(gstTestUnavailable));                   \
        }                                                            \
    } while (false)

/// Skips the test unless every named element factory is registered. GStreamerTestBase::initTestCase() guarantees the
/// elements GStreamer::completeInit() requires and so the rest of their plugins (coreelements, udp, rtp, rtpmanager,
/// playback, videoparsersbad): name only the optional ones.
#define GSTTEST_SKIP_UNLESS_FACTORIES(...)                                                              \
    do {                                                                                                \
        const QString gstTestMissing = GstTest::firstMissingFactory({__VA_ARGS__});                     \
        if (!gstTestMissing.isEmpty()) {                                                                \
            QSKIP(qPrintable(QStringLiteral("GStreamer factory unavailable: %1").arg(gstTestMissing))); \
        }                                                                                               \
    } while (false)

#endif
