#include "Quectel/QuectelPlan.h"

#include <utility>
#include <variant>

#include "NMEASentence.h"
#include "Quectel/QuectelCodec.h"

namespace Quectel::Plan {

namespace {

using QuectelCodec::Fields;
namespace Field = QuectelCodec::Field;

GPSTextMatcher rateReadback(const MessageRate& rate)
{
    return [message = rate.message, value = rate.rate, version = rate.version](std::string_view body) {
        Fields reply(body);
        // V1.0 documents an empty optional version after standard NMEA rates.
        if (version.empty() && reply.size() == Field::RATE_MAX_FIELDS && reply.back().empty()) {
            reply.pop_back();
        }
        unsigned read = 0;
        return QuectelCodec::readback(
            reply, "PQTMCFGMSGRATE",
            reply.size() == (version.empty() ? Field::RATE_MIN_FIELDS : Field::RATE_MAX_FIELDS) &&
                reply[Field::RATE_MESSAGE] == message && QuectelCodec::number(reply[Field::RATE_VALUE], read) &&
                read == value && (version.empty() || reply[Field::RATE_VERSION] == version));
    };
}

}  // namespace

QByteArray writeBase(const GPSBaseStationConfig::Mode& mode, const GPSProtocolMath::Ecef& position, bool distanceField)
{
    QByteArray body = "PQTMCFGSVIN,W,";
    if (std::holds_alternative<GPSBaseStationConfig::Fixed>(mode)) {
        body += QByteArray::number(BaseMode::FIXED) + ",0,0," + gpsFixedDecimal(position.x, ECEF_DECIMALS) + ',' +
                gpsFixedDecimal(position.y, ECEF_DECIMALS) + ',' + gpsFixedDecimal(position.z, ECEF_DECIMALS);
    } else if (const auto* survey = std::get_if<GPSBaseStationConfig::SurveyIn>(&mode)) {
        body += QByteArray::number(BaseMode::SURVEY_IN) + ',' +
                QByteArray::number(static_cast<qint64>(survey->duration.count())) + ',' +
                gpsFixedDecimal(survey->accuracyMeters, SURVEY_ACCURACY_DECIMALS) + ",0,0,0";
    }
    if (distanceField) {
        body += ",0";
    }
    return body;
}

GPSCommandSequence::Command command(QByteArrayView body, GPSTextMatcher reply, std::chrono::milliseconds timeout)
{
    return gpsCommand(NMEAUtils::frame(body), timeout, std::move(reply), {.label = body.toByteArray()});
}

GPSTextMatcher acknowledgement(QByteArrayView body)
{
    const qsizetype comma = body.indexOf(',');
    return [name = body.first(comma < 0 ? body.size() : comma).toByteArray()](std::string_view reply) {
        return QuectelCodec::acknowledgement(Fields(reply), QByteArrayView(name));
    };
}

GPSCommandSequence messageRates(std::span<const MessageRate> rates)
{
    GPSCommandSequence sequence;
    for (const auto& rate : rates) {
        const QByteArray message = QByteArrayView(rate.message).toByteArray();
        QByteArray suffix;
        if (!rate.version.empty()) {
            suffix = ',' + QByteArrayView(rate.version).toByteArray();
        }
        const QByteArray write = "PQTMCFGMSGRATE,W," + message + ',' + QByteArray::number(rate.rate) + suffix;
        const QByteArray read = "PQTMCFGMSGRATE,R," + message + suffix;
        sequence.steps.emplace_back(command(write, acknowledgement(write)));
        sequence.steps.emplace_back(command(read, rateReadback(rate)));
    }
    return sequence;
}

}  // namespace Quectel::Plan
