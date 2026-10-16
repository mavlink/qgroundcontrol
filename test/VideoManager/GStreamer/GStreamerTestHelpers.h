#pragma once

#ifdef QGC_GST_STREAMING

#include <chrono>
#include <gst/gst.h>
#include <gst/video/video-info.h>
#include <initializer_list>
#include <memory>
#include <utility>
#include <vector>

#include <QtCore/QRegularExpression>
#include <QtCore/QString>
#include <QtCore/QStringList>

#include "GstScoped.h"
#include "UnitTest.h"

class QRhi;

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

/// The first element made by @p factoryName anywhere inside @p bin, or null.
GStreamer::GstElementPtr findByFactory(GstBin* bin, const char* factoryName);

/// A sample holding one uninitialized @p format frame; the sample owns the only buffer ref, so the buffer is writable.
GstSample* makeVideoSample(GstVideoFormat format, int width, int height, GstVideoInfo* info = nullptr);

/// Sets @p released once @p object is finalized.
void watchRelease(GstMiniObject* object, bool* released);

/// A QRhi on the Null backend: no device, and createFrom() accepts any native handle.
std::unique_ptr<QRhi> makeNullRhi();

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

/// libproxy's warning about a host proxy bypass entry it can't parse (e.g. "169.254/16" on macOS runners), which
/// rtspsrc's proxy lookup reaches the GStreamer log bridge with.
inline QRegularExpression libproxyIgnoreMaskWarning()
{
    return QRegularExpression(QStringLiteral("\"pxbackend\" \"Could not parse ignore mask"));
}

}  // namespace GstTest

#endif
