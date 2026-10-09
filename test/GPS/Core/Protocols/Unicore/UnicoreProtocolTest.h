#pragma once

#include "Protocols/Support/GPSProtocolTestBase.h"

/// The Unicore family: identity, base modes, averaging evidence, command failures and measurement freshness.
class UnicoreProtocolTest : public GPSProtocolTestBase
{
    Q_OBJECT

private slots:
    void _scenario_data();
    void _scenario();
    void _commandFailures_data();
    void _commandFailures();
};
