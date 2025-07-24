/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "ULogParser.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QByteArray>
#include <QtCore/QString>

#include <ulog_cpp/data_container.hpp>
#include <ulog_cpp/reader.hpp>

using namespace ulog_cpp;

QGC_LOGGING_CATEGORY(ULogParserLog, "qgc.analyzeview.ulogparser")

namespace ULogParser {

bool getTagsFromLog(const QByteArray &log, QList<GeoTagWorker::CameraFeedbackPacket> &cameraFeedback, QString &errorMessage)
{
    errorMessage.clear();

    std::shared_ptr<DataContainer> data = std::make_shared<DataContainer>(DataContainer::StorageConfig::FullLog);
    Reader parser(data);
    parser.readChunk(reinterpret_cast<const uint8_t*>(log.constData()), log.size());

    if (!data->parsingErrors().empty()) {
        for (const std::string &parsing_error : data->parsingErrors()) {
            (void) errorMessage.append(parsing_error);
            (void) errorMessage.append(", ");
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

} // namespace ULogParser
