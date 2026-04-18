#pragma once

#include <cstdint>

#include <QtCore/QString>
#include <QtCore/QStringList>

/// Disk-quota enforcement for recorded video files. Pure I/O against a
/// directory with a size budget — no dependency on SettingsManager or any
/// VideoManager state.
namespace VideoStorageCleaner {

/// Deletes the oldest files in `directory` matching `nameFilters` until the
/// total size fits within `maxBytes`. Returns the number of files removed.
///
/// Files are sorted by modification time (oldest last-accessed are removed
/// first). The name filters are matched against basenames (e.g. "*.mkv").
/// A zero or negative `maxBytes` is treated as "no limit" and nothing is
/// removed.
int pruneToLimit(const QString& directory, const QStringList& nameFilters, uint64_t maxBytes);

}  // namespace VideoStorageCleaner
