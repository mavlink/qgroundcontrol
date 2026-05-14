#pragma once

#include <QtCore/QHash>
#include <QtCore/QPointF>
#include <QtCore/QSet>
#include <QtCore/QString>
#include <QtCore/QVector>

#include <map>
#include <memory>
#include <string>

#include <ulog_cpp/data_handler_interface.hpp>
#include <ulog_cpp/messages.hpp>

struct LogParseResult;

/// \brief Full-scan ULog DataHandlerInterface implementation.
///
/// Streams through a ULog file in a single pass, collecting signal samples,
/// parameters, log messages, events, and dropouts into a LogParseResult.
/// Call finalize() after parsing to build mode segments and sort signal lists.
///
class ULogFullHandler final : public ulog_cpp::DataHandlerInterface
{
public:
    explicit ULogFullHandler(LogParseResult &result);
    ~ULogFullHandler() = default;

    void error(const std::string &msg, bool is_recoverable) override;
    void messageFormat(const ulog_cpp::MessageFormat &message_format) override;
    void addLoggedMessage(const ulog_cpp::AddLoggedMessage &add_logged_message) override;
    void headerComplete() override;
    void data(const ulog_cpp::Data &data) override;
    void logging(const ulog_cpp::Logging &logging) override;
    void parameter(const ulog_cpp::Parameter &parameter) override;
    void parameterDefault(const ulog_cpp::ParameterDefault &parameter_default) override;
    void dropout(const ulog_cpp::Dropout &dropout) override;

    bool hadFatalError() const { return _hadFatalError; }
    bool isHeaderComplete() const { return _headerComplete; }

    /// Post-parse: derive mode segments from vehicle_status.nav_state samples
    /// and sort availableFields / plottableFields lists.
    void finalize();

private:
    LogParseResult &_result;

    struct SubscriptionInfo {
        std::shared_ptr<ulog_cpp::MessageFormat> format;
        uint8_t multiId{0};
        std::string topicName;
    };

    std::map<std::string, std::shared_ptr<ulog_cpp::MessageFormat>> _formats;
    std::map<uint16_t, SubscriptionInfo> _subscriptions;
    QSet<QString> _fieldSet;
    QSet<QString> _plottableFieldSet;
    // Map of parameter name -> default value (system default, from ParameterDefault messages)
    QHash<QString, double> _paramDefaults;
    double _lastTimestampSecs{-1.0};
    bool _hadFatalError{false};
    bool _headerComplete{false};
};
