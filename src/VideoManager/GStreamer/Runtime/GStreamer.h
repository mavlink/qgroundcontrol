#pragma once

#include <functional>
#include <optional>

#include <QtCore/QByteArray>
#include <QtCore/QStringList>

#include "VideoBackend.h"

typedef struct _GstElement GstElement;

class QGCQVideoSinkController;
class QObject;
class QQuickWindow;
class QVideoSink;

namespace GStreamer {

/// A QGC video feature whose GStreamer elements are not all registered.
struct MissingPluginFeature
{
    QString feature;
    QStringList factories;  ///< The missing element factories; "a|b" when neither alternative exists.
    bool required = false;  ///< Part of the display core: GStreamer video can't work without it.
    QString hint;           ///< The likely cause, when it isn't simply a missing plugin.
};

/// The features in GStreamer.cc's plugin policy that miss an element factory, as reported by @p hasFactory.
/// completeInit() fails only when a required one is missing, and warns about the others.
QList<MissingPluginFeature> missingPluginFeatures(const std::function<bool(const char* factoryName)>& hasFactory);

/// @p debugLevel: the debug threshold to start with (none when unset), unless GST_DEBUG is set.
bool completeInit(std::optional<int> debugLevel = std::nullopt);
void setDebugLevel(int level);
/// A new sink bin (GstQgc::makeSinkBin()) with the settings of a VideoReceiver::DisplayConfig, which are fixed once it
/// is built, returned as an owned, non-floating reference (adopt it), or nullptr.
GstElement* createVideoSink(const QByteArray& conversionElement, bool disablePixelAspectRatio, bool gpuZeroCopy);

/// Binds the qgcqvideosink inside @p sinkBin, a new bin as createVideoSink() returns it, to @p videoSink through a new
/// QGCQVideoSinkController, a child of @p controllerParent. Returns the controller, or nullptr on failure.
QGCQVideoSinkController* setupQVideoSinkElement(GstElement* sinkBin, QVideoSink* videoSink, QObject* controllerParent);

// Process/runtime lifecycle, reached through GStreamerVideoBackend.
bool initialize(const VideoBackend::InitParams& params);
void onMainWindowReady(QQuickWindow* window);

}  // namespace GStreamer
