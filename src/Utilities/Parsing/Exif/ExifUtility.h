#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QDateTime>
#include <QtCore/QString>

#include <libexif/exif-data.h>


namespace ExifUtility
{

// ============================================================================
// EXIF Data Management
// ============================================================================

/// Load EXIF data from a JPEG buffer
/// @return ExifData pointer (caller must call exif_data_unref), or nullptr on failure
ExifData* loadFromBuffer(const QByteArray &buffer);

/// Create new empty EXIF data structure with standard options set
/// @return ExifData pointer (caller must call exif_data_unref), or nullptr on failure
ExifData* createNew();

/// Save EXIF data back to a JPEG buffer
/// @param data The EXIF data to save
/// @param buffer The original JPEG buffer (modified in place)
/// @return true if successful
/// @note Only JPEG format is supported for writing. TIFF/DNG files return false.
bool saveToBuffer(ExifData *data, QByteArray &buffer);

/// Check if a buffer is a JPEG image (starts with 0xFF 0xD8)
bool isJpeg(const QByteArray &buffer);

/// Check if a buffer is a TIFF-based image (TIFF, DNG, etc.)
/// TIFF files start with "II" (little-endian) or "MM" (big-endian)
bool isTiff(const QByteArray &buffer);

/// Check if a buffer contains valid JPEG with EXIF data
bool hasExifData(const QByteArray &buffer);

// ============================================================================
// Tag Reading Helpers
// ============================================================================

/// Read a string value from an EXIF tag
QString readString(ExifData *data, ExifTag tag, ExifIfd ifd = EXIF_IFD_0);

/// Read a short (16-bit) value from an EXIF tag
int readShort(ExifData *data, ExifTag tag, ExifIfd ifd = EXIF_IFD_0);

/// Read a rational value from an EXIF tag
double readRational(ExifData *data, ExifTag tag, ExifIfd ifd = EXIF_IFD_0);

// ============================================================================
// Tag Writing Helpers
// ============================================================================

/// Get existing entry or create a new one with auto-initialization
/// Use for standard EXIF tags that exif_entry_initialize() can handle
/// @return Entry pointer (owned by IFD, do not free)
ExifEntry* initTag(ExifData *data, ExifIfd ifd, ExifTag tag);

/// Create a new entry with custom size for tags that need manual setup
/// Use for GPS tags and other tags that exif_entry_initialize() doesn't handle
/// @return Entry pointer (owned by IFD, do not free), or nullptr on failure
ExifEntry* createTag(ExifData *data, ExifIfd ifd, ExifTag tag, ExifFormat format, unsigned long components);

// ============================================================================
// GPS Coordinate Helpers
// ============================================================================

/// Convert GPS coordinate from EXIF rational format (deg/min/sec) to decimal degrees
double gpsRationalToDecimal(ExifEntry *entry, ExifByteOrder order);

/// Write GPS coordinate as EXIF rationals (degrees, minutes, seconds)
void writeGpsCoordinate(ExifEntry *entry, ExifByteOrder order, double value);

/// Write a single rational value to an entry
void writeRational(ExifEntry *entry, ExifByteOrder order, double value, int denominator = 100);

/// Write an ASCII character to a GPS reference entry (N/S/E/W)
void writeGpsRef(ExifEntry *entry, char value);

/// Write a byte value to a GPS altitude reference entry (0=above, 1=below sea level)
void writeGpsAltRef(ExifEntry *entry, unsigned char value);

// ============================================================================
// DateTime Helpers
// ============================================================================

/// Write a DateTime value to EXIF_TAG_DATE_TIME_ORIGINAL
/// @param data The EXIF data structure
/// @param dateTime The date/time to write
/// @return true if successful
bool writeDateTimeOriginal(ExifData *data, const QDateTime &dateTime);

} // namespace ExifUtility
