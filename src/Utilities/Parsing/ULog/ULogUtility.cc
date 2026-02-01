#include "ULogUtility.h"
#include <QtCore/QLoggingCategory>

#include <cstring>

#include <ulog_cpp/reader.hpp>

Q_STATIC_LOGGING_CATEGORY(ULogUtilityLog, "Utilities.ULogUtility")

namespace ULogUtility
{

// ============================================================================
// Header Validation
// ============================================================================

bool isValidHeader(const char *data, qint64 size)
{
    if (size < kMagicSize) {
        return false;
    }
    return memcmp(data, kMagicBytes, kMagicSize) == 0;
}

int getVersion(const char *data, qint64 size)
{
    if (size < 5) {
        return -1;
    }
    if (!isValidHeader(data, size)) {
        return -1;
    }
    return static_cast<uint8_t>(data[4]);
}

uint64_t getHeaderTimestamp(const char *data, qint64 size)
{
    if (size < kHeaderSize) {
        return 0;
    }
    if (!isValidHeader(data, size)) {
        return 0;
    }

    // Timestamp is at offset 8 (after magic[4] + version[1] + compat[1] + flags[2])
    uint64_t timestamp;
    memcpy(&timestamp, data + 8, sizeof(timestamp));
    return timestamp;
}

// ============================================================================
// MessageHandler Implementation
// ============================================================================

MessageHandler::MessageHandler(const std::string &messageName,
                               const MessageCallback &callback,
                               QString &errorMsg)
    : _targetMessageName(messageName)
    , _callback(callback)
    , _errorMessage(errorMsg)
{
}

void MessageHandler::error(const std::string &msg, bool is_recoverable)
{
    if (!is_recoverable) {
        _hadFatalError = true;
    }
    if (!_errorMessage.isEmpty()) {
        _errorMessage.append(QStringLiteral(", "));
    }
    _errorMessage.append(QString::fromStdString(msg));
}

void MessageHandler::messageFormat(const ulog_cpp::MessageFormat &message_format)
{
    if (message_format.name() == _targetMessageName) {
        _messageFormat = std::make_shared<ulog_cpp::MessageFormat>(message_format);
    }
}

void MessageHandler::addLoggedMessage(const ulog_cpp::AddLoggedMessage &add_logged_message)
{
    if (add_logged_message.messageName() == _targetMessageName && _messageFormat) {
        _messageIds.insert(add_logged_message.msgId());
    }
}

void MessageHandler::headerComplete()
{
    _headerComplete = true;
    if (_messageFormat) {
        _messageFormat->resolveDefinition({{_targetMessageName, _messageFormat}});
    }
}

void MessageHandler::data(const ulog_cpp::Data &data)
{
    if (!_headerComplete || !_messageFormat) {
        return;
    }

    if (!_messageIds.contains(data.msgId())) {
        return;
    }

    try {
        const ulog_cpp::TypedDataView typedData(data, *_messageFormat);
        ++_messageCount;
        if (!_callback(typedData)) {
            // Callback returned false, stop processing
            // Note: ulog_cpp doesn't support early termination, so we just skip remaining
        }
    } catch (const ulog_cpp::AccessException &exception) {
        qCWarning(ULogUtilityLog) << "Failed to parse" << QString::fromStdString(_targetMessageName)
                                  << ":" << exception.what();
        QStringList fields;
        for (const std::string &name : _messageFormat->fieldNames()) {
            fields.append(QString::fromStdString(name));
        }
        qCDebug(ULogUtilityLog) << "Available fields:" << fields;
    }
}

// ============================================================================
// Message Iteration
// ============================================================================

bool iterateMessages(const char *data, qint64 size,
                     const std::string &messageName,
                     const MessageCallback &callback,
                     QString &errorMessage)
{
    errorMessage.clear();

    auto handler = std::make_shared<MessageHandler>(messageName, callback, errorMessage);
    ulog_cpp::Reader parser(handler);
    parser.readChunk(reinterpret_cast<const uint8_t*>(data), static_cast<size_t>(size));

    if (handler->hadFatalError()) {
        errorMessage = QStringLiteral("Could not parse ULog");
        return false;
    }

    if (!handler->isHeaderComplete()) {
        errorMessage = QStringLiteral("Could not parse ULog header");
        return false;
    }

    return true;
}

} // namespace ULogUtility
