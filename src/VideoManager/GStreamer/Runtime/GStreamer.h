#pragma once

#include <functional>
#include <optional>

#include <QtCore/QByteArray>
#include <QtCore/QStringList>

#include "GStreamerEnvironment.h"
#include "VideoDecoderOptions.h"

typedef struct _GstElement GstElement;

class QGCQVideoSinkController;
class QObject;
class QQuickWindow;
class QVideoSink;
class VideoReceiver;

namespace GStreamer {

using VideoDecoderOptions = VideoDecoder::Options;
using enum VideoDecoder::Options;

/// Construct-only tunables for the qgcvideosinkbin; GstVideoReceiver fills them from its VideoReceiver::DisplayConfig.
struct VideoSinkConfig
{
    QByteArray conversionElement;
    bool disablePixelAspectRatio = false;
    bool gpuZeroCopy = false;
};

/// A QGC video feature whose GStreamer elements are not all registered.
struct MissingPluginFeature
{
    QString feature;
    QStringList factories;  ///< The missing element factories; "a|b" when neither alternative exists.
    bool required = false;  ///< Part of the display core: GStreamer video can't work without it.
};

/// The features in GStreamer.cc's plugin policy that miss an element factory, as reported by @p hasFactory.
/// completeInit() fails only when a required one is missing, and warns about the others.
QList<MissingPluginFeature> missingPluginFeatures(const std::function<bool(const char* factoryName)>& hasFactory);

/// @p debugLevel: the debug threshold to start with (none when unset), unless GST_DEBUG is set.
bool completeInit(std::optional<int> debugLevel = std::nullopt);
void setDebugLevel(int level);
/// A new qgcvideosinkbin, returned as an owned, non-floating reference (adopt it), or nullptr.
GstElement* createVideoSink(const VideoSinkConfig& config);
VideoReceiver* createVideoReceiver(QObject* parent = nullptr);

/// Binds the qgcqvideosink inside @p sinkBin, a new bin as createVideoSink() returns it, to @p videoSink through a new
/// QGCQVideoSinkController, a child of @p controllerParent. Returns the controller, or nullptr on failure.
QGCQVideoSinkController* setupQVideoSinkElement(GstElement* sinkBin, QVideoSink* videoSink, QObject* controllerParent);

/// Hardware decoder families currently present in the GStreamer registry, as VideoDecoderOptions
/// values (always omits ForceVideoDecoderDefault/Software). Lets the settings layer validate a
/// persisted ForceVideoDecoder* choice against what the running build can actually provide.
QList<VideoDecoderOptions> availableDecoderFamilies();

// Process/runtime lifecycle, reached through GStreamerVideoBackend.
Environment::ValidationResult prepareEnvironment();
bool initialize(const QStringList& arguments, const Environment::ValidationResult& envResult,
                std::optional<int> debugLevel = std::nullopt);
/// Proves, for the boot test, that the runtime initialize() set up works: decoding, recording, the bundled plugins
/// and GIO (GStreamerSelfTest.h). Logs each failure under Video.GStreamer.SelfTest; GUI thread, after initialize().
bool selfTest();
void onMainWindowReady(QQuickWindow* window);

}  // namespace GStreamer
