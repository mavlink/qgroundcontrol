#pragma once

#include <functional>

#include <QtCore/QByteArray>
#include <QtCore/QStringList>

#include "GStreamerEnvironment.h"

class Fact;
class QObject;
class QQuickItem;
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

/// Construct-only tunables for the qgcvideosinkbin, resolved from VideoSettings by the
/// VideoBackend layer so this facade stays decoupled from the settings singleton.
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
void* createVideoSink(const VideoSinkConfig& config);
void releaseVideoSink(void* sink);
VideoReceiver* createVideoReceiver(QObject* parent = nullptr);

/// Wire @p videoSink into the qgcqvideosink element inside @p sinkBin and create a
/// QGCQVideoSinkController child of @p controllerParent. Idempotent: prior controllers
/// under the same parent are torn down first. Returns true on success.
bool setupQVideoSinkElement(void* sinkBin, QVideoSink* videoSink, QObject* controllerParent);

/// Hardware decoder families currently present in the GStreamer registry, as VideoDecoderOptions
/// values (always omits ForceVideoDecoderDefault/Software). Lets the settings layer validate a
/// persisted ForceVideoDecoder* choice against what the running build can actually provide.
QList<VideoDecoderOptions> availableDecoderFamilies();

// Process/runtime lifecycle. Reached only through VideoBackend, which is the layer that
// no-ops these for the QtMultimedia build; this header is included solely under QGC_GST_STREAMING.
Environment::ValidationResult prepareEnvironment();
bool initialize(const QStringList& arguments, const Environment::ValidationResult& envResult);
void attachAppSink(QObject* receiver, void* sink, QQuickItem* widget);
void bindDebugLevelFact(Fact* fact, QObject* context);
void onMainWindowReady(QQuickWindow* window);

}  // namespace GStreamer
