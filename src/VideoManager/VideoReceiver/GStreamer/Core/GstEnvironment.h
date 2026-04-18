#pragma once

class QString;

namespace GStreamer {

/// Query whether environment preparation produced a valid bundled-runtime
/// path layout. Returns true by default on dev builds that skip path
/// injection entirely. On failure (bundle present but malformed), writes
/// the diagnostic message into `error` when non-null.
bool envPathsValid(QString* error = nullptr);

}  // namespace GStreamer
