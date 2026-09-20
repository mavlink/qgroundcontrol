#pragma once

#include "GPSAsciiProtocol.h"

/// Read-only receiver input; setting the local serial baud rate never sends a receiver command.
class GPSNativePassive : public GPSAsciiProtocol
{
public:
    using GPSAsciiProtocol::GPSAsciiProtocol;

    int configure(unsigned& baud, const GPSConfig& config) override;

    bool receiverReady() const override { return _configured; }

private:
    bool _configured = false;
};
