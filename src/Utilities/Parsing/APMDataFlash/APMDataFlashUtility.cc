#include "APMDataFlashUtility.h"

#include <bit>

#include <QtCore/qfloat16.h>

#include "LittleEndian.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(APMDataFlashUtilityLog, "Utilities.APMDataFlashUtility")

namespace APMDataFlashUtility
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
    return static_cast<float>(std::bit_cast<qfloat16>(bits));
}

// ============================================================================
// Value Parsing
// ============================================================================

QVariant parseValue(const char* data, qint64 size, char formatChar)
{
    const int requiredSize = formatCharSize(formatChar);
    if (requiredSize == 0) {
        qCWarning(APMDataFlashUtilityLog) << "Unsupported DataFlash format character:" << formatChar;
        return {};
    }
    if (!data || size < requiredSize) {
        qCWarning(APMDataFlashUtilityLog) << "Missing or truncated DataFlash value for format:" << formatChar;
        return {};
    }

    const std::span<const uint8_t> bytes{reinterpret_cast<const uint8_t*>(data),
                                         static_cast<std::size_t>(requiredSize)};
    switch (formatChar) {
    case 'b':
        return *LittleEndian::read<int8_t>(bytes);
    case 'B':
    case 'M':
        return *LittleEndian::read<uint8_t>(bytes);
    case 'h': {
        const auto val = *LittleEndian::read<int16_t>(bytes);
        return val;
    }
    case 'H': {
        const auto val = *LittleEndian::read<uint16_t>(bytes);
        return val;
    }
    case 'c': {
        const auto val = *LittleEndian::read<int16_t>(bytes);
        return val / 100.0;
    }
    case 'C': {
        const auto val = *LittleEndian::read<uint16_t>(bytes);
        return val / 100.0;
    }
    case 'i': {
        const auto val = *LittleEndian::read<int32_t>(bytes);
        return val;
    }
    case 'I': {
        const auto val = *LittleEndian::read<uint32_t>(bytes);
        return val;
    }
    case 'e': {
        const auto val = *LittleEndian::read<int32_t>(bytes);
        return val / 100.0;
    }
    case 'E': {
        const auto val = *LittleEndian::read<uint32_t>(bytes);
        return val / 100.0;
    }
    case 'L': {
        const auto val = *LittleEndian::read<int32_t>(bytes);
        return val / 1.0e7;  // Latitude/longitude in degrees
    }
    case 'f': {
        const auto val = *LittleEndian::read<float>(bytes);
        return static_cast<double>(val);
    }
    case 'd': {
        const auto val = *LittleEndian::read<double>(bytes);
        return val;
    }
    case 'q': {
        const auto val = *LittleEndian::read<int64_t>(bytes);
        return static_cast<qlonglong>(val);
    }
    case 'Q': {
        const auto val = *LittleEndian::read<uint64_t>(bytes);
        return static_cast<qulonglong>(val);
    }
    case 'g': {
        const auto bits = *LittleEndian::read<uint16_t>(bytes);
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

QMap<QString, QVariant> parseMessage(const char* data, qint64 size, const MessageFormat& fmt)
{
    if (!data || size < 0) {
        qCWarning(APMDataFlashUtilityLog) << "Invalid DataFlash message payload";
        return {};
    }

    QMap<QString, QVariant> result;
    qint64 offset = 0;

    for (qsizetype i = 0; i < fmt.format.size(); ++i) {
        const char formatChar = fmt.format.at(i).toLatin1();

        const int fieldSize = formatCharSize(formatChar);
        const QVariant value = parseValue(data + offset, size - offset, formatChar);
        if (!value.isValid()) {
            return {};
        }
        if (i < fmt.columns.size()) {
            result[fmt.columns.at(i)] = value;
        }
        offset += fieldSize;
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
                    const MessageCallback &callback,
                    const std::function<void(float)> &progressCallback)
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

        if (progressCallback && (count % 1000 == 0)) {
            progressCallback(static_cast<float>(pos) / static_cast<float>(size));
        }
    }

    return count;
}

} // namespace APMDataFlashUtility
