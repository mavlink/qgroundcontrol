#pragma once

#include <functional>

#include <QtCore/QByteArray>
#include <QtCore/QStringList>

#include "GStreamerEnvironment.h"

typedef struct _GstElement GstElement;

class Fact;
class QGCQVideoSinkController;
class QObject;
class QQuickWindow;
class QVideoSink;
class VideoReceiver;

namespace GStreamer {

enum VideoDecoderOptions : int
{
    ForceVideoDecoderDefault = 0,
    ForceVideoDecoderSoftware,
    ForceVideoDecoderNVIDIA,
    ForceVideoDecoderVAAPI,
    ForceVideoDecoderDirectX3D,
    ForceVideoDecoderVideoToolbox,
    ForceVideoDecoderIntel,
    ForceVideoDecoderVulkan,
    ForceVideoDecoderHardware
};

/// Construct-only tunables for the qgcvideosinkbin; videoSinkConfigFromSettings() resolves them from VideoSettings.
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

bool completeInit();
void setDebugLevel(int level);
/// Whether the sink may import GPU memory without a copy: not with the CPU path or a software decoder forced, nor
/// under Direct3D12, whose device GStreamer can't wrap.
bool gpuZeroCopyAllowedForCurrentGraphicsApi(bool forceCpuVideoPath, bool forceSoftwareDecoder);
/// The sink configuration VideoSettings asks for, with GPU zero-copy only where the graphics API allows it. GUI thread.
VideoSinkConfig videoSinkConfigFromSettings();
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

// Process/runtime lifecycle. Reached only through VideoBackend, which is the layer that
// no-ops these for builds without GStreamer; this header is included solely under QGC_GST_STREAMING.
Environment::ValidationResult prepareEnvironment();
bool initialize(const QStringList& arguments, const Environment::ValidationResult& envResult);
void bindDebugLevelFact(Fact* fact, QObject* context);
void onMainWindowReady(QQuickWindow* window);

}  // namespace GStreamer
