#include "GPSRecordingEvent.h"

#include <type_traits>

GPSRecordingEvent::Kind GPSRecordingEvent::kind() const
{
    return std::visit(
        [](const auto& event) {
            using T = std::decay_t<decltype(event)>;
            if constexpr (std::is_same_v<T, Session>)
                return Kind::Session;
            else if constexpr (std::is_same_v<T, Open>)
                return event.success ? Kind::Open : Kind::OpenError;
            else if constexpr (std::is_same_v<T, Read>) {
                switch (event.outcome) {
                    case Read::Outcome::Data:
                        return Kind::Rx;
                    case Read::Outcome::TimedOut:
                        return Kind::Timeout;
                    case Read::Outcome::Error:
                        return Kind::ReadError;
                    case Read::Outcome::Closed:
                        return Kind::Disconnect;
                    case Read::Outcome::Cancelled:
                        return Kind::Cancel;
                }
                return static_cast<Kind>(-1);
            } else if constexpr (std::is_same_v<T, Write>)
                return event.complete ? Kind::Tx : Kind::WriteError;
            else if constexpr (std::is_same_v<T, Baud>)
                return event.success ? Kind::Baud : Kind::BaudError;
            else if constexpr (std::is_same_v<T, Configuration>)
                return event.status ? Kind::ConfigurationFinished : Kind::ConfigurationStarted;
            else if constexpr (std::is_same_v<T, BoundedWrite>)
                return Kind::BoundedWrite;
            else
                return Kind::Close;
        },
        payload);
}

const QByteArray& GPSRecordingEvent::bytes() const
{
    static const QByteArray empty;
    return std::visit(
        [](const auto& event) -> const QByteArray& {
            if constexpr (requires { event.bytes; })
                return event.bytes;
            else
                return empty;
        },
        payload);
}

int GPSRecordingEvent::value() const
{
    return std::visit(
        [](const auto& event) {
            if constexpr (requires { event.value; })
                return event.value;
            else if constexpr (std::is_same_v<std::decay_t<decltype(event)>, Configuration>)
                return event.status.value_or(0);
            else
                return 0;
        },
        payload);
}

const GPSRecordingMetadata& GPSRecordingEvent::metadata() const
{
    static const GPSRecordingMetadata empty;
    const auto* session = std::get_if<Session>(&payload);
    return session ? session->metadata : empty;
}

bool GPSRecordingEvent::resumed() const
{
    const auto* open = std::get_if<Open>(&payload);
    return open && open->resumed;
}

bool GPSRecordingEvent::fatal() const
{
    return std::visit(
        [](const auto& event) {
            if constexpr (requires { event.fatal; })
                return event.fatal;
            else
                return false;
        },
        payload);
}

std::optional<GPSWriteResult> GPSRecordingEvent::writeResult() const
{
    const auto* write = std::get_if<BoundedWrite>(&payload);
    return write ? std::optional(write->result) : std::nullopt;
}

std::optional<qint64> GPSRecordingEvent::receivedAtUs() const
{
    const auto* read = std::get_if<Read>(&payload);
    return read ? read->receivedAtUs : std::nullopt;
}

std::optional<GPSOpenStatus> GPSRecordingEvent::openStatus() const
{
    const auto* open = std::get_if<Open>(&payload);
    return open ? open->status : std::nullopt;
}

std::optional<GPSReadStatus> GPSRecordingEvent::readStatus() const
{
    const auto* read = std::get_if<Read>(&payload);
    return read ? read->status : std::nullopt;
}
