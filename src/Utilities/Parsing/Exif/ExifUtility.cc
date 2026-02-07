#include "ExifUtility.h"
#include <QtCore/QLoggingCategory>

#include <cassert>
#include <cmath>

Q_STATIC_LOGGING_CATEGORY(ExifUtilityLog, "Utilities.ExifUtility")

namespace ExifUtility
{

// ============================================================================
// EXIF Data Management
// ============================================================================

ExifData* loadFromBuffer(const QByteArray &buffer)
{
    return exif_data_new_from_data(
        reinterpret_cast<const unsigned char*>(buffer.constData()),
        static_cast<unsigned int>(buffer.size())
    );
}

ExifData* createNew()
{
    ExifData *data = exif_data_new();
    if (data) {
        // Set standard options as recommended by libexif examples
        exif_data_set_option(data, EXIF_DATA_OPTION_FOLLOW_SPECIFICATION);
        exif_data_set_data_type(data, EXIF_DATA_TYPE_COMPRESSED);
        exif_data_set_byte_order(data, EXIF_BYTE_ORDER_INTEL);
        // Create mandatory EXIF fields with default values
        exif_data_fix(data);
    }
    return data;
}

bool saveToBuffer(ExifData *data, QByteArray &buffer)
{
    if (!data) {
        qCWarning(ExifUtilityLog) << "Null EXIF data";
        return false;
    }

    if (!isJpeg(buffer)) {
        if (isTiff(buffer)) {
            qCWarning(ExifUtilityLog) << "TIFF/DNG format not supported for EXIF writing";
        } else {
            qCWarning(ExifUtilityLog) << "Not a valid JPEG file";
        }
        return false;
    }

    // Generate EXIF output
    unsigned char *exifBuffer = nullptr;
    unsigned int exifSize = 0;
    exif_data_save_data(data, &exifBuffer, &exifSize);

    if (!exifBuffer || exifSize == 0) {
        qCWarning(ExifUtilityLog) << "Failed to generate EXIF data";
        if (exifBuffer) free(exifBuffer);
        return false;
    }

    // Find existing APP1 marker
    int app1Start = -1;
    int app1End = -1;
    int pos = 2;

    while (pos < buffer.size() - 3) {
        if (static_cast<unsigned char>(buffer[pos]) == 0xFF) {
            unsigned char marker = static_cast<unsigned char>(buffer[pos + 1]);

            if (marker == 0xE1) {  // APP1 (EXIF)
                app1Start = pos;
                int segmentLen = (static_cast<unsigned char>(buffer[pos + 2]) << 8) |
                                 static_cast<unsigned char>(buffer[pos + 3]);
                app1End = pos + 2 + segmentLen;
                break;
            } else if (marker == 0xDA) {  // SOS - stop searching
                break;
            } else if (marker >= 0xE0 && marker <= 0xEF) {  // Other APPn
                int segmentLen = (static_cast<unsigned char>(buffer[pos + 2]) << 8) |
                                 static_cast<unsigned char>(buffer[pos + 3]);
                pos += 2 + segmentLen;
                continue;
            } else if (marker == 0xD8 || marker == 0xD9 || marker == 0x00) {
                pos++;
                continue;
            }
        }
        pos++;
    }

    // Build new APP1 segment
    QByteArray newApp1;
    newApp1.append('\xFF');
    newApp1.append('\xE1');

    int app1Len = exifSize + 2;
    newApp1.append(static_cast<char>((app1Len >> 8) & 0xFF));
    newApp1.append(static_cast<char>(app1Len & 0xFF));
    newApp1.append(reinterpret_cast<const char*>(exifBuffer), exifSize);

    free(exifBuffer);

    // Reconstruct JPEG
    QByteArray newBuffer;
    newBuffer.reserve(buffer.size() + newApp1.size());

    // SOI marker
    newBuffer.append(buffer.left(2));

    // New APP1
    newBuffer.append(newApp1);

    // Rest of image (skip old APP1 if found)
    if (app1Start > 0 && app1End > app1Start && app1End <= buffer.size()) {
        newBuffer.append(buffer.mid(app1End));
    } else {
        newBuffer.append(buffer.mid(2));
    }

    buffer = newBuffer;
    return true;
}

bool isJpeg(const QByteArray &buffer)
{
    return buffer.size() >= 2 &&
           static_cast<unsigned char>(buffer[0]) == 0xFF &&
           static_cast<unsigned char>(buffer[1]) == 0xD8;
}

bool isTiff(const QByteArray &buffer)
{
    if (buffer.size() < 4) {
        return false;
    }
    // TIFF files start with "II" (Intel/little-endian) or "MM" (Motorola/big-endian)
    // followed by magic number 42 (0x002A)
    if (buffer[0] == 'I' && buffer[1] == 'I') {
        return static_cast<unsigned char>(buffer[2]) == 0x2A &&
               static_cast<unsigned char>(buffer[3]) == 0x00;
    }
    if (buffer[0] == 'M' && buffer[1] == 'M') {
        return static_cast<unsigned char>(buffer[2]) == 0x00 &&
               static_cast<unsigned char>(buffer[3]) == 0x2A;
    }
    return false;
}

bool hasExifData(const QByteArray &buffer)
{
    if (buffer.size() < 12) {
        return false;
    }

    // TIFF files contain EXIF data inline
    if (isTiff(buffer)) {
        return true;
    }

    // For JPEG, check for APP1 marker with EXIF header
    if (!isJpeg(buffer)) {
        return false;
    }

    QByteArray app1Marker("\xFF\xE1", 2);
    int app1Pos = buffer.indexOf(app1Marker, 2);
    if (app1Pos < 0 || app1Pos + 10 > buffer.size()) {
        return false;
    }

    QByteArray exifHeader("Exif\0\0", 6);
    return buffer.mid(app1Pos + 4, 6) == exifHeader;
}

// ============================================================================
// Tag Reading Helpers
// ============================================================================

QString readString(ExifData *data, ExifTag tag, ExifIfd ifd)
{
    if (!data) return QString();

    ExifEntry *entry = exif_content_get_entry(data->ifd[ifd], tag);
    if (!entry) {
        entry = exif_data_get_entry(data, tag);
    }
    if (!entry) {
        return QString();
    }

    char value[256];
    exif_entry_get_value(entry, value, sizeof(value));
    return QString::fromUtf8(value).trimmed();
}

int readShort(ExifData *data, ExifTag tag, ExifIfd ifd)
{
    if (!data) return 0;

    ExifEntry *entry = exif_content_get_entry(data->ifd[ifd], tag);
    if (!entry || entry->size < 2) {
        return 0;
    }

    ExifByteOrder order = exif_data_get_byte_order(data);
    return exif_get_short(entry->data, order);
}

double readRational(ExifData *data, ExifTag tag, ExifIfd ifd)
{
    if (!data) return 0.0;

    ExifEntry *entry = exif_content_get_entry(data->ifd[ifd], tag);
    if (!entry || entry->size < 8) {
        return 0.0;
    }

    ExifByteOrder order = exif_data_get_byte_order(data);
    ExifRational rational = exif_get_rational(entry->data, order);
    if (rational.denominator == 0) {
        return 0.0;
    }
    return static_cast<double>(rational.numerator) / static_cast<double>(rational.denominator);
}

// ============================================================================
// Tag Writing Helpers
// ============================================================================

ExifEntry* initTag(ExifData *data, ExifIfd ifd, ExifTag tag)
{
    if (!data) return nullptr;

    // Return existing tag if present
    ExifEntry *entry = exif_content_get_entry(data->ifd[ifd], tag);
    if (entry) {
        return entry;
    }

    // Create new entry
    entry = exif_entry_new();
    if (!entry) return nullptr;

    // Tag must be set before adding to content
    entry->tag = tag;

    // Add to IFD
    exif_content_add_entry(data->ifd[ifd], entry);

    // Initialize with default data (allocates memory for standard tags)
    exif_entry_initialize(entry, tag);

    // IFD now owns the entry, release our reference
    exif_entry_unref(entry);

    return exif_content_get_entry(data->ifd[ifd], tag);
}

ExifEntry* createTag(ExifData *data, ExifIfd ifd, ExifTag tag, ExifFormat format, unsigned long components)
{
    if (!data) return nullptr;

    // Create a memory allocator for this entry
    ExifMem *mem = exif_mem_new_default();
    if (!mem) return nullptr;

    // Create entry using our allocator
    ExifEntry *entry = exif_entry_new_mem(mem);
    if (!entry) {
        exif_mem_unref(mem);
        return nullptr;
    }

    // Calculate size and allocate data buffer
    size_t size = exif_format_get_size(format) * components;
    unsigned char *buf = static_cast<unsigned char*>(exif_mem_alloc(mem, size));
    if (!buf) {
        exif_entry_unref(entry);
        exif_mem_unref(mem);
        return nullptr;
    }

    // Fill in entry fields
    entry->data = buf;
    entry->size = size;
    entry->tag = tag;
    entry->format = format;
    entry->components = components;

    // Add to IFD
    exif_content_add_entry(data->ifd[ifd], entry);

    // Release our references - IFD now owns the entry
    exif_mem_unref(mem);
    exif_entry_unref(entry);

    return exif_content_get_entry(data->ifd[ifd], tag);
}

// ============================================================================
// GPS Coordinate Helpers
// ============================================================================

double gpsRationalToDecimal(ExifEntry *entry, ExifByteOrder order)
{
    if (!entry || entry->components < 3 || !entry->data) {
        return 0.0;
    }

    double degrees = 0.0;
    double minutes = 0.0;
    double seconds = 0.0;

    for (unsigned int i = 0; i < entry->components && i < 3; i++) {
        ExifRational rational = exif_get_rational(entry->data + i * 8, order);
        if (rational.denominator == 0) {
            continue;
        }
        double value = static_cast<double>(rational.numerator) / static_cast<double>(rational.denominator);
        switch (i) {
            case 0: degrees = value; break;
            case 1: minutes = value; break;
            case 2: seconds = value; break;
        }
    }

    return degrees + (minutes / 60.0) + (seconds / 3600.0);
}

void writeGpsCoordinate(ExifEntry *entry, ExifByteOrder order, double value)
{
    if (!entry || !entry->data || entry->size < 24) return;

    double absVal = fabs(value);
    int degrees = static_cast<int>(absVal);
    double minutesF = (absVal - degrees) * 60.0;
    int minutes = static_cast<int>(minutesF);
    double seconds = (minutesF - minutes) * 60.0;

    ExifRational rationals[3] = {
        {static_cast<ExifLong>(degrees), 1},
        {static_cast<ExifLong>(minutes), 1},
        {static_cast<ExifLong>(static_cast<int>(seconds * 1000)), 1000}
    };

    for (int i = 0; i < 3; i++) {
        exif_set_rational(entry->data + i * 8, order, rationals[i]);
    }
}

void writeRational(ExifEntry *entry, ExifByteOrder order, double value, int denominator)
{
    if (!entry || !entry->data || entry->size < 8) return;

    ExifRational rational = {
        static_cast<ExifLong>(static_cast<int>(fabs(value) * denominator)),
        static_cast<ExifLong>(denominator)
    };
    exif_set_rational(entry->data, order, rational);
}

void writeGpsRef(ExifEntry *entry, char value)
{
    if (!entry || !entry->data || entry->size < 2) return;
    entry->data[0] = static_cast<unsigned char>(value);
    entry->data[1] = '\0';
}

void writeGpsAltRef(ExifEntry *entry, unsigned char value)
{
    if (!entry || !entry->data || entry->size < 1) return;
    entry->data[0] = value;
}

// ============================================================================
// DateTime Helpers
// ============================================================================

bool writeDateTimeOriginal(ExifData *data, const QDateTime &dateTime)
{
    if (!data || !dateTime.isValid()) {
        return false;
    }

    // EXIF DateTime format: "YYYY:MM:DD HH:MM:SS" (20 bytes including null terminator)
    const QString dateStr = dateTime.toString(QStringLiteral("yyyy:MM:dd hh:mm:ss"));
    const QByteArray dateBytes = dateStr.toLatin1();

    // Write to both DateTimeOriginal and DateTimeDigitized for compatibility
    // (some readers prefer one over the other)
    const ExifTag tags[] = {EXIF_TAG_DATE_TIME_ORIGINAL, EXIF_TAG_DATE_TIME_DIGITIZED};
    bool success = false;

    for (ExifTag tag : tags) {
        ExifEntry *entry = initTag(data, EXIF_IFD_EXIF, tag);
        if (!entry) {
            qCWarning(ExifUtilityLog) << "Failed to create DateTime tag:" << tag;
            continue;
        }

        // Entry should have 20 bytes allocated by initTag
        if (entry->size >= 20 && entry->data) {
            memcpy(entry->data, dateBytes.constData(), qMin(static_cast<qsizetype>(19), dateBytes.size()));
            entry->data[19] = '\0';
            success = true;
        } else {
            qCWarning(ExifUtilityLog) << "DateTime entry size mismatch:" << entry->size << "for tag" << tag;
        }
    }

    return success;
}

} // namespace ExifUtility
