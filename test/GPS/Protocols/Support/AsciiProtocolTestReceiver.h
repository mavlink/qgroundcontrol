#pragma once

#include "GPSAsciiProtocol.h"

namespace GPSTest {

class AsciiProtocolTestReceiver final : public GPSAsciiProtocol
{
public:
    using GPSAsciiProtocol::GPSAsciiProtocol;

    bool configure(unsigned&, const GPSConfig&) override
    {
        resetStream();
        return true;
    }
};

}  // namespace GPSTest
