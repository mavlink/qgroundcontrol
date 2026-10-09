#pragma once

#include "Protocols/Support/GPSProtocolTestBase.h"

/// The Septentrio SBF family: block decoding, recorded fixtures, base configuration and survey evidence.
class SBFProtocolTest : public GPSProtocolTestBase
{
    Q_OBJECT

private slots:
    void _scenario_data();
    void _scenario();
};
