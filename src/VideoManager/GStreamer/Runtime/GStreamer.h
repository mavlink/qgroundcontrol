#pragma once

#include <functional>
#include <optional>

#include <QtCore/QStringList>

#include "VideoBackend.h"

class QQuickWindow;

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
// Process/runtime lifecycle, reached through GStreamerVideoBackend.
bool initialize(const VideoBackend::InitParams& params);
void onMainWindowReady(QQuickWindow* window);

}  // namespace GStreamer
