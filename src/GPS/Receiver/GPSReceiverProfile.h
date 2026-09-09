#pragma once

#include <QtCore/QString>

#include "GPSDriver.h"

/// Runtime receiver intent, independent of the saved NMEA/RTK settings layout.
struct GPSReceiverProfile
{
    struct Endpoint
    {
        enum class Kind
        {
            Disabled,
            Serial,
            Tcp,
            UdpListener,
            UdpPeer,
        };

        Kind kind = Kind::Disabled;
        QString device;
        QString host;
        int port = 0;
        int localPort = 0;
        int baud = 0;
        bool discoverSerialDevice = false;

        bool operator==(const Endpoint&) const = default;
    };

    enum class ConfigurationPolicy
    {
        Passive,
        Configure,
    };

    Endpoint endpoint;
    ConfigurationPolicy configurationPolicy = ConfigurationPolicy::Passive;
    GPSType driverType = GPSType::u_blox;
    GPSReceiverConfig receiver{.role = GPSReceiverConfig::Role::Position,
                               .outputProtocol = GPSReceiverConfig::OutputProtocol::NMEA,
                               .base = {}};
    QString receiverName;

    /// Drop inactive fields so edits to other transport/role settings do not replace a live session.
    GPSReceiverProfile normalized() const;
    QString validationError() const;
    QString networkHost() const;
    bool operator==(const GPSReceiverProfile& other) const;
};
