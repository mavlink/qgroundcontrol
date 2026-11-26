#pragma once

/// @file ArchiveExtraction.h
/// @brief Extraction of an archive into a directory
/// @note Private to QGCCompression

#include <QtCore/QString>

#include "ArchiveReader.h"

namespace LibArchive {

/// Extract an archive into a directory, as Compression::extractArchive() documents: ZIP, and 7z and TAR (plain or
/// compressed) if the build enables them. The caller has checked that the directory does not exist or is empty, and
/// gives its path absolute and without "." and "..".
/// Return values: see the end of ArchiveReader.h. An archive whose format this build lacks fails with
/// InvalidArchive here, with Result::unrecognized set
Result extractArchive(const QString& archivePath, const QString& outputDirectoryPath,
                      const Compression::ExtractOptions& options);

}  // namespace LibArchive
