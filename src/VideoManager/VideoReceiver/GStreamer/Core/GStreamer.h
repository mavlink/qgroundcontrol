#pragma once

#include <QtCore/QFuture>
#include <QtCore/QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(GStreamerLog)
Q_DECLARE_LOGGING_CATEGORY(GStreamerAPILog)

class VideoReceiver;

namespace GStreamer {

/// Prepare environment, run gst_init asynchronously, and register plugins.
/// Returns a future that resolves true
/// on success.  Encapsulates prepareEnvironment + initialize +
/// completeInit so callers never touch GStreamer internals.
QFuture<bool> initAsync();

/// Low-level init steps — exposed for unit tests only.
void prepareEnvironment();
bool initialize();
bool completeInit();
bool isAvailable();

void setDebugLevel(int level);

}  // namespace GStreamer
