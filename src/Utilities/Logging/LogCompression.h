#pragma once

#include <QtCore/QByteArray>

/// Lightweight compression utilities for log transmission.
/// Uses zlib via Qt's qCompress/qUncompress.
namespace LogCompression {

/// Protocol header bytes
constexpr quint8 HeaderUncompressed = 0x00;
constexpr quint8 HeaderCompressed   = 0x01;

/// Compression levels (maps to zlib)
enum class Level {
    None    = 0,   ///< No compression (still adds header)
    Fast    = 1,   ///< Fastest compression
    Default = 6,   ///< Balanced (zlib default)
    Best    = 9    ///< Best compression ratio
};

/// Compress data with protocol header.
/// @param data Raw data to compress
/// @param level Compression level
/// @param minSize Minimum size to attempt compression (smaller data sent uncompressed)
/// @return Header byte + payload (compressed or uncompressed)
QByteArray compress(const QByteArray& data,
                    Level level = Level::Default,
                    int minSize = 256);

/// Decompress data with protocol header.
/// @param data Header byte + payload
/// @return Original uncompressed data, or empty on error
QByteArray decompress(const QByteArray& data);

/// Check if data has compression header.
/// @param data Data with protocol header
/// @return true if first byte indicates compressed data
bool isCompressed(const QByteArray& data);

/// Get compression ratio from last compress() call (thread-local).
/// @return Ratio as percentage (e.g., 65 means 65% of original size)
int lastCompressionRatio();

} // namespace LogCompression
