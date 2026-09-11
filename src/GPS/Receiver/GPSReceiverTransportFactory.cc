#include "GPSReceiverTransportFactory.h"

#include "TcpGPSTransport.h"
#include "UdpGPSTransport.h"

GPSProvider::TransportFactory GPSReceiverTransportFactory::network(const GPSReceiverProfile& profile)
{
    const auto selected = profile.normalized();
    if (selected.configurationPolicy != GPSReceiverProfile::ConfigurationPolicy::Configure ||
        !selected.validationError().isEmpty()) {
        return {};
    }
    switch (selected.endpoint.kind) {
        case GPSReceiverProfile::Endpoint::Kind::Tcp:
            return [endpoint = selected.endpoint](const std::atomic_bool& stop) {
                return std::make_unique<TcpGPSTransport>(endpoint.host, static_cast<quint16>(endpoint.port), stop);
            };
        case GPSReceiverProfile::Endpoint::Kind::UdpPeer:
            return [endpoint = selected.endpoint](const std::atomic_bool& stop) {
                return std::make_unique<UdpGPSTransport>(endpoint.host, static_cast<quint16>(endpoint.port), stop,
                                                         static_cast<quint16>(endpoint.localPort));
            };
        case GPSReceiverProfile::Endpoint::Kind::Disabled:
        case GPSReceiverProfile::Endpoint::Kind::Serial:
        case GPSReceiverProfile::Endpoint::Kind::UdpListener:
            return {};
    }
    return {};
}
