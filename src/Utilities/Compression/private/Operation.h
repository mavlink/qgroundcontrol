#pragma once

/// @file Operation.h
/// @brief What the public functions of Compression.h do around a LibArchive operation: check the input and
///        build the Outcome
/// @note Private to QGCCompression

#include <QtCore/QLoggingCategory>
#include <QtCore/QString>

#include "ArchiveReader.h"
#include "CompressionTypes.h"

Q_DECLARE_LOGGING_CATEGORY(CompressionLog)

namespace Compression {

/// Check that a file exists and can be read
Outcome validateFileInput(const QString& filePath);

/// Whether two paths name the same file, also through a link or in another spelling of the name
bool isSameFile(const QString& path, const QString& otherPath);

/// Build the Outcome of a LibArchive operation
/// @param what What could not be done, the start of the failure message
Outcome operationOutcome(const LibArchive::Result& result, const QString& what);

}  // namespace Compression
