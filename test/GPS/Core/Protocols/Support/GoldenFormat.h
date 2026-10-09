#pragma once

// The text of the golden transcripts, and their comparison with the checked-in files under
// test/GPS/Core/Protocols/golden/.

#include <QtCore/QSet>
#include <QtCore/QString>
#include <QtCore/QStringList>

#include "Protocols/Support/GoldenTranscript.h"

namespace GPSTest::Golden {

/// A unified-diff-style hunk around the first difference of @a actual from @a expected, titled golden/@a title.
[[nodiscard]] QString difference(const QString& title, const QString& expected, const QString& actual);

/// Compares @a actual with the file @a relative under @a root, or rewrites it when QGC_GPS_GOLDEN_UPDATE=1.
/// @return empty when it matches or was rewritten, else what differs.
[[nodiscard]] QString checkGolden(const QString& root, const QString& relative, const QString& actual);

/// Compares the files matching @a pattern under @a root, as paths relative to it, with @a expected. Update mode
/// removes the stale ones. @return empty when they match, else the stale and missing files.
[[nodiscard]] QString checkInventory(const QString& root, const QString& pattern, const QSet<QString>& expected);

[[nodiscard]] QString typeName(GPSType type);

/// The `request` and `decoder` line describing what @a config asks of a @a type receiver.
[[nodiscard]] QString describeRequest(GPSType type, const GPSReceiverConfig& config);

/// The scenario golden of @a run; a @a detectionOnly scenario stops after detection.
[[nodiscard]] QString formatRun(const char* about, const Scenario& setup, bool detectionOnly, const Run& run);

/// The events of one decode, then those an empty decode after FRESHNESS_HORIZON expired, each marked so.
[[nodiscard]] QString formatDecode(const Decode& decode);

/// @a lines as golden text: trailing spaces removed, each line ended.
[[nodiscard]] QString transcriptText(QStringList lines);

}  // namespace GPSTest::Golden
