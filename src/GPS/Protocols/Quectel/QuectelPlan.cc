#include "Quectel/QuectelPlan.h"

#include <string>
#include <utility>
#include <variant>

#include "GPSCommandText.h"
#include "Quectel/QuectelCodec_p.h"

namespace Quectel::Plan {

namespace {

using QuectelCodec::Fields;

std::string_view view(QByteArrayView bytes)
{
    return {bytes.data(), static_cast<size_t>(bytes.size())};
}

GPSTextMatcher rateReadback(const MessageRate& rate)
{
    return [message = rate.message, value = rate.rate, version = rate.version](std::string_view body) {
        Fields reply(body);
        // V1.0 documents an empty optional version after standard NMEA rates.
        if (version.empty() && reply.size() == 5 && reply.back().empty()) {
            reply.pop_back();
        }
        unsigned read = 0;
        return QuectelCodec::readback(reply, "PQTMCFGMSGRATE",
                                      reply.size() == (version.empty() ? 4 : 5) && reply[2] == message &&
                                          QuectelCodec::number(reply[3], read) && read == value &&
                                          (version.empty() || reply[4] == version));
    };
}

}  // namespace

QByteArray writeBase(const GPSBaseStationConfig::Mode& mode, const GPSProtocolMath::Ecef& position, bool distanceField)
{
    QByteArray body = "PQTMCFGSVIN,W,";
    if (std::holds_alternative<GPSBaseStationConfig::Fixed>(mode)) {
        body += "2,0,0," + gpsFixedDecimal(position.x, 4) + ',' + gpsFixedDecimal(position.y, 4) + ',' +
                gpsFixedDecimal(position.z, 4);
    } else {
        const auto& survey = std::get<GPSBaseStationConfig::SurveyIn>(mode);
        body += "1," + QByteArray::number(static_cast<qint64>(survey.duration.count())) + ',' +
                gpsFixedDecimal(survey.accuracyMeters, 9) + ",0,0,0";
    }
    if (distanceField) {
        body += ",0";
    }
    return body;
}

GPSCommandSequence::Command command(QByteArrayView body, GPSTextMatcher reply, std::chrono::milliseconds timeout)
{
    return {.step = {body.toByteArray().toStdString(), timeout},
            .wire = QuectelCodec::frame(body),
            .reply = std::move(reply)};
}

GPSTextMatcher acknowledgement(QByteArrayView body)
{
    const qsizetype comma = body.indexOf(',');
    return [name = body.first(comma < 0 ? body.size() : comma).toByteArray()](std::string_view reply) {
        return QuectelCodec::acknowledgement(Fields(reply), view(name));
    };
}

GPSCommandSequence messageRates(std::span<const MessageRate> rates)
{
    GPSCommandSequence sequence;
    for (const auto& rate : rates) {
        const QByteArray message(rate.message.data(), static_cast<qsizetype>(rate.message.size()));
        QByteArray suffix;
        if (!rate.version.empty()) {
            suffix = ',' + QByteArray(rate.version.data(), static_cast<qsizetype>(rate.version.size()));
        }
        const QByteArray write = "PQTMCFGMSGRATE,W," + message + ',' + QByteArray::number(rate.rate) + suffix;
        const QByteArray read = "PQTMCFGMSGRATE,R," + message + suffix;
        sequence.steps.emplace_back(command(write, acknowledgement(write)));
        sequence.steps.emplace_back(command(read, rateReadback(rate)));
    }
    return sequence;
}

}  // namespace Quectel::Plan
