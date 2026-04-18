#pragma once

#include <QtCore/QStringList>

#include <functional>

class QString;

/// Crash-recovery scanner for orphaned recording sessions.
///
/// A session manifest is "orphaned" when it has `version == 1` but no
/// `stopTimestamp` — i.e. `RecordingSession::stop()` never ran (process crash,
/// power loss, or kill -9). The scanner walks `recordingDir`, probes each
/// referenced video file's playability with a short QMediaPlayer preroll, and
/// either renames the manifest to `.json.ok` (all tracks playable) or moves
/// manifest + video files under `corrupted/<timestamp>/`.
///
/// Decoupled from RecordingSession to isolate filesystem/media-probe logic
/// from live-session lifecycle — no shared state with a running session.
namespace RecordingOrphanScanner {

using CorruptionCallback = std::function<void(const QStringList& movedFiles)>;

/// Scan `recordingDir` for orphaned manifests. Returns the count found.
/// `onCorruption` fires once per unplayable orphan with the list of files
/// quarantined under `corrupted/`. Safe to call with an empty callback.
int scan(const QString& recordingDir, const CorruptionCallback& onCorruption = {});

}  // namespace RecordingOrphanScanner
