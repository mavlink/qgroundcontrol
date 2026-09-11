#include "GPSRecordingFormat.h"

#include <algorithm>
#include <utility>

#include "GPSReceiverProfile.h"

namespace {
using T = GPSRecordingMetadata::Transport;
using K = GPSRecordingEvent::Kind;
}  // namespace

bool GPSRecordingProvenance::valid() const
{
    const auto identifier = [](const QString& text) {
        return text.size() <= 128 && std::all_of(text.cbegin(), text.cend(), [](QChar c) {
                   return (c >= u'a' && c <= u'z') || (c >= u'A' && c <= u'Z') || (c >= u'0' && c <= u'9') ||
                          c == u'.' || c == u'_' || c == u'-' || c == u'+';
               });
    };
    return identifier(producer) && identifier(build);
}

GPSRecordingMetadata GPSRecordingMetadata::forReceiver(const GPSReceiverConfig& config, GPSType type)
{
    GPSRecordingMetadata metadata;
    metadata.receiver = config;
    metadata.driverType = static_cast<int>(type);
    metadata.configured = true;
    return metadata;
}

GPSRecordingMetadata GPSRecordingMetadata::fromProfile(const GPSReceiverProfile& profile)
{
    GPSRecordingMetadata result;
    result.receiver = profile.receiver;
    result.configured = profile.configurationPolicy == GPSReceiverProfile::ConfigurationPolicy::Configure;
    result.driverType = result.configured ? static_cast<int>(profile.driverType) : -1;
    result.initialBaud = profile.endpoint.baud;
    using E = GPSReceiverProfile::Endpoint::Kind;
    switch (profile.endpoint.kind) {
        case E::Disabled:
            result.transport = T::Unknown;
            break;
        case E::Serial:
            result.transport = T::Serial;
            break;
        case E::Tcp:
            result.transport = T::Tcp;
            result.fixedBaud = result.configured ? 115200 : 0;
            break;
        case E::UdpListener:
            result.transport = T::UdpListener;
            result.fixedBaud = result.configured ? 115200 : 0;
            break;
        case E::UdpPeer:
            result.transport = T::UdpPeer;
            result.fixedBaud = result.configured ? 115200 : 0;
            break;
    }
    return result;
}

bool GPSRecordingDocument::selectStream(quint64 requestedStream, QVector<GPSRecordingEvent>& selected,
                                        std::optional<GPSRecordingMetadata>& metadata, quint64& selectedStream,
                                        QString& error) const
{
    const quint64 id = requestedStream ? requestedStream : events.isEmpty() ? 0 : events.first().stream;
    QVector<GPSRecordingEvent> output;
    std::optional<GPSRecordingMetadata> profile;
    for (const auto& event : events) {
        if (event.stream != id) {
            continue;
        }
        output.append(event);
        if (event.kind() == K::Session) {
            profile = event.metadata();
        }
    }
    if (requestedStream && output.isEmpty()) {
        error = QStringLiteral("Requested recording stream was not found");
        return false;
    }
    selected = std::move(output);
    metadata = profile;
    selectedStream = id;
    error.clear();
    return true;
}
