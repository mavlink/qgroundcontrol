#pragma once

#include <QtCore/QString>
#include <QtMultimedia/QMediaFormat>

/// File-path construction for recordings, subtitles, and image grabs.
/// Pure string helpers — no I/O, no settings access.
namespace VideoFileNaming {

/// Maps a recording container format to its filename extension.
/// Returns "mkv" for anything other than QuickTime/MPEG4.
QString extensionForFormat(QMediaFormat::FileFormat format);

/// Returns the format used by the UI as a user-facing filename base when no
/// explicit name is supplied. Today: "yyyy-MM-dd_hh.mm.ss".
QString defaultVideoBaseName();

/// Builds the per-stream recording path template. The `%1` placeholder is
/// filled by the caller with a per-stream prefix ("", "thermal.", etc.),
/// producing e.g. `<savePath>/<base>.thermal.mkv`.
///
/// Example: buildRecordingTemplate("/tmp", "foo", Matroska) →
///     "/tmp/foo.%1mkv"
QString buildRecordingTemplate(const QString& savePath,
                               const QString& baseName,
                               QMediaFormat::FileFormat format);

/// Returns the `.srt` sibling for a recording path, replacing the extension.
/// If `videoPath` has no extension, appends ".srt".
QString subtitleSiblingPath(const QString& videoPath);

/// Builds the default image-grab destination: `<photoSavePath>/<timestamp>.jpg`.
/// Timestamp uses millisecond precision so rapid grabs don't collide.
QString buildImageGrabPath(const QString& photoSavePath);

}  // namespace VideoFileNaming
