#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QLoggingCategory>
#include <QtCore/QMap>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QVariant>

#include <cstdint>
#include <functional>

Q_DECLARE_LOGGING_CATEGORY(DataFlashUtilityLog)

namespace DataFlashUtility
{

// ============================================================================
// Constants
// ============================================================================

constexpr uint8_t kHeaderByte1 = 0xA3;
constexpr uint8_t kHeaderByte2 = 0x95;
constexpr uint8_t kFmtMessageType = 128;
constexpr int kFmtPayloadSize = 86;

// ============================================================================
// Message Format Structure
// ============================================================================

struct MessageFormat {
    uint8_t type = 0;
    uint8_t length = 0;
    QString name;
    QString format;
    QStringList columns;
};

// ============================================================================
// Format Character Functions
// ============================================================================

/// Get the byte size for a DataFlash format character
/// @param c Format character (from pymavlink FORMAT_TO_STRUCT)
/// @return Size in bytes, or 0 for unknown format characters
int formatCharSize(char c);

/// Calculate total payload size from a format string
/// @param format Format string (e.g., "QBILLefffff")
/// @return Total size in bytes
int calculatePayloadSize(const QString &format);

// ============================================================================
// Value Parsing
// ============================================================================

/// Parse a value from binary data based on format character
/// Handles scaling for special types:
/// - 'c'/'C': Centi-degrees (divide by 100)
/// - 'e'/'E': Centi-units (divide by 100)
/// - 'L': Lat/Lon (divide by 1e7)
/// - 'g': Half-precision float (IEEE 754 binary16)
/// @param data Pointer to binary data
/// @param formatChar Format character
/// @return QVariant containing the parsed value
QVariant parseValue(const char *data, char formatChar);

/// Parse message data into a map of field name -> value
/// @param data Pointer to the message payload (after header)
/// @param fmt Message format definition
/// @return Map of column name to parsed value
QMap<QString, QVariant> parseMessage(const char *data, const MessageFormat &fmt);

// ============================================================================
// Header and Message Detection
// ============================================================================

/// Check if data starts with a valid DataFlash header
/// @param data Pointer to data
/// @param size Size of available data
/// @return true if data starts with 0xA3 0x95
bool isValidHeader(const char *data, qint64 size);

/// Find the next message header starting from offset
/// @param data Pointer to data
/// @param size Total size of data
/// @param offset Starting position to search from
/// @return Offset of next header, or -1 if not found
qint64 findNextHeader(const char *data, qint64 size, qint64 offset);

// ============================================================================
// FMT Message Parsing
// ============================================================================

/// Parse an FMT message payload into a MessageFormat structure
/// @param data Pointer to FMT payload (86 bytes: Type(1)+Length(1)+Name(4)+Format(16)+Columns(64))
/// @return Parsed MessageFormat
MessageFormat parseFmtPayload(const char *data);

/// Parse all FMT messages from a DataFlash log (first pass)
/// @param data Pointer to complete log data
/// @param size Size of log data
/// @param formats Output map of message type -> MessageFormat
/// @return true if any FMT messages were found
bool parseFmtMessages(const char *data, qint64 size, QMap<uint8_t, MessageFormat> &formats);

// ============================================================================
// Message Iteration
// ============================================================================

/// Callback for message iteration
/// @param msgType Message type
/// @param payload Pointer to message payload (after header)
/// @param payloadSize Size of payload
/// @param fmt MessageFormat for this message type
/// @return true to continue iteration, false to stop
using MessageCallback = std::function<bool(uint8_t msgType, const char *payload,
                                            int payloadSize, const MessageFormat &fmt)>;

/// Iterate over all messages in a DataFlash log
/// @param data Pointer to complete log data
/// @param size Size of log data
/// @param formats Message formats from parseFmtMessages()
/// @param callback Function called for each message
/// @return Number of messages processed
int iterateMessages(const char *data, qint64 size,
                    const QMap<uint8_t, MessageFormat> &formats,
                    const MessageCallback &callback);

// ============================================================================
// Half-Precision Float Conversion
// ============================================================================

/// Convert IEEE 754 binary16 (half-precision) to single-precision float
/// @param bits 16-bit half-precision representation
/// @return Converted float value
float halfToFloat(uint16_t bits);

} // namespace DataFlashUtility
