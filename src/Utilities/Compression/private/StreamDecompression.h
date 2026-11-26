#pragma once

/// @file StreamDecompression.h
/// @brief Decompression of a single compressed stream (.gz, .xz, .zst, ...) to a file or to memory
/// @note Private to QGCCompression

#include <functional>

#include <QtCore/QByteArray>
#include <QtCore/QByteArrayView>
#include <QtCore/QString>

#include "ArchiveReader.h"

namespace LibArchive {

// Return values: see the end of ArchiveReader.h. Behaviour: see the single-file functions of Compression.h.

/// Decompress a single compressed stream to a file, whose directory is created if it is missing
Result decompressToFile(const QString& inputPath, const QString& outputPath, const ProgressCallback& progress,
                        qint64 maxBytes);

/// Decompress a single compressed stream from a file and give its data to @a sink a chunk at a time
/// A sink that returns false cancels the operation (Cancelled)
Result decompressToSink(const QString& inputPath, qint64 maxBytes, const std::function<bool(QByteArrayView)>& sink);

/// Decompress a single compressed stream into @a data
/// @param source Path of the file to read, or the bytes of the stream
template <typename Source>
Result decompressToMemory(const Source& source, qint64 maxBytes, QByteArray& data);

}  // namespace LibArchive
