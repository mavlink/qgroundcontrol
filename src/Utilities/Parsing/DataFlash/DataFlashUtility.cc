#include "DataFlashUtility.h"
#include <QtCore/QLoggingCategory>

#include <cstring>

Q_STATIC_LOGGING_CATEGORY(DataFlashUtilityLog, "Utilities.DataFlashUtility")

namespace DataFlashUtility
{

// ============================================================================
// Format Character Functions
// ============================================================================

int formatCharSize(char c)
{
    switch (c) {
    case 'b': case 'B': case 'M':
        return 1;
    case 'h': case 'H': case 'c': case 'C': case 'g':  // g = half-precision float
        return 2;
    case 'i': case 'I': case 'e': case 'E': case 'L': case 'f':
        return 4;
    case 'd': case 'q': case 'Q':
        return 8;
    case 'n':
        return 4;
    case 'N':
        return 16;
    case 'Z': case 'a':  // Z = 64-char string, a = 64-byte array (32 int16)
        return 64;
    default:
        return 0;
    }
}

int calculatePayloadSize(const QString &format)
{
    int size = 0;
    for (const QChar &ch : format) {
        size += formatCharSize(ch.toLatin1());
    }
    return size;
}

// ============================================================================
// Half-Precision Float Conversion
// ============================================================================

float halfToFloat(uint16_t bits)
{
    const uint32_t sign = (bits & 0x8000) << 16;
    const uint32_t exponent = (bits >> 10) & 0x1F;
    const uint32_t mantissa = bits & 0x3FF;
    uint32_t result;

    if (exponent == 0) {
        result = sign;  // Zero or denormalized (treat as zero)
    } else if (exponent == 31) {
        result = sign | 0x7F800000 | (mantissa << 13);  // Inf or NaN
    } else {
        result = sign | ((exponent + 112) << 23) | (mantissa << 13);
    }

    float fval;
    memcpy(&fval, &result, sizeof(fval));
    return fval;
}

// ============================================================================
// Value Parsing
// ============================================================================

QVariant parseValue(const char *data, char formatChar)
{
    switch (formatChar) {
    case 'b':
        return static_cast<int8_t>(*data);
    case 'B':
    case 'M':
        return static_cast<uint8_t>(*data);
    case 'h': {
        int16_t val;
        memcpy(&val, data, sizeof(val));
        return val;
    }
    case 'H': {
        uint16_t val;
        memcpy(&val, data, sizeof(val));
        return val;
    }
    case 'c': {
        int16_t val;
        memcpy(&val, data, sizeof(val));
        return val / 100.0;
    }
    case 'C': {
        uint16_t val;
        memcpy(&val, data, sizeof(val));
        return val / 100.0;
    }
    case 'i': {
        int32_t val;
        memcpy(&val, data, sizeof(val));
        return val;
    }
    case 'I': {
        uint32_t val;
        memcpy(&val, data, sizeof(val));
        return val;
    }
    case 'e': {
        int32_t val;
        memcpy(&val, data, sizeof(val));
        return val / 100.0;
    }
    case 'E': {
        uint32_t val;
        memcpy(&val, data, sizeof(val));
        return val / 100.0;
    }
    case 'L': {
        int32_t val;
        memcpy(&val, data, sizeof(val));
        return val / 1.0e7;  // Latitude/longitude in degrees
    }
    case 'f': {
        float val;
        memcpy(&val, data, sizeof(val));
        return static_cast<double>(val);
    }
    case 'd': {
        double val;
        memcpy(&val, data, sizeof(val));
        return val;
    }
    case 'q': {
        int64_t val;
        memcpy(&val, data, sizeof(val));
        return static_cast<qlonglong>(val);
    }
    case 'Q': {
        uint64_t val;
        memcpy(&val, data, sizeof(val));
        return static_cast<qulonglong>(val);
    }
    case 'g': {
        uint16_t bits;
        memcpy(&bits, data, sizeof(bits));
        return static_cast<double>(halfToFloat(bits));
    }
    case 'n':
        return QString::fromLatin1(data, qstrnlen(data, 4));
    case 'N':
        return QString::fromLatin1(data, qstrnlen(data, 16));
    case 'Z':
        return QString::fromLatin1(data, qstrnlen(data, 64));
    case 'a':
        // 64-byte array (32 int16 values) - return as raw bytes
        return QByteArray(data, 64);
    default:
        return QVariant();
    }
}

QMap<QString, QVariant> parseMessage(const char *data, const MessageFormat &fmt)
{
    QMap<QString, QVariant> result;
    int offset = 0;

    for (int i = 0; i < fmt.format.length() && i < fmt.columns.size(); ++i) {
        const char formatChar = fmt.format.at(i).toLatin1();
        const QString &columnName = fmt.columns.at(i);

        const int size = formatCharSize(formatChar);
        if (size == 0) {
            continue;
        }

        result[columnName] = parseValue(data + offset, formatChar);
        offset += size;
    }

    return result;
}

// ============================================================================
// Header and Message Detection
// ============================================================================

bool isValidHeader(const char *data, qint64 size)
{
    if (size < 3) {
        return false;
    }
    return static_cast<uint8_t>(data[0]) == kHeaderByte1 &&
           static_cast<uint8_t>(data[1]) == kHeaderByte2;
}

qint64 findNextHeader(const char *data, qint64 size, qint64 offset)
{
    while (offset + 2 < size) {
        if (static_cast<uint8_t>(data[offset]) == kHeaderByte1 &&
            static_cast<uint8_t>(data[offset + 1]) == kHeaderByte2) {
            return offset;
        }
        ++offset;
    }
    return -1;
}

// ============================================================================
// FMT Message Parsing
// ============================================================================

MessageFormat parseFmtPayload(const char *data)
{
    MessageFormat fmt;
    fmt.type = static_cast<uint8_t>(data[0]);
    fmt.length = static_cast<uint8_t>(data[1]);
    fmt.name = QString::fromLatin1(data + 2, qstrnlen(data + 2, 4));
    fmt.format = QString::fromLatin1(data + 6, qstrnlen(data + 6, 16));
    const QString columnsStr = QString::fromLatin1(data + 22, qstrnlen(data + 22, 64));
    fmt.columns = columnsStr.trimmed().split(',');
    return fmt;
}

bool parseFmtMessages(const char *data, qint64 size, QMap<uint8_t, MessageFormat> &formats)
{
    formats.clear();

    if (!isValidHeader(data, size)) {
        return false;
    }

    qint64 pos = 0;

    while (pos + 3 <= size) {
        // Find next message header
        if (static_cast<uint8_t>(data[pos]) != kHeaderByte1 ||
            static_cast<uint8_t>(data[pos + 1]) != kHeaderByte2) {
            ++pos;
            continue;
        }

        const uint8_t msgType = static_cast<uint8_t>(data[pos + 2]);
        pos += 3;

        if (msgType == kFmtMessageType) {
            if (pos + kFmtPayloadSize > size) {
                break;
            }

            const MessageFormat fmt = parseFmtPayload(data + pos);
            formats[fmt.type] = fmt;
            pos += kFmtPayloadSize;
        } else {
            // Skip message if we know its length
            if (formats.contains(msgType)) {
                pos += formats[msgType].length - 3;  // -3 for header already consumed
            } else {
                // Unknown format, try to find next header
                ++pos;
            }
        }
    }

    return !formats.isEmpty();
}

// ============================================================================
// Message Iteration
// ============================================================================

int iterateMessages(const char *data, qint64 size,
                    const QMap<uint8_t, MessageFormat> &formats,
                    const MessageCallback &callback)
{
    int count = 0;
    qint64 pos = 0;

    while (pos + 3 <= size) {
        // Find next message header
        if (static_cast<uint8_t>(data[pos]) != kHeaderByte1 ||
            static_cast<uint8_t>(data[pos + 1]) != kHeaderByte2) {
            ++pos;
            continue;
        }

        const uint8_t msgType = static_cast<uint8_t>(data[pos + 2]);
        pos += 3;

        if (!formats.contains(msgType)) {
            continue;
        }

        const MessageFormat &fmt = formats[msgType];
        const int payloadSize = fmt.length - 3;

        if (pos + payloadSize > size) {
            break;
        }

        ++count;
        if (!callback(msgType, data + pos, payloadSize, fmt)) {
            break;
        }

        pos += payloadSize;
    }

    return count;
}

} // namespace DataFlashUtility
