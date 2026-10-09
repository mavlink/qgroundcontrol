#pragma once

#include "Protocols/Support/GPSProtocolTestBase.h"

/// The Ashtech (Trimble) family: survey receipts, metadata and framing.
class AshtechProtocolTest : public GPSProtocolTestBase
{
    Q_OBJECT

private slots:
    void _scenario_data();
    void _scenario();
};
