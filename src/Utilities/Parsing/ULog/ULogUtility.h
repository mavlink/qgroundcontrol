#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QLoggingCategory>
#include <QtCore/QString>
#include <QtCore/QStringList>

#include <cstdint>
#include <functional>
#include <memory>
#include <set>
#include <string>

#include <ulog_cpp/data_handler_interface.hpp>
#include <ulog_cpp/messages.hpp>
#include <ulog_cpp/subscription.hpp>

Q_DECLARE_LOGGING_CATEGORY(ULogUtilityLog)

namespace ULogUtility
{

// ============================================================================
// Constants
// ============================================================================

/// ULog file magic bytes: "ULog" followed by 0x01 (version 1) followed by 0x12 (file compat)
constexpr char kMagicBytes[] = {'U', 'L', 'o', 'g'};
constexpr int kMagicSize = 4;
constexpr int kHeaderSize = 16;  // Full header size

// ============================================================================
// Header Validation
// ============================================================================

/// Check if data starts with a valid ULog header magic
/// @param data Pointer to data
/// @param size Size of available data
/// @return true if data starts with "ULog" magic bytes
bool isValidHeader(const char *data, qint64 size);

/// Get the ULog format version from header
/// @param data Pointer to data (must be at least 5 bytes)
/// @param size Size of available data
/// @return Version number (typically 1), or -1 if invalid
int getVersion(const char *data, qint64 size);

/// Get the ULog header timestamp (microseconds since epoch)
/// @param data Pointer to data (must be at least kHeaderSize bytes)
/// @param size Size of available data
/// @return Timestamp in microseconds, or 0 if invalid
uint64_t getHeaderTimestamp(const char *data, qint64 size);

// ============================================================================
// Convenience Functions
// ============================================================================

/// Check if data appears to be a ULog file (convenience for QByteArray)
inline bool isValidHeader(const QByteArray &data)
{
    return isValidHeader(data.constData(), data.size());
}

/// Get ULog version (convenience for QByteArray)
inline int getVersion(const QByteArray &data)
{
    return getVersion(data.constData(), data.size());
}

// ============================================================================
// Message Iteration
// ============================================================================

/// Callback for processing ULog messages
/// @param sample The typed data view for the message
/// @return true to continue processing, false to stop
using MessageCallback = std::function<bool(const ulog_cpp::TypedDataView &sample)>;

/// Generic streaming handler for ULog messages by name
/// Filters messages by name and calls a callback for each matching message
class MessageHandler : public ulog_cpp::DataHandlerInterface {
public:
    /// Create a handler that filters for a specific message type
    /// @param messageName Name of the message to filter for (e.g., "camera_capture")
    /// @param callback Function called for each matching message
    /// @param errorMsg Reference to store error messages
    explicit MessageHandler(const std::string &messageName,
                           const MessageCallback &callback,
                           QString &errorMsg);

    // DataHandlerInterface overrides
    void error(const std::string &msg, bool is_recoverable) override;
    void messageFormat(const ulog_cpp::MessageFormat &message_format) override;
    void addLoggedMessage(const ulog_cpp::AddLoggedMessage &add_logged_message) override;
    void headerComplete() override;
    void data(const ulog_cpp::Data &data) override;

    /// Check if a fatal error occurred during parsing
    bool hadFatalError() const { return _hadFatalError; }

    /// Check if the ULog header was successfully parsed
    bool isHeaderComplete() const { return _headerComplete; }

    /// Get the number of messages processed
    int messageCount() const { return _messageCount; }

    /// Check if the target message format was found
    bool hasMessageFormat() const { return _messageFormat != nullptr; }

private:
    std::string _targetMessageName;
    MessageCallback _callback;
    QString &_errorMessage;
    std::shared_ptr<ulog_cpp::MessageFormat> _messageFormat;
    std::set<uint16_t> _messageIds;
    bool _hadFatalError = false;
    bool _headerComplete = false;
    int _messageCount = 0;
};

/// Parse a ULog file and call callback for each matching message
/// @param data Pointer to the ULog data
/// @param size Size of the data in bytes
/// @param messageName Name of the message to filter for
/// @param callback Function called for each matching message
/// @param errorMessage Output error message if parsing fails
/// @return true on success (even if no messages found), false on parse error
bool iterateMessages(const char *data, qint64 size,
                     const std::string &messageName,
                     const MessageCallback &callback,
                     QString &errorMessage);

/// Convenience overload for QByteArray
inline bool iterateMessages(const QByteArray &data,
                            const std::string &messageName,
                            const MessageCallback &callback,
                            QString &errorMessage)
{
    return iterateMessages(data.constData(), data.size(), messageName, callback, errorMessage);
}

} // namespace ULogUtility
