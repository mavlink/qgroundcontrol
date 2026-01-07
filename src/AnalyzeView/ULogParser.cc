#include "ULogParser.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QByteArray>
#include <QtCore/QString>

#include <cmath>
#include <unordered_map>

#include <ulog_cpp/data_container.hpp>
#include <ulog_cpp/data_handler_interface.hpp>
#include <ulog_cpp/reader.hpp>
#include <ulog_cpp/subscription.hpp>

using namespace ulog_cpp;

QGC_LOGGING_CATEGORY(ULogParserLog, "AnalyzeView.ULogParser")

namespace ULogParser {

bool getTagsFromLog(const QByteArray &log, QList<GeoTagWorker::CameraFeedbackPacket> &cameraFeedback, QString &errorMessage)
{
    errorMessage.clear();

    std::shared_ptr<DataContainer> data = std::make_shared<DataContainer>(DataContainer::StorageConfig::FullLog);
    Reader parser(data);
    parser.readChunk(reinterpret_cast<const uint8_t*>(log.constData()), log.size());

    if (!data->parsingErrors().empty()) {
        for (const std::string &parsing_error : data->parsingErrors()) {
            (void) errorMessage.append(QString::fromStdString(parsing_error));
            (void) errorMessage.append(QStringLiteral(", "));
        }
    }

    if (data->hadFatalError()) {
        errorMessage = QStringLiteral("Could not parse ULog");
        return false;
    }

    if (!data->isHeaderComplete()) {
        errorMessage = QStringLiteral("Could not parse ULog header");
        return false;
    }

    const std::set<std::string> subscription_names = data->subscriptionNames();
    if (subscription_names.find("camera_capture") != subscription_names.end()) {
        const std::shared_ptr<Subscription> subscription = data->subscription("camera_capture");
        for (const TypedDataView &sample : *subscription) {
            GeoTagWorker::CameraFeedbackPacket feedback = {0};

            try {
                feedback.timestamp = sample.at("timestamp").as<uint64_t>() / 1.0e6; // to seconds
                feedback.timestampUTC = sample.at("timestamp_utc").as<uint64_t>() / 1.0e6; // to seconds
                feedback.imageSequence = sample.at("seq").as<uint32_t>();
                feedback.latitude = sample.at("lat").as<double>();
                feedback.longitude = sample.at("lon").as<double>();
                feedback.longitude = fmod(180.0 + feedback.longitude, 360.0) - 180.0;
                feedback.altitude = sample.at("alt").as<float>();
                feedback.groundDistance = sample.at("ground_distance").as<float>();
                // feedback.attitude = sample.at("q");
                feedback.captureResult = sample.at("result").as<uint8_t>();

                (void) cameraFeedback.append(feedback);
            } catch (const AccessException &exception) {
                qCDebug(ULogParserLog) << Q_FUNC_INFO << exception.what();
            }
        }
    }

    if (cameraFeedback.isEmpty()) {
        errorMessage = QStringLiteral("Could not detect camera_capture packets in ULog");
        return false;
    }

    return true;
}

class CameraCaptureHandler : public DataHandlerInterface {
public:
    explicit CameraCaptureHandler(QList<GeoTagWorker::CameraFeedbackPacket> &packets, QString &errorMsg);

    void error(const std::string &msg, bool is_recoverable) override;
    void messageFormat(const MessageFormat &message_format) override;
    void addLoggedMessage(const AddLoggedMessage &add_logged_message) override;
    void headerComplete() override;
    void data(const Data &data) override;

    bool hadFatalError() const { return _hadFatalError; }
    bool isHeaderComplete() const { return _headerComplete; }

private:
    QList<GeoTagWorker::CameraFeedbackPacket> &_cameraFeedback;
    QString &_errorMessage;
    std::shared_ptr<MessageFormat> _cameraCaptureFormat;
    std::unordered_map<uint16_t, uint8_t> _cameraCaptureIds;
    bool _hadFatalError = false;
    bool _headerComplete = false;
};

CameraCaptureHandler::CameraCaptureHandler(QList<GeoTagWorker::CameraFeedbackPacket> &packets, QString &errorMsg)
    : _cameraFeedback(packets)
    , _errorMessage(errorMsg)
{
}

void CameraCaptureHandler::error(const std::string &msg, bool is_recoverable)
{
    if (!is_recoverable) {
        _hadFatalError = true;
    }
    (void) _errorMessage.append(QString::fromStdString(msg));
    (void) _errorMessage.append(QStringLiteral(", "));
}

void CameraCaptureHandler::messageFormat(const MessageFormat &message_format)
{
    if (message_format.name() == "camera_capture") {
        _cameraCaptureFormat = std::make_shared<MessageFormat>(message_format);
    }
}

void CameraCaptureHandler::addLoggedMessage(const AddLoggedMessage &add_logged_message)
{
    if (add_logged_message.messageName() == "camera_capture" && _cameraCaptureFormat) {
        _cameraCaptureIds[add_logged_message.msgId()] = add_logged_message.multiId();
    }
}

void CameraCaptureHandler::headerComplete()
{
    _headerComplete = true;
    if (_cameraCaptureFormat) {
        _cameraCaptureFormat->resolveDefinition({{"camera_capture", _cameraCaptureFormat}});
    }
}

void CameraCaptureHandler::data(const Data &data)
{
    if (!_headerComplete || !_cameraCaptureFormat) {
        return;
    }

    auto it = _cameraCaptureIds.find(data.msgId());
    if (it == _cameraCaptureIds.end()) {
        return;
    }

    TypedDataView sample(data, *_cameraCaptureFormat);
    GeoTagWorker::CameraFeedbackPacket feedback = {0};

    try {
        feedback.timestamp = sample.at("timestamp").as<uint64_t>() / 1.0e6;
        feedback.timestampUTC = sample.at("timestamp_utc").as<uint64_t>() / 1.0e6;
        feedback.imageSequence = sample.at("seq").as<uint32_t>();
        feedback.latitude = sample.at("lat").as<double>();
        feedback.longitude = sample.at("lon").as<double>();
        feedback.longitude = fmod(180.0 + feedback.longitude, 360.0) - 180.0;
        feedback.altitude = sample.at("alt").as<float>();
        feedback.groundDistance = sample.at("ground_distance").as<float>();
        feedback.captureResult = sample.at("result").as<uint8_t>();

        (void) _cameraFeedback.append(feedback);
    } catch (const AccessException &exception) {
        qCDebug(ULogParserLog) << Q_FUNC_INFO << exception.what();
    }
}

bool getTagsFromLogStreamed(const QByteArray &log, QList<GeoTagWorker::CameraFeedbackPacket> &cameraFeedback, QString &errorMessage)
{
    errorMessage.clear();

    auto handler = std::make_shared<CameraCaptureHandler>(cameraFeedback, errorMessage);
    Reader parser(handler);
    parser.readChunk(reinterpret_cast<const uint8_t*>(log.constData()), log.size());

    if (handler->hadFatalError()) {
        errorMessage = QStringLiteral("Could not parse ULog");
        return false;
    }

    if (!handler->isHeaderComplete()) {
        errorMessage = QStringLiteral("Could not parse ULog header");
        return false;
    }

    if (cameraFeedback.isEmpty()) {
        errorMessage = QStringLiteral("Could not detect camera_capture packets in ULog");
        return false;
    }

    return true;
}

} // namespace ULogParser
